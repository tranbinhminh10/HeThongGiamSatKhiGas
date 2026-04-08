# 🔥 Hệ thống giám sát khí gas và điều khiển cảnh báo an toàn trong gia đình sử dụng Web Server

Dự án sử dụng vi điều khiển **ESP32** để giám sát nồng độ khí Gas, tự động báo động và gửi thông báo từ xa qua Internet.

## ✨ Tính năng nổi bật
* **Đa nhiệm (Dual-Core):** Sử dụng FreeRTOS để tách biệt tác vụ đọc cảm biến và tác vụ mạng, đảm bảo không có độ trễ.
* **Cảnh báo đa kênh:** * Tại chỗ: Còi báo động (PWM) và Relay quạt hút.
  * Từ xa: Thông báo qua **Telegram Bot** và **Email (SMTP)**.
* **Giao diện Web:** Theo dõi nồng độ Gas và điều khiển thiết bị qua trình duyệt.
* **Chế độ thông minh:** Hỗ trợ nút nhấn vật lý để **Mute** (tắt còi) hoặc **Test Mode**.

## 🛠 Linh kiện sử dụng
* ESP32 DevKit V1
* Cảm biến Gas (giả lập bằng Potentiometer trên Wokwi)
* LCD 16x2 I2C
* Buzzer, LED (mô phỏng Relay/Fan), Push Button

## 🚀 Hướng dẫn chạy mô phỏng
1. Truy cập vào [Wokwi.com](https://wokwi.com).
2. Chọn dự án ESP32.
3. Copy nội dung file `diagram.json` dán vào tab sơ đồ mạch.
4. Copy code từ `HeThongBaoChay.ino` và `webpage.h` vào các tab tương ứng.
5. Nhấn **Play** để bắt đầu.

---
*Dự án được thực hiện bởi: Binh Minh Tran*