// ESP32 Robot Control System - JavaScript Module
console.log('🤖 ESP32 Robot Control Loading from SPIFFS...');

const img = document.getElementById('stream');
const status = document.getElementById('status');

// === FUNCIONES DE ESTADO Y LOGGING ===
function appendStatus(s, type = 'info') { 
  const timestamp = new Date().toLocaleTimeString();
  const className = type === 'error' ? 'status-error' : 
                   type === 'success' ? 'status-good' : 
                   type === 'warning' ? 'status-warning' : '';
  const line = `${timestamp} - ${s}`;
  status.innerHTML = `<span class="${className}">${line}</span>\n` + status.innerHTML;
  
  // Limitar líneas mostradas para evitar lag
  const lines = status.innerHTML.split('\n');
  if (lines.length > 50) {
    status.innerHTML = lines.slice(0, 50).join('\n');
  }
}

function getTarget() {
  const ip = document.getElementById('robot_ip').value.trim();
  const port = document.getElementById('robot_port').value.trim();
  return { ip: ip, port: port };
}

// === FUNCIONES DE COMUNICACIÓN ===
function testConnection() {
  appendStatus('Testing ESP32-CAM connection...', 'info');
  
  fetch('/cmd?cmd=PING')
    .then(response => {
      appendStatus(`HTTP Status: ${response.status} ${response.statusText}`, 
                   response.ok ? 'success' : 'error');
      return response.text();
    })
    .then(text => {
      appendStatus(`Response: ${text}`, 'success');
    })
    .catch(error => {
      appendStatus(`Connection Test FAILED: ${error.message}`, 'error');
    });
}

function sendCmd(cmd) {
  // Debug: log inmediato cuando se llama la función
  console.log(`🔧 sendCmd called with: ${cmd}`);
  appendStatus(`🔧 Function called: sendCmd('${cmd}')`, 'info');
  
  const tgt = getTarget();
  let url = '/cmd?cmd=' + encodeURIComponent(cmd);
  if (tgt.ip) url += '&ip=' + encodeURIComponent(tgt.ip);
  if (tgt.port) url += '&port=' + encodeURIComponent(tgt.port);
  
  console.log(`🌐 Sending request to: ${url}`);
  appendStatus(`🌐 Sending: ${cmd}`, 'info');
  
  fetch(url)
  .then(response => {
    console.log(`📡 Response received: ${response.status} ${response.statusText}`);
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    return response.text();
  })
  .then(text => {
    console.log(`✅ Response text: ${text}`);
    appendStatus(`✓ ${cmd} -> ${text}`, 'success');
  })
  .catch(error => {
    console.error(`❌ Command failed: ${error}`);
    appendStatus(`✗ ${cmd} -> ${error.message}`, 'error');
    
    // Auto-retry para comandos críticos
    if (cmd === 'STOP') {
      appendStatus('STOP failed - retrying...', 'warning');
      setTimeout(() => sendCmd('STOP'), 1000);
    }
  });
}

// === FUNCIONES DE CONTROL DE STREAM ===
function startStream() {
  img.src = '/stream?t=' + Date.now();
  appendStatus('Starting video stream...', 'info');
}

function restartStream() {
  appendStatus('Restarting video stream...', 'warning');
  img.src = '';
  setTimeout(startStream, 500);
}

// Event listeners para el stream
img.onload = function() { 
  appendStatus('Video stream connected', 'success'); 
};

img.onerror = function() { 
  appendStatus('Video stream failed - retrying in 3s...', 'error'); 
  setTimeout(startStream, 3000); 
};

// === CONTROL DE TECLADO ===
function setupKeyboardControl() {
  document.addEventListener('keydown', function(event) {
    // Solo activar si no estamos escribiendo en un input
    if (event.target.tagName === 'INPUT' || event.target.tagName === 'TEXTAREA') return;
    
    console.log(`⌨️ Key pressed: ${event.key}`);
    
    switch(event.key.toLowerCase()) {
      case 'w':
      case 'arrowup':
        sendCmd('ALL F 200');
        event.preventDefault();
        break;
      case 's':
      case 'arrowdown':
        sendCmd('ALL R 200');
        event.preventDefault();
        break;
      case 'a':
      case 'arrowleft':
        sendCmd('MOTOR 1 F 180');
        event.preventDefault();
        break;
      case 'd':
      case 'arrowright':
        sendCmd('MOTOR 2 F 180');
        event.preventDefault();
        break;
      case ' ':
      case 'escape':
        sendCmd('STOP');
        event.preventDefault();
        break;
      case 'p':
        testConnection();
        event.preventDefault();
        break;
      case 'r':
        restartStream();
        event.preventDefault();
        break;
    }
  });
  
  appendStatus('Keyboard controls enabled: WASD/Arrows=move, Space/Esc=stop, P=ping, R=restart', 'info');
}

// === FUNCIÓN DE INICIALIZACIÓN ===
function initializeApp() {
  appendStatus('🤖 ESP32 Robot Control System starting from SPIFFS...', 'info');
  
  // Test inicial de conexión y stream
  setTimeout(() => {
    testConnection();
    startStream();
    setupKeyboardControl();
  }, 500);
  
  // Test periódico de conectividad cada 10 segundos
  setInterval(() => {
    fetch('/cmd?cmd=PING', { timeout: 2000 })
      .then(r => r.ok ? null : appendStatus('ESP32-CAM not responding', 'warning'))
      .catch(() => appendStatus('ESP32-CAM disconnected', 'error'));
  }, 10000);
  
  // Heartbeat visual cada 30 segundos
  setInterval(() => {
    appendStatus('System running...', 'info');
  }, 30000);
  
  appendStatus('✅ System initialized successfully from SPIFFS!', 'success');
}

// === AUTO-INICIALIZACIÓN ===
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initializeApp);
} else {
  initializeApp();
}

// === EXPORTAR FUNCIONES GLOBALES ===
// Para compatibilidad con HTML inline onclick handlers
window.sendCmd = sendCmd;
window.testConnection = testConnection;
window.restartStream = restartStream;

// Debug global
window.robotDebug = {
  sendTest: () => sendCmd('PING'),
  sendStop: () => sendCmd('STOP'),
  sendForward: () => sendCmd('ALL F 100'),
  clearStatus: () => status.innerHTML = '',
  getStatus: () => status.textContent
};

console.log('✅ ESP32 Robot Control loaded successfully from SPIFFS!');
console.log('🔧 Debug commands available: robotDebug.sendTest(), robotDebug.sendStop(), etc.');
