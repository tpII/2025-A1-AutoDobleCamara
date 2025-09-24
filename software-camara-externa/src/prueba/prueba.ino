#include <WiFi.h>
#include <WebServer.h>

// Credenciales del Access Point
const char* ssid = "CamaraAP";
const char* password = "12345678";

// IP estática del AP
IPAddress local_ip(192,168,5,1);
IPAddress gateway(192,168,5,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

// Prototipos
void handle_OnConnect();
void handle_NotFound();

void setup() {
  Serial.begin(115200);
  delay(100);

  // Forzar modo AP (opcional pero explícito)
  WiFi.mode(WIFI_AP);

  // Configurar IP estática para la interfaz softAP
  if(!WiFi.softAPConfig(local_ip, gateway, subnet)) {
    Serial.println("softAPConfig failed");
  }

  // Iniciar Access Point
  bool apStarted = WiFi.softAP(ssid, password);
  if(apStarted) {
    Serial.print("Access Point started. SSID: ");
    Serial.println(ssid);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP");
  }

  // Rutas HTTP
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

// Handler para la raíz
void handle_OnConnect() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Camara AP</title></head><body>";
  html += "<h1>CamaraAP</h1>";
  html += "<p>Conexión exitosa al Access Point.</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handler para rutas no encontradas
void handle_NotFound() {
  String message = "Not Found\n";
  message += "URI: ";
  message += server.uri();
  server.send(404, "text/plain", message);
}
