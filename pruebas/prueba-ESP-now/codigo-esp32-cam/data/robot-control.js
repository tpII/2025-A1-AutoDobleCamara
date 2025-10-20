document.addEventListener('DOMContentLoaded', function() {
    const speedSlider = document.getElementById('speed');
    const speedValue = document.getElementById('speed-value');
    const cameraFeed = document.getElementById('camera-feed');
    let currentSpeed = speedSlider.value;

    // Actualizar valor de velocidad
    speedSlider.addEventListener('input', function() {
        currentSpeed = this.value;
        speedValue.textContent = currentSpeed;
    });

    // Función para enviar comandos
    async function sendCommand(cmd) {
        try {
            const response = await fetch('/control', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `cmd=${encodeURIComponent(cmd)}`
            });
            if (!response.ok) {
                console.error('Error al enviar comando');
            }
        } catch (error) {
            console.error('Error:', error);
        }
    }

    // Configuración de los botones
    document.getElementById('forward').addEventListener('click', () => {
        sendCommand(`ALL F ${currentSpeed}`);
    });

    document.getElementById('backward').addEventListener('click', () => {
        sendCommand(`ALL R ${currentSpeed}`);
    });

    document.getElementById('left').addEventListener('click', () => {
        sendCommand(`MOTOR 1 R ${currentSpeed}`);
        sendCommand(`MOTOR 2 F ${currentSpeed}`);
    });

    document.getElementById('right').addEventListener('click', () => {
        sendCommand(`MOTOR 1 F ${currentSpeed}`);
        sendCommand(`MOTOR 2 R ${currentSpeed}`);
    });

    document.getElementById('stop').addEventListener('click', () => {
        sendCommand('STOP');
    });

    // Actualizar la imagen de la cámara cada 100ms
    setInterval(() => {
        cameraFeed.src = `/stream?t=${Date.now()}`;
    }, 100);

    // Soporte para teclas
    document.addEventListener('keydown', function(event) {
        switch(event.key) {
            case 'ArrowUp':
                document.getElementById('forward').click();
                break;
            case 'ArrowDown':
                document.getElementById('backward').click();
                break;
            case 'ArrowLeft':
                document.getElementById('left').click();
                break;
            case 'ArrowRight':
                document.getElementById('right').click();
                break;
            case ' ': // Spacebar
                document.getElementById('stop').click();
                break;
        }
    });
});