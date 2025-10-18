# 📡 Instrucciones de Configuración WiFi - Modo Station

## 🔧 Paso 1: Configurar tu Red WiFi

Abre el archivo `src/main.cpp` y modifica estas líneas (están al inicio del archivo):

```cpp
// Configuración WiFi - CAMBIA ESTOS VALORES POR TU RED
const char* ssid = "TU_RED_WIFI";           // ← Pon el nombre de tu WiFi aquí
const char* password = "TU_PASSWORD_WIFI";  // ← Pon tu contraseña aquí
```

**Ejemplo:**

```cpp
const char* ssid = "WiFi_Casa_Lopez";
const char* password = "mipassword2024";
```

### ⚠️ Importante:

- El **ESP32 solo funciona con WiFi de 2.4GHz** (no funciona con 5GHz)
- Si tu router tiene ambas bandas (2.4GHz y 5GHz), asegúrate de usar la red de 2.4GHz
- La contraseña distingue entre MAYÚSCULAS y minúsculas

---

## 🚀 Paso 2: Compilar y Cargar el Código

### Opción A: Desde PlatformIO (VSCode)

1. Conecta el ESP32 por USB
2. Presiona `Ctrl+Alt+U` (o usa el botón "Upload" en la barra inferior)
3. Espera a que termine la carga

### Opción B: Desde Terminal

```bash
cd /ruta/a/prueba-en-esp32
pio run -t upload
```

---

## 📟 Paso 3: Monitorear la Conexión

Abre el **Monitor Serial** para ver el estado de la conexión:

```bash
pio device monitor
```

O presiona el botón "Monitor" en PlatformIO.

**Deberías ver algo como:**

```
=================================
ESP32 Camera - Detección de Objetos
=================================

Conectando a WiFi: WiFi_Casa_Lopez
........
✅ ¡Conectado a WiFi exitosamente!

--- Información de Conexión ---
SSID: WiFi_Casa_Lopez
Intensidad de señal (RSSI): -45 dBm
IP del ESP32: 192.168.1.156
Gateway: 192.168.1.1
MAC Address: 24:6F:28:XX:XX:XX
-------------------------------

Cámara inicializada correctamente!

🚀 Servidor web iniciado en puerto 80

╔════════════════════════════════════════╗
║  Accede desde tu navegador a:          ║
║  http://192.168.1.156                  ║
╚════════════════════════════════════════╝

Endpoints disponibles:
  • Interfaz web: http://192.168.1.156
  • Detección JSON: http://192.168.1.156/detect
  • Stream video: http://192.168.1.156/stream
```

### 🔍 Anota la IP del ESP32

La línea importante es:

```
IP del ESP32: 192.168.1.156  ← Esta es TU IP (será diferente)
```

---

## 🌐 Paso 4: Acceder desde tu Navegador

### Desde cualquier dispositivo conectado a LA MISMA RED WiFi:

1. **Celular**: Asegúrate de estar conectado a la misma red WiFi
2. **Computadora**: Asegúrate de estar conectado a la misma red WiFi
3. Abre tu navegador (Chrome, Firefox, Safari, etc.)
4. Escribe la IP del ESP32: `http://192.168.1.156` (usa la IP que salió en TU monitor serial)

### Endpoints disponibles:

- **`http://IP_DEL_ESP32/`** → Interfaz web principal con medición de distancia
- **`http://IP_DEL_ESP32/detect`** → JSON con datos de detección
- **`http://IP_DEL_ESP32/stream`** → Stream de video con bounding boxes

---

## ❌ Solución de Problemas

### Problema 1: "No se pudo conectar a WiFi"

**Verifica:**

1. ✅ SSID y contraseña correctos (respeta mayúsculas/minúsculas)
2. ✅ El router esté encendido
3. ✅ El ESP32 esté dentro del alcance del WiFi
4. ✅ Estás usando WiFi de **2.4GHz** (no 5GHz)

**Solución:**

- Revisa el SSID: algunos routers tienen nombres como "MiWiFi_2.4G" y "MiWiFi_5G"
- Usa la red que termina en "\_2.4G" o similar

---

### Problema 2: "Se conecta pero no puedo acceder desde el navegador"

**Verifica:**

1. ✅ Tu celular/PC está conectado a LA MISMA red WiFi
2. ✅ Estás usando la IP correcta (la que salió en el Monitor Serial)
3. ✅ Estás usando `http://` (no `https://`)
4. ✅ No tienes VPN activa

**Ejemplo de IP incorrecta:**

- ❌ `http://192.168.4.1` (esta era la IP del modo AP)
- ✅ `http://192.168.1.156` (esta es la IP en tu red)

---

### Problema 3: "Se desconecta constantemente"

El código tiene reconexión automática. Si se desconecta:

1. Verifica la intensidad de señal en el Monitor Serial:

   ```
   Intensidad de señal (RSSI): -45 dBm  ← Buena señal
   ```

2. Valores de referencia:
   - `-30 a -50 dBm` → Excelente
   - `-50 a -60 dBm` → Buena
   - `-60 a -70 dBm` → Regular (puede desconectarse)
   - `-70 a -90 dBm` → Mala (se desconectará)

**Solución:** Acerca el ESP32 al router o usa un router con mejor señal.

---

### Problema 4: "¿Cuál es mi IP si cambia cada vez que reinicio?"

La IP puede cambiar si tu router usa DHCP dinámico.

**Opciones:**

#### A) Ver la IP en el Monitor Serial cada vez

- Es la forma más simple
- Solo abre el Monitor Serial y anota la IP

#### B) Configurar IP estática (avanzado)

Agrega esto en el `setup()`, después de `WiFi.begin()`:

```cpp
WiFi.begin(ssid, password);

// Esperar conexión...

// Configurar IP estática (OPCIONAL)
IPAddress local_IP(192, 168, 1, 100);  // Tu IP deseada
IPAddress gateway(192, 168, 1, 1);     // Tu gateway (router)
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);

WiFi.config(local_IP, gateway, subnet, primaryDNS);
```

Ahora tu ESP32 siempre tendrá la IP `192.168.1.100`

---

## 💡 Ventajas del Modo Station vs Modo AP

### ✅ Modo Station (Actual)

- ✅ No hay problemas de "Sin Internet" en el celular
- ✅ Mejor alcance (usa tu router WiFi)
- ✅ Múltiples dispositivos pueden acceder simultáneamente
- ✅ Compatible con iOS sin problemas
- ❌ Necesitas tener un router WiFi disponible

### ❌ Modo AP (Anterior)

- ✅ No necesitas router WiFi
- ✅ Funciona en cualquier lugar
- ❌ Celulares modernos se desconectan (sin Internet)
- ❌ Alcance limitado del ESP32
- ❌ Problemas con iOS

---

## 🎯 Resumen Rápido

1. **Edita** `src/main.cpp` → Cambia SSID y contraseña
2. **Compila y carga** el código al ESP32
3. **Abre** Monitor Serial → Anota la IP
4. **Accede** desde tu navegador: `http://IP_DEL_ESP32`

---

## 📞 ¿Necesitas Ayuda?

Si sigues teniendo problemas:

1. Copia TODO el texto del Monitor Serial
2. Verifica que estés en la misma red WiFi
3. Prueba hacer ping a la IP del ESP32:
   ```bash
   ping 192.168.1.156  # Usa tu IP
   ```

¡Listo! Ahora deberías poder acceder sin problemas desde cualquier dispositivo en tu red.
