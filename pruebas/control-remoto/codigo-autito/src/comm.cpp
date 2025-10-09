#include <comm.h>
#include "config.h" // <-- usar parámetros centralizados


// declara la función definida en console/main para reenviar líneas
extern void processLine(String line);

static unsigned long lastConnectAttempt = 0;

static WiFiClient client;
static String netBuf;
static bool wifiConnected = false;
static unsigned long wifiConnectedTime = 0;

static String serverIpStr;
static uint16_t serverPort;

void commNetBegin(const char* ssid, const char* pass, const char* serverIp, uint16_t port)
{
    serverIpStr = serverIp;
    serverPort = port;
    WiFi.mode(WIFI_STA);
    
    // Configurar IP estática si está definida
    if (strlen(ESP_STATIC_IP) > 0) {
        IPAddress local_IP;
        IPAddress gateway;
        IPAddress subnet;
        
        if (local_IP.fromString(ESP_STATIC_IP) && 
            gateway.fromString(ESP_GATEWAY) && 
            subnet.fromString(ESP_SUBNET)) {
            
            Serial.printf("🔧 Configurando IP estática: %s\n", ESP_STATIC_IP);
            Serial.printf("🔧 Gateway: %s, Subnet: %s\n", ESP_GATEWAY, ESP_SUBNET);
            
            if (!WiFi.config(local_IP, gateway, subnet)) {
                Serial.println("❌ Error configurando IP estática - usando DHCP");
            } else {
                Serial.println("✅ IP estática configurada");
            }
        } else {
            Serial.println("❌ Error parseando IPs estáticas - usando DHCP");
        }
    } else {
        Serial.println("📡 Usando DHCP para obtener IP");
    }
    
    WiFi.begin(ssid, pass);
    Serial.printf("WiFi: connecting to %s …\n", ssid);
}

static bool ensureConnected()
{
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnected = false;
        unsigned long now = millis();
        if (now - lastConnectAttempt >= CONNECT_RETRY_MS) {
            lastConnectAttempt = now;
            Serial.println("WiFi: waiting connection...");
        }
        return false;
    }

    // Marcar cuando WiFi se conecta por primera vez
    if (!wifiConnected) {
        wifiConnected = true;
        wifiConnectedTime = millis();
        Serial.println("🌐 WiFi conectado! Esperando estabilización antes de TCP...");
    }

    // Esperar 3 segundos después de conectar WiFi antes de intentar TCP
    if (millis() - wifiConnectedTime < 3000) {
        return false;
    }

    if (!client.connected()) {
        unsigned long now = millis();
        if (now - lastConnectAttempt >= CONNECT_RETRY_MS) {
            lastConnectAttempt = now;
            Serial.printf("TCP: connecting to %s:%u …\n", serverIpStr.c_str(), serverPort);
            client.stop();
            client.connect(serverIpStr.c_str(), serverPort);
            if (client.connected()) Serial.println("TCP: connected");
            else Serial.println("TCP: connect failed");
        }
        return false;
    }
    return true;
}

void commNetLoop()
{
    if (!ensureConnected()) return;

    while (client.available()) {
        char c = client.read();
        // manejar CR/LF/backspace similar a Serial
        if (c == '\r') continue;
        if (c == '\n') {
            Serial.println(); // mostrar en consola local
            processLine(netBuf);
            netBuf = "";
            continue;
        }
        if (c == 8 || c == 127) {
            if (netBuf.length() > 0) {
                netBuf.remove(netBuf.length() - 1, 1);
                Serial.print("\b \b");
            }
            continue;
        }
        if (c >= 32 && c <= 126) {
            netBuf += c;
            Serial.print(c);
            if (netBuf.length() > NETBUF_MAXLEN) netBuf = netBuf.substring(netBuf.length() - NETBUF_MAXLEN);
        }
    }
}