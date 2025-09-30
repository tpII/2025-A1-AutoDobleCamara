#include <Arduino.h>
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
  delay(1000);
  
  Serial.println("Iniciando ESP32-CAM Access Point...");
  
  // Desconectar de cualquier red WiFi existente
  WiFi.disconnect(true);
  delay(1000);
  
  // Configurar modo AP
  WiFi.mode(WIFI_AP);
  delay(1000);
  
  // Configurar IP estática para la interfaz softAP
  if(!WiFi.softAPConfig(local_ip, gateway, subnet)) {
    Serial.println("ERROR: softAPConfig failed");
    return;
  }
  
  // Iniciar Access Point con configuraciones específicas
  bool apStarted = WiFi.softAP(ssid, password, 1, 0, 4); // Canal 1, no oculto, max 4 conexiones
  
  if(apStarted) {
    Serial.println("=== ACCESS POINT INICIADO EXITOSAMENTE ===");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.print("IP del AP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.println("==========================================");
  } else {
    Serial.println("ERROR: Failed to start Access Point");
    return;
  }
  
  // Esperar un poco para que el AP se estabilice
  delay(2000);
  
  // Configurar rutas HTTP
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  
  // Iniciar servidor web
  server.begin();
  Serial.println("Servidor HTTP iniciado en puerto 80");
  Serial.println("Conecta tu dispositivo a la red 'CamaraAP' y ve a http://192.168.5.1");
}

void loop() {
  server.handleClient();
  
  // Mostrar información cada 10 segundos
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    Serial.print("Clientes conectados: ");
    Serial.println(WiFi.softAPgetStationNum());
    lastPrint = millis();
  }
}

// Handler para la raíz
void handle_OnConnect() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Camara AP</title></head><body>";
  html += "<h1>ESP32-CAM Access Point</h1>";
  html += "<p>Conexión exitosa al Access Point.</p>";
  html += "<p>IP del servidor: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p>Clientes conectados: " + String(WiFi.softAPgetStationNum()) + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
  Serial.println("Cliente accedió a la página principal");
}

// Handler para rutas no encontradas
void handle_NotFound() {
  String message = "Página no encontrada\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(404, "text/plain", message);
  Serial.println("404 - Página no encontrada: " + server.uri());
}