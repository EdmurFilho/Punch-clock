#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- Configure aqui ---
const char* ssid = "SEU_SSID";         // Nome da sua rede Wi-Fi
const char* password = "SUA_SENHA";    // Senha da sua rede Wi-Fi
String scriptURL = "SUA_URL_DO_APPS_SCRIPT"; // URL que você copiou do Apps Script
// ----------------------

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Conectar ao Wi-Fi
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  
}

void enviarDados(String valor1, String valor2) {
  if (WiFi.status() == WL_CONNECTED) {
   
    HTTPClient http;
    String urlCompleta = scriptURL;
    urlCompleta += "?valor1=" + valor1;
    urlCompleta += "&valor2=" + valor2;

    Serial.print("Enviando dados para: ");
    Serial.println(urlCompleta);

    http.begin(urlCompleta);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); 

 
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("[HTTP] Código da resposta: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println("[HTTP] Resposta do servidor:");
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] Falha no GET, erro: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Erro: WiFi não conectado");
  }
}