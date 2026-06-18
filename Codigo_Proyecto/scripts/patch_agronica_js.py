from pathlib import Path
path = Path('main/agronica.c')
text = path.read_text(encoding='utf-8')
start = text.index('"<script>"')
end = text.index('"</script></body></html>"', start) + len('"</script></body></html>"')
replacement = '''    "<script>"
    "const chartData = { humidity: [], light: [], labels: [], maxPoints: 12 };"
    "function drawChart() {const canvas = document.getElementById('sensor_chart'); if (!canvas) return; const ctx = canvas.getContext('2d'); const w = canvas.width; const h = canvas.height; ctx.clearRect(0, 0, w, h); ctx.fillStyle = 'rgba(255,255,255,0.08)'; ctx.fillRect(0, 0, w, h); ctx.fillStyle = '#f0f4f8'; ctx.font = '12px Arial'; if (chartData.labels.length > 0) { ctx.fillText(chartData.labels[chartData.labels.length - 1], 10, h - 10); } }"
    "function addPoint(label, hum, light) { if (chartData.labels.length >= chartData.maxPoints) { chartData.labels.shift(); chartData.humidity.shift(); chartData.light.shift(); } chartData.labels.push(label); chartData.humidity.push(hum); chartData.light.push(light); drawChart(); }"
    "function updateClock() {const now = new Date();document.getElementById('clock').innerText = now.toLocaleTimeString();document.getElementById('date').innerText = now.toLocaleDateString();}"
    "function updateData() {fetch('/data').then(res => res.json()).then(data => {document.getElementById('humidity').innerText = data.humidity;document.getElementById('light').innerText = data.light;document.getElementById('valve_status').innerText = data.valveStatus;document.getElementById('pump_status').innerText = data.pumpStatus;document.getElementById('last_update').innerText = new Date(data.timestamp).toLocaleTimeString();document.getElementById('wifi_status').innerText = data.wifiStatus;document.getElementById('valve_status').className = data.valveStatus === 'ON' ? 'status on' : 'status off';document.getElementById('pump_status').className = data.pumpStatus === 'ON' ? 'status on' : 'status off';document.getElementById('auto_mode').innerText = data.autoMode;var link = document.getElementById('dashboard_link');link.href = 'http://' + data.ip;link.innerText = data.ip === 'Sin IP' ? 'Sin IP' : 'http://' + data.ip; addPoint(new Date(data.timestamp).toLocaleTimeString(), Number(data.humidity), Number(data.light));});}"
    "window.onload = function() {updateClock();updateData();setInterval(updateClock,1000);setInterval(updateData,3000);};"
    "</script></body></html>";'''
path.write_text(text[:start] + replacement + text[end:], encoding='utf-8')
print('patched')
