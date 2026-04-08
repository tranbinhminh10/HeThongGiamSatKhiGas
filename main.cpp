#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h> 
#include <WebServer.h>
#include <ESP_Mail_Client.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <HTTPClient.h> 
#include "webpage.h"

// ================= CẤU HÌNH PWM CÒI (LEDC) =================
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 1000
#define BUZZER_RESOLUTION 8

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Telegram config 
#define BOT_TOKEN "8747382601:AAGGLz2DrzrxmosvnEz1py9_cUm0mxT8gLM"
#define CHAT_ID "1668510441"

// Email config
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "binhminhtran10@gmail.com"
#define AUTHOR_PASSWORD "gzskgtmgfnrwfbeg"
#define RECIPIENT_EMAIL "22T1020667@husc.edu.vn"

// Objects
SMTPSession smtp;
WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin config
const int gasPin = 34;
const int relayPin = 5;
const int buzzerPin = 4;
const int pwmPin = 18;
const int buttonPin = 19; 

// Biến hệ thống
int gasThreshold = 1800;
unsigned long lastCheckTime = 0;
bool emailSent = false;
bool startupMsgSent = false;
int alertGasValue = 0; 

// Biến Còi & Nút nhấn
bool isMuted = false;        
bool isBuzzerOn = false;     
int manualPwmVal = 0;        
unsigned long buttonPressTime = 0;
bool lastButtonState = HIGH;
bool isTesting = false;

// ================= TELEGRAM =================
void sendTelegramMessage(String message) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client; 
        client.setInsecure();    
        
        HTTPClient http;
        message.replace(" ", "%20"); 
        
        String url = "https://api.telegram.org/bot";
        url += BOT_TOKEN;
        url += "/sendMessage?chat_id=";
        url += CHAT_ID;
        url += "&text=";
        url += message;

        http.begin(client, url); 
        int httpResponseCode = http.GET();
        
        if (httpResponseCode > 0) {
            Serial.println("✅ Telegram OK!");
        } else {
            Serial.print("❌ Telegram Loi: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
}

// ================= EMAIL =================
void sendAlertEmail(int gasVal) {
    ESP_Mail_Session session;
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    SMTP_Message message;
    message.sender.name = "He thong Bao chay";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "CANH BAO RO RI GAS!";
    message.addRecipient("Chu nha", RECIPIENT_EMAIL);

    String htmlMsg = "<h2 style='color:red;'>Gas cao!</h2><p>Gia tri: ";
    htmlMsg += gasVal;
    htmlMsg += "</p>";

    message.html.content = htmlMsg.c_str();

    if (!smtp.connect(&session)) { return; }
    MailClient.sendMail(&smtp, &message);
}

// ================= TASK CHẠY NGẦM TRÊN CORE 0 =================
void networkTask(void *pvParameters) {
    String tMsg = "🚨 CANH_BAO_GAS_MUC_";
    tMsg += alertGasValue;
    
    sendTelegramMessage(tMsg);
    sendAlertEmail(alertGasValue);
    
    vTaskDelete(NULL); 
}

// ================= WEB =================
void setupWebServer() {
    server.on("/", []() { server.send(200, "text/html", PAGE_HTML); });
    
    server.on("/getGas", []() {
        String json = "{\"gas\":";
        json += analogRead(gasPin);
        json += "}";
        server.send(200, "application/json", json);
    });
    
    server.on("/relay", []() {
        digitalWrite(relayPin, server.arg("state") == "ON" ? HIGH : LOW);
        server.send(200, "text/plain", "OK");
    });
    
    server.on("/pwm", []() {
        manualPwmVal = server.arg("value").toInt();
        if (analogRead(gasPin) <= gasThreshold && !isTesting) {
            analogWrite(pwmPin, manualPwmVal);
        }
        server.send(200, "text/plain", "OK");
    });
    
    server.begin();
}

// ================= SETUP =================
void setup() {
    Serial.begin(115200);

    pinMode(relayPin, OUTPUT);
    pinMode(pwmPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP); 

    // Khởi tạo PWM cứng cho còi
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RESOLUTION);
    ledcAttachPin(buzzerPin, BUZZER_CHANNEL);
    ledcWrite(BUZZER_CHANNEL, 0); // Tắt còi khi vừa khởi động

    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();
    lcd.print("Connecting WiFi");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); Serial.print(".");
    }

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WiFi Connected!");
    
    setupWebServer();
}

// ================= LOOP =================
void loop() {
    server.handleClient();

    if (!startupMsgSent && WiFi.status() == WL_CONNECTED) {
        delay(2000);
        sendTelegramMessage("✅ He_thong_khoi_dong_thanh_cong!");
        startupMsgSent = true;
    }

    // ---------------- LẮNG NGHE NÚT NHẤN ----------------
    bool currentButtonState = digitalRead(buttonPin);

    if (lastButtonState == HIGH && currentButtonState == LOW) {
        buttonPressTime = millis();
    }

    if (lastButtonState == LOW && currentButtonState == HIGH) {
        unsigned long pressDuration = millis() - buttonPressTime;

        if (pressDuration > 3000) {
            // TEST MODE
            isTesting = !isTesting;

            if (isTesting) {
                isMuted = false;
                ledcWrite(BUZZER_CHANNEL, 128); // 🔥 Bật còi (Duty cycle 50%)
                isBuzzerOn = true;
                digitalWrite(relayPin, HIGH);
                analogWrite(pwmPin, 255);
                lcd.clear();
                lcd.print("--- TESTING ---");
            } else {
                ledcWrite(BUZZER_CHANNEL, 0); // 🔥 Tắt còi hoàn toàn
                isBuzzerOn = false;
                digitalWrite(relayPin, LOW);
                analogWrite(pwmPin, manualPwmVal);
                lcd.clear();
            }
        } 
        else if (pressDuration > 50) {
            // MUTE
            isMuted = true;
            ledcWrite(BUZZER_CHANNEL, 0); // 🔥 Tắt còi hoàn toàn
            isBuzzerOn = false;

            Serial.println(">>> DA KHOA COI (MUTE)!");
        }
    }
    lastButtonState = currentButtonState;

    // ---------------- KIỂM TRA GAS ----------------
    if (!isTesting) { 
        if (millis() - lastCheckTime >= 1500) {
            lastCheckTime = millis();
            int gasVal = analogRead(gasPin);

            lcd.setCursor(0, 0); 
            lcd.print("Gas: ");
            lcd.print(gasVal);
            lcd.print("    "); 

            if (gasVal > gasThreshold) {
                digitalWrite(relayPin, HIGH);
                analogWrite(pwmPin, 255); 
                lcd.setCursor(0, 1); lcd.print("!!! WARNING !!!");

                if (isMuted == false) {
                    if (isBuzzerOn == false) { 
                        ledcWrite(BUZZER_CHANNEL, 128); // 🔥 Bật còi
                        isBuzzerOn = true;
                    }
                } else {
                    ledcWrite(BUZZER_CHANNEL, 0); // 🔥 Tắt còi hoàn toàn
                    isBuzzerOn = false;
                }

                if (!emailSent) {
                    alertGasValue = gasVal; 
                    emailSent = true;
                    
                    xTaskCreatePinnedToCore(
                        networkTask, 
                        "TaskNetwork", 
                        10000, 
                        NULL, 
                        1, 
                        NULL, 
                        0 
                    );
                }
            } else {
                lcd.setCursor(0, 1); lcd.print("Status: Normal ");
                digitalWrite(relayPin, LOW);
                ledcWrite(BUZZER_CHANNEL, 0); // 🔥 Tắt còi hoàn toàn
                isBuzzerOn = false; 
                analogWrite(pwmPin, manualPwmVal); 
                
                emailSent = false;
                isMuted = false; 
            }
        }
    }
}