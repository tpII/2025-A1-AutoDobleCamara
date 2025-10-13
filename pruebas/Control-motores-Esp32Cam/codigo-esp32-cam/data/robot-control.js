// ESP32 Robot Control System - JavaScript Module
console.log('🤖 ESP32 Robot Control Loading from SPIFFS...');

const img = document.getElementById('stream');
const status = document.getElementById('status');
const speedSlider = document.getElementById('speedSlider');
const currentSpeedSpan = document.getElementById('currentSpeed');

let currentSpeed = 200; // Velocidad global
let commandQueue = 0; // Contador de comandos pendientes

// === FUNCIONES DE ESTADO Y LOGGING ===
function appendStatus(s, type = 'info') { 
  if (!status) return; // Protección si el elemento no existe
  
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
  const ip = document.getElementById('robot_ip')?.value.trim() || '';
  const port = document.getElementById('robot_port')?.value.trim() || '';
  return { ip: ip, port: port };
}

// === FUNCIONES DE CONTROL DE STREAM ===
function startStream() {
  if (!img) return;
  img.src = '/stream?t=' + Date.now();
  appendStatus('Starting video stream...', 'info');
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

// Nueva función para actualizar velocidad
function updateSpeed(newSpeed) {
    currentSpeed = parseInt(newSpeed);
    currentSpeedSpan.textContent = newSpeed;
    appendStatus(`🎛️ Global speed set to: ${newSpeed}`, 'info');
}

// Función mejorada sendCmd con velocidad dinámica
function sendCmd(cmd) {
    console.log(`🔧 sendCmd called with: ${cmd}`);
    
    // Aplicar velocidad global a comandos ALL
    let finalCmd = cmd;
    const cmdParts = cmd.split(' ');
    if (cmdParts[0] === 'ALL' && cmdParts.length === 3) {
        finalCmd = `ALL ${cmdParts[1]} ${currentSpeed}`;
        appendStatus(`🎛️ Adjusted command: ${finalCmd}`, 'info');
    } else {
        appendStatus(`🔧 Function called: sendCmd('${cmd}')`, 'info');
    }
    
    const tgt = getTarget();
    let url = '/cmd?cmd=' + encodeURIComponent(finalCmd);
    if (tgt.ip) url += '&ip=' + encodeURIComponent(tgt.ip);
    if (tgt.port) url += '&port=' + encodeURIComponent(tgt.port);
    
    console.log(`🌐 Sending request to: ${url}`);
    appendStatus(`🌐 Sending: ${finalCmd}`, 'info');
    
    // Incrementar contador de comandos pendientes
    commandQueue++;
    updateCommandQueueStatus();
    
    // Añadir AbortController para timeout
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000); // 5s timeout
    
    fetch(url, {
        signal: controller.signal,
        method: 'GET',
        headers: {
            'Cache-Control': 'no-cache',
            'Connection': 'close'
        }
    })
    .then(response => {
        clearTimeout(timeoutId);
        commandQueue = Math.max(0, commandQueue - 1);
        updateCommandQueueStatus();
        
        console.log(`📡 Response received: ${response.status} ${response.statusText}`);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        return response.text();
    })
    .then(text => {
        console.log(`✅ Response text: ${text}`);
        appendStatus(`✓ ${finalCmd} -> ${text}`, 'success');
    })
    .catch(error => {
        clearTimeout(timeoutId);
        commandQueue = Math.max(0, commandQueue - 1);
        updateCommandQueueStatus();
        
        console.error(`❌ Command failed: ${error}`);
        
        if (error.name === 'AbortError') {
            appendStatus(`⏱️ ${finalCmd} -> TIMEOUT (5s)`, 'error');
            appendStatus('💡 Try restarting stream if commands get stuck', 'warning');
        } else {
            appendStatus(`✗ ${finalCmd} -> ${error.message}`, 'error');
        }
        
        // Auto-retry para comandos críticos
        if (cmd === 'STOP') {
            appendStatus('STOP failed - retrying...', 'warning');
            setTimeout(() => sendCmd('STOP'), 1000);
        }
    });
}

// Nueva función para mostrar estado de la cola de comandos
function updateCommandQueueStatus() {
    if (commandQueue > 0) {
        appendStatus(`⏳ Commands in queue: ${commandQueue}`, 'warning');
    }
    
    // Si hay muchos comandos encolados, sugerir reiniciar stream
    if (commandQueue > 3) {
        appendStatus('⚠️ Too many pending commands - consider restarting stream', 'error');
    }
}

// Función mejorada para reiniciar stream
function restartStream() {
  appendStatus('🔄 Restarting video stream...', 'warning');
  appendStatus(`📊 Clearing ${commandQueue} pending commands`, 'info');
  
  // Reset contador de comandos
  commandQueue = 0;
  updateCommandQueueStatus();
  
  img.src = '';
  
  // Delay más largo para asegurar que la conexión se cierre
  setTimeout(() => {
    startStream();
    appendStatus('✅ Stream restarted - commands should work now', 'success');
  }, 1000); // Incrementado de 500ms a 1000ms
}

// Event listeners mejorados para el stream
img.onload = function() { 
  appendStatus('📹 Video stream connected', 'success'); 
};

img.onerror = function() { 
  appendStatus('📹 Video stream failed - retrying in 3s...', 'error'); 
  setTimeout(() => {
    startStream();
  }, 3000); 
};

// Detectar si el stream se desconecta
let streamCheckInterval = setInterval(() => {
  if (img.complete && img.naturalHeight === 0) {
    appendStatus('📹 Stream disconnected - auto-restarting...', 'warning');
    restartStream();
  }
}, 10000); // Check cada 10 segundos

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
    appendStatus('🤖 ESP32 Robot Control System ready', 'success');
    appendStatus('🎛️ Speed control enabled', 'info');
    appendStatus('📊 Command queue monitoring active', 'info');
    updateSpeed(speedSlider.value);
    testConnection();
    startStream();
    setupKeyboardControl();
  }, 500);
  
  // Test periódico de conectividad cada 15 segundos (reducido de 10s)
  setInterval(() => {
    // Solo hacer ping si no hay comandos pendientes
    if (commandQueue === 0) {
      fetch('/cmd?cmd=PING', { 
        signal: AbortSignal.timeout(2000),
        headers: { 'Cache-Control': 'no-cache' }
      })
      .then(r => r.ok ? null : appendStatus('ESP32-CAM not responding', 'warning'))
      .catch(() => appendStatus('ESP32-CAM disconnected', 'error'));
    }
  }, 15000);
  
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
window.updateSpeed = updateSpeed;
window.startStream = startStream;

// Debug global
window.robotDebug = {
  sendTest: () => sendCmd('PING'),
  sendStop: () => sendCmd('STOP'),
  sendForward: () => sendCmd('ALL F 100'),
  clearStatus: () => status.innerHTML = '',
  getStatus: () => status.textContent
};

console.log('✅ ESP32 Robot Control loaded successfully from SPIFFS!');
console.log('🔄 Overriding inline functions with full functionality');
