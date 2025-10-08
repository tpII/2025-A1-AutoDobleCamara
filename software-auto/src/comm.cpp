#include <comm.h>
#include "config.h" // <-- usar parámetros centralizados


// declara la función definida en console/main para reenviar líneas
extern void processLine(String line);

static unsigned long lastConnectAttempt = 0;

static WiFiClient client;
static String netBuf;

static String serverIpStr;
static uint16_t serverPort;

void commNetBegin(const char* ssid, const char* pass, const char* serverIp, uint16_t port)
{
    serverIpStr = serverIp;
    serverPort = port;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    Serial.printf("WiFi: connecting to %s …\n", ssid);
}

static bool ensureConnected()
{
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastConnectAttempt >= CONNECT_RETRY_MS) {
            lastConnectAttempt = now;
            Serial.println("WiFi: waiting connection...");
        }
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