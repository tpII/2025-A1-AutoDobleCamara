# 🔧 Solución de Problemas WiFi - ESP32 AP

## ❌ Problema: No puedo conectarme al WiFi "ESP32_AP"

### ✅ Soluciones Paso a Paso

#### 1. **Verifica que el ESP32 esté ejecutándose**

Abre el **Monitor Serial** (PlatformIO: Serial Monitor) y busca:

```
✅ WiFi AP iniciado correctamente
SSID: ESP32_AP
Password: 12345678
IP del ESP32: 192.168.4.1
```

Si ves esto, el AP está funcionando correctamente.

---

#### 2. **Problema: El celular no encuentra la red "ESP32_AP"**

**Causas posibles:**

- El ESP32 no se inició correctamente
- Interferencia de otras redes WiFi
- El celular no soporta el canal WiFi usado

**Soluciones:**

a) **Reinicia el ESP32** (botón EN/RST)

b) **Cambia el canal WiFi** en `main.cpp`:

```cpp
// Cambiar el "1" por otro canal (1-13)
WiFi.softAP(ssid, password, 6, false, 4);  // Probar canal 6 u 11
```

c) **Aleja el ESP32 de otras redes WiFi** (routers cercanos)

---

#### 3. **Problema: Encuentro la red pero dice "Contraseña incorrecta"**

**Causas posibles:**

- La contraseña debe tener mínimo 8 caracteres
- Algunos celulares tienen problemas con caracteres especiales
- Error de tipeo

**Soluciones:**

a) **Verifica la contraseña exacta** en el Monitor Serial

b) **Usa una contraseña más larga** (cambia en `main.cpp`):

```cpp
const char* password = "esp32-2024";  // O cualquier otra de 8+ caracteres
```

c) **Evita caracteres especiales**, usa solo letras y números

---

#### 4. **Problema: Se conecta pero dice "Sin Internet" y se desconecta**

**Causa:** Android/iOS moderno desconecta automáticamente de redes sin Internet.

**Solución:** Deshabilita "Cambio automático de red" en tu celular:

**Android:**

1. Configuración → WiFi → ESP32_AP → Avanzado
2. Desactiva "Cambiar a datos móviles automáticamente"
3. O desactiva "Administración de red inteligente"

**iPhone/iOS:**

1. No hay solución directa
2. iOS desconectará cada ~30 segundos
3. **Alternativa:** Usa la IP antes de que se desconecte
4. O desactiva los Datos Móviles mientras usas el ESP32

---

#### 5. **Problema: Se conecta pero no puedo abrir http://192.168.4.1**

**Soluciones:**

a) **Desactiva VPN** si tienes una activa en el celular

b) **Desactiva Datos Móviles** temporalmente

c) **Usa la IP completa:** `http://192.168.4.1` (no olvides el `http://`)

d) **Verifica la IP del ESP32** en el Monitor Serial:

```
IP del ESP32: 192.168.4.1  ← Usa esta IP
```

e) **Prueba desde el navegador del celular** (no desde apps de terceros)

---

#### 6. **Configuración Alternativa: WiFi Station (conectarse a tu router)**

Si tienes muchos problemas con el modo AP, cambia el ESP32 para que se conecte a TU red WiFi:

**Modificación en `main.cpp`:**

```cpp
// Configuración WiFi Station (conectar a tu router)
const char* ssid = "TU_RED_WIFI";       // Nombre de tu WiFi
const char* password = "TU_PASSWORD";   // Contraseña de tu WiFi

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 conectándose a WiFi...");

  // Conectar a red WiFi existente
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Esperar conexión
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado a WiFi!");
    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());  // ← ANOTA ESTA IP
    Serial.println("Abre esta IP desde tu celular/PC en la misma red");
  } else {
    Serial.println("\n❌ No se pudo conectar a WiFi");
  }

  setupCamera();

  server.on("/", handleRoot);
  server.on("/detect", handleDetect);
  server.on("/stream", handleStream);
  server.begin();
  Serial.println("Servidor iniciado");
}
```

**Ventajas de Station Mode:**

- ✅ No hay problemas de "Sin Internet"
- ✅ Funciona mejor en iOS
- ✅ Puedes usar desde múltiples dispositivos
- ✅ Alcance mayor (usa tu router WiFi)

**Desventajas:**

- ❌ Requiere que tengas un router WiFi disponible
- ❌ El ESP32 y tu celular deben estar en la misma red

---

## 🔍 Diagnóstico Avanzado

### Verificar estado del AP desde Monitor Serial

Agrega al `loop()`:

```cpp
void loop() {
  server.handleClient();

  // Debug cada 10 segundos
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 10000) {
    Serial.printf("Clientes WiFi conectados: %d\n", WiFi.softAPgetStationNum());
    lastCheck = millis();
  }

  delay(10);
}
```

Esto te dirá si el celular realmente se está conectando.

---

## 📱 Tips para cada Sistema Operativo

### **Android 9+**

- Problema conocido: Android desconecta automáticamente
- Solución: Desactiva "Cambiar a datos móviles" en configuración de WiFi

### **iOS 13+**

- Problema: iOS es muy agresivo desconectando redes sin Internet
- Solución: Desactiva Datos Móviles o usa modo Station (conectar ESP32 a tu router)

### **Windows 10/11**

- Generalmente funciona sin problemas
- Si no funciona: Deshabilita "Conexión de uso medido" en la red ESP32_AP

---

## 🚀 Configuración Recomendada Final

```cpp
// En main.cpp, función setup():

// OPCIÓN A: Modo AP (ESP32 crea su propia red)
WiFi.mode(WIFI_AP);
WiFi.softAP("ESP32_CAM", "12345678", 6, false, 4);
// Conéctate desde celular a "ESP32_CAM" → http://192.168.4.1

// OPCIÓN B: Modo Station (ESP32 se conecta a tu router)
WiFi.mode(WIFI_STA);
WiFi.begin("TU_WIFI", "TU_PASSWORD");
// Mira la IP en Serial Monitor → http://IP_DEL_ESP32
```

**Mi recomendación:**

- Si tienes iOS: **Usa modo Station (Opción B)**
- Si tienes Android: **Ambos modos funcionan**, pero Station es más estable
- Si no tienes WiFi disponible: **Usa modo AP y desactiva Datos Móviles**

---

## ✅ Checklist de Verificación

Antes de pedir ayuda, verifica:

- [ ] El ESP32 está encendido y el LED está parpadeando
- [ ] El Monitor Serial muestra "WiFi AP iniciado correctamente"
- [ ] El SSID "ESP32_AP" aparece en la lista de redes WiFi del celular
- [ ] La contraseña es exactamente "12345678"
- [ ] Los Datos Móviles están desactivados en el celular
- [ ] No hay VPN activa
- [ ] Estás usando `http://` (no `https://`)
- [ ] La IP es la correcta (192.168.4.1 por defecto)

---

¿Sigue sin funcionar? Prueba la **Opción B (Station Mode)** que es más compatible con celulares modernos.
