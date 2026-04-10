#ifndef WEBPAGE_H
#define WEBPAGE_H
#include <Arduino.h>

const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Gas Monitor Dashboard</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<link href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;800&display=swap" rel="stylesheet">

<style>
  :root {
    --bg-color: #0f172a;
    --card-bg: #1e293b;
    --border-color: #334155;
    --safe: #10b981;
    --warn: #f59e0b;
    --danger: #ef4444;
    --text-main: #f8fafc;
    --text-muted: #94a3b8;
  }

  * { box-sizing: border-width: 0; margin: 0; padding: 0; }

  body {
    font-family: 'Poppins', sans-serif;
    background: var(--bg-color);
    color: var(--text-main);
    display: flex;
    justify-content: center;
    padding: 20px;
    min-height: 100vh;
  }

  .dashboard {
    width: 100%;
    max-width: 900px;
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
  }

  .header {
    grid-column: 1 / -1;
    text-align: center;
    padding: 10px 0 20px 0;
  }

  .header h2 { font-size: 24px; font-weight: 800; letter-spacing: 1px; color: #38bdf8; }

  .card {
    background: var(--card-bg);
    border: 1px solid var(--border-color);
    border-radius: 20px;
    padding: 25px;
    box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.3);
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
  }

  .chart-card { grid-column: 1 / -1; }

  /* Typography */
  .title { font-size: 14px; color: var(--text-muted); text-transform: uppercase; font-weight: 600; margin-bottom: 10px;}
  .value-display { font-size: 48px; font-weight: 800; line-height: 1; margin-bottom: 5px; transition: color 0.3s; }
  .status-badge { 
    padding: 6px 16px; border-radius: 20px; font-size: 14px; font-weight: 600; 
    background: rgba(255,255,255,0.1); border: 1px solid transparent;
  }

  /* Buttons */
  .btn-group { display: flex; gap: 10px; width: 100%; margin-top: 10px; }
  button {
    flex: 1; padding: 12px; font-family: inherit; font-size: 16px; font-weight: 600;
    color: white; border: none; border-radius: 12px; cursor: pointer;
    transition: transform 0.1s, filter 0.2s;
  }
  button:hover { filter: brightness(1.1); }
  button:active { transform: scale(0.96); }
  .on { background: linear-gradient(135deg, #059669 0%, #10b981 100%); box-shadow: 0 4px 15px rgba(16, 185, 129, 0.3); }
  .off { background: linear-gradient(135deg, #b91c1c 0%, #ef4444 100%); box-shadow: 0 4px 15px rgba(239, 68, 68, 0.3); }

  /* Slider */
  .slider-container { width: 100%; margin-top: 15px; }
  .slider {
    -webkit-appearance: none; width: 100%; height: 8px; border-radius: 5px;
    background: #334155; outline: none; margin: 15px 0;
  }
  .slider::-webkit-slider-thumb {
    -webkit-appearance: none; appearance: none;
    width: 24px; height: 24px; border-radius: 50%;
    background: #38bdf8; cursor: pointer; box-shadow: 0 0 10px rgba(56, 189, 248, 0.5);
  }
  .pwm-text { font-size: 14px; color: var(--text-muted); display: flex; justify-content: space-between;}

  @media (max-width: 600px) {
    .dashboard { grid-template-columns: 1fr; }
  }
</style>
</head>
<body>

<div class="dashboard">
  <div class="header">
    <h2>🔥 GAS SENSOR DASHBOARD</h2>
  </div>

  <div class="card">
    <div class="title">Current Concentration</div>
    <div class="value-display" id="gasValue">0%</div>
    <div class="status-badge" id="status">Connecting...</div>
  </div>

  <div class="card">
    <div class="title">Device Controls</div>
    <div class="btn-group">
      <button class="on" onclick="relay('ON')">Relay ON</button>
      <button class="off" onclick="relay('OFF')">Relay OFF</button>
    </div>
    
    <div class="slider-container">
      <div class="pwm-text"><span>Fan Speed (PWM)</span> <span id="pwmVal">0%</span></div>
      <input class="slider" type="range" min="0" max="255" value="0" oninput="setPWM(this.value)">
    </div>
  </div>

  <div class="card chart-card">
    <canvas id="gasChart" width="400" height="150"></canvas>
  </div>
</div>

<script>
let gasData = [];
let labels = [];
const ctx = document.getElementById('gasChart').getContext('2d');

// Gradient for chart fill
let gradientSafe = ctx.createLinearGradient(0, 0, 0, 400);
gradientSafe.addColorStop(0, 'rgba(16, 185, 129, 0.4)');
gradientSafe.addColorStop(1, 'rgba(16, 185, 129, 0)');

const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: labels,
    datasets: [{
      label: 'Gas Level (PPM)',
      data: gasData,
      borderWidth: 3,
      borderColor: '#10b981',
      backgroundColor: gradientSafe,
      fill: true,
      tension: 0.4, // Make the line smooth
      pointRadius: 0, // Hide points for a cleaner look
    }]
  },
  options: {
    responsive: true,
    animation: false,
    plugins: {
      legend: { display: false }
    },
    scales: {
      x: { display: false }, // Hide X axis for modern look
      y: { 
        grid: { color: '#334155' },
        ticks: { color: '#94a3b8' }, 
        beginAtZero: true, 
        max: 4095 
      }
    }
  }
});

// Realtime Update Logic
setInterval(() => {
  fetch('/getGas')
  .then(res => res.json())
  .then(data => {
    let gas = data.gas;
    let percent = Math.floor((gas / 4095) * 100);

    // Update Text
    const gasValueEl = document.getElementById("gasValue");
    const statusEl = document.getElementById("status");
    gasValueEl.innerText = percent + "%";

    let statusText = "";
    let color = "";
    let bgColor = "";

    // Threshold logic
    if (gas < 1200) {
      statusText = "SAFE";
      color = "var(--safe)";
      bgColor = "rgba(16, 185, 129, 0.2)";
    } 
    else if (gas < 1800) {
      statusText = "WARNING";
      color = "var(--warn)";
      bgColor = "rgba(245, 158, 11, 0.2)";
    } 
    else {
      statusText = "DANGER";
      color = "var(--danger)";
      bgColor = "rgba(239, 68, 68, 0.2)";
    }

    gasValueEl.style.color = color;
    statusEl.innerText = statusText;
    statusEl.style.color = color;
    statusEl.style.borderColor = color;
    statusEl.style.backgroundColor = bgColor;

    // Update Chart
    gasData.push(gas);
    labels.push("");
    if (gasData.length > 25) { // Show more data points
      gasData.shift();
      labels.shift();
    }

    // Dynamic Chart Colors
    chart.data.datasets[0].borderColor = color;
    chart.update();
  });
}, 1500);

function relay(state) {
  fetch('/relay?state=' + state);
}

function setPWM(val) {
  let percent = Math.floor((val / 255) * 100);
  document.getElementById("pwmVal").innerText = percent + "%";
  fetch('/pwm?value=' + val);
}
</script>

</body>
</html>
)rawliteral";

#endif