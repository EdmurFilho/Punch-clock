#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

const char* WIFI_SSID  = "Paradiso";
const char* WIFI_PASSWORD = "8167350Rm";
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbzKDTB0AF1i-2s98-PGGb_tyA1vLXqO0yyGhnQ0Ojj8XqDpp5cZslnmx3wD8fYhhGX6Uw/exec";

LiquidCrystal_I2C lcd(0x27,16,2);

const byte UID_SIZE = 4;

struct Registros {
    const char* uid_str; 
    const char* nome;  
    bool estado;
};

Registros LISTA_PESSOAS[] = {
    {"D3 EB 1E F4", "Edmur", 0},  
};

const int NUM_PESSOAS = sizeof(LISTA_PESSOAS) / sizeof(LISTA_PESSOAS[0]);
const char* pessoa;
const char* acaoS;
#define SS_PIN  5     // SDA no módulo RC522
#define RST_PIN 4    // RST no módulo RC522

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Cria o objeto do leitor

bool UIDvalido = true;
int indice;

String formatarUID(byte *buffer, byte bufferSize);
const char* buscarNomePorUID(String uid_procurado);
String seguirRedirecionamento(String url);
bool escreverEmLista(String identificacao, int numDados, const char* palavras[]);
void montarCabecalho(String _boardID, const String& colunaInicial, const std::vector<String>& cabecalhos);
bool escreverEmCelula(String identificacao, String celula, String dado);
String lerCelula(String identificacao, String celula);
void montarCabecalho(String _boardID, const String& colunaInicial, const std::vector<String>& cabecalhos);
void playConfirmBeep();
void playRejectBeep();

#define buzzer 32
#define Red 13
#define Green 12

bool comum = 0;

void setup() {
  lcd.init();                      
  lcd.backlight();
  
  pinMode(buzzer, OUTPUT);

  Serial.begin(115200);
  SPI.begin(18, 19, 23, SS_PIN);   // SCK=18, MISO=19, MOSI=23, SS=5
  mfrc522.PCD_Init();              // Inicializa o RC522

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a ");
  Serial.println(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) { // Tenta por 20 segundos
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac: ");
  Serial.println(WiFi.macAddress());

  Serial.println("Aproxime o cartão mfrc522 do leitor...");
  montarCabecalho("ESP32", "A", {"Data completa", "Data", "Hora", "Pessoa", "Entrou/Saiu"});
  }

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid_lido = formatarUID(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print("TAG UID: ");
    Serial.print(uid_lido);
    pessoa = buscarNomePorUID(uid_lido);

    if(UIDvalido){
      bool acao = LISTA_PESSOAS[indice].estado;
      acaoS = !acao ? "Entrou" : "Saiu";
      digitalWrite(Green, comum);
      lcd.clear();
      lcd.setCursor(0, 1);
      Serial.println(pessoa);
      lcd.print(pessoa);
      lcd.setCursor(10,1);
      Serial.println(acaoS);
      lcd.print(acaoS);
      LISTA_PESSOAS[indice].estado = !LISTA_PESSOAS[indice].estado;
      playConfirmBeep();
      delay(100);
      digitalWrite(Green, !comum);
      const char* DadosParaEnviar[] = {pessoa, acaoS};
      escreverEmLista("ESP32", 2, DadosParaEnviar);
      lcd.clear();
    }else{
      digitalWrite(Red, comum);
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("TAG INVALIDO!");
      playRejectBeep();
      delay(100);
      digitalWrite(Red, !comum);
      lcd.clear();
    }
  }
}

String formatarUID(byte *buffer, byte bufferSize) {
    String str_uid = "";
    for (byte i = 0; i < bufferSize; i++) {
        // Converte o byte para hexadecimal, adiciona '0' se for menor que 16 (0x10)
        if (buffer[i] < 0x10) {
            str_uid += "0";
        }
        str_uid += String(buffer[i], HEX);
        if (i < bufferSize - 1) {
            str_uid += " "; // Adiciona espaço entre os bytes
        }
    }
    str_uid.toUpperCase(); // Garante que a String seja "D3 EB 1E F4" e não "d3 eb 1e f4"
    return str_uid;
}

const char* buscarNomePorUID(String uid_procurado) {
    // Itera sobre o array constante
    for (int i = 0; i < NUM_PESSOAS; i++) {
        // Compara a String do UID lido com a String do UID cadastrado
        if (uid_procurado.equals(LISTA_PESSOAS[i].uid_str)) {
            UIDvalido = true;
            indice = i;
            return LISTA_PESSOAS[i].nome; // Encontrou! Retorna o nome
        }
    }
    Serial.print("UID DESCONHECIDO: ");
    Serial.println(uid_procurado);
    UIDvalido = false;
    return nullptr; // UID não encontrado
} 

String seguirRedirecionamento(String url){
    // Cria instâncias locais
    WiFiClientSecure client;
    HTTPClient http;
    
    client.setInsecure();
    
    // Inicia a comunicação usando o cliente seguro
    http.begin(client, url); 
    
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
        String newUrl = http.getLocation();
        http.end();
        return newUrl; 
    } else {
        String response = http.getString();
        http.end();
        return response;
    }
}

bool escreverEmLista(String identificacao, int numDados, const char* palavras[]){
    bool flag_envio = false;
    String url = googleScriptURL;
    
    // Cria instâncias locais
    WiFiClientSecure client; 
    HTTPClient http;
    
    // Usando StaticJsonDocument para evitar warnings
    StaticJsonDocument<256> jsonDoc; 
    
    jsonDoc["action"] = "escreverEmLista";
    jsonDoc["identificacao"] = identificacao;
    
    // Usando a sintaxe moderna para evitar warnings
    JsonArray jsonDados = jsonDoc["dados"].to<JsonArray>();
    
    for (int i = 0; i < numDados; i++) {
        jsonDados.add(palavras[i]);
    }

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    
    Serial.print("JSON Enviado (Apenas Palavras): ");
    Serial.println(jsonString);

    // SOLUÇÃO: Ignora a verificação de certificado SSL
    client.setInsecure();
    
    // Inicia a comunicação usando o cliente seguro
    http.begin(client, url); 
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.print("Sucesso, Resposta do Script: ");
        Serial.println(response);
        flag_envio = true;
    } else if (httpResponseCode > 0) {
        Serial.printf("ERRO HTTP: Código %d\n", httpResponseCode);
        Serial.println(http.getString());
    } else {
        Serial.printf("ERRO CONEXÃO: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    return flag_envio;
}

bool escreverEmCelula(String identificacao, String celula, String dado){
    bool flag_envio = 0;
    String url = googleScriptURL;
    
    // Cria instâncias locais
    WiFiClientSecure client;
    HTTPClient http;

    // Usando StaticJsonDocument
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["action"] = "escreverEmCelula";
    jsonDoc["identificacao"] = identificacao;
    jsonDoc["celula"] = celula;
    jsonDoc["dado"] = dado;

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    
    // SOLUÇÃO: Ignora a verificação de certificado SSL
    client.setInsecure();
    
    // Inicia a comunicação usando o cliente seguro
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("Dados enviados (Resposta: " + response + ")");
        flag_envio = 1;
    } else if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("ERRO HTTP: Código %d, Resposta: %s\n", httpResponseCode, response.c_str());
    } else {
        Serial.printf("Erro ao enviar dados: %s\n", http.errorToString(httpResponseCode).c_str());
        flag_envio = 0;
    }
    http.end();
    return flag_envio;
}

String lerCelula(String identificacao, String celula){
    String url = googleScriptURL;
    
    // Cria instâncias locais
    WiFiClientSecure client;
    HTTPClient http;
    
    // Usando StaticJsonDocument
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["action"] = "lerCelula";
    jsonDoc["identificacao"] = identificacao;
    jsonDoc["celula"] = celula;

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    
    // SOLUÇÃO: Ignora a verificação de certificado SSL
    client.setInsecure();
    
    // Inicia a comunicação usando o cliente seguro
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode == 200) {
        String response = http.getString();
        http.end();
        return response;
    } else if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("Erro ao ler célula: Código %d\n", httpResponseCode);
        http.end();
        return "Erro ao ler célula";
    } else {
        Serial.printf("Erro de conexão ao ler célula: %s\n", http.errorToString(httpResponseCode).c_str());
        http.end();
        return "Erro ao ler célula";
    }
}

void montarCabecalho(String _boardID, const String& colunaInicial, const std::vector<String>& cabecalhos) {
  // Verifica se a primeira célula já contém o primeiro cabeçalho esperado
  String celula = lerCelula(_boardID, colunaInicial + "1");

  if (celula != cabecalhos[0] && celula != "Erro ao ler célula") {
    Serial.print("Carregando cabeçalho...");
    Serial.println("");
    Serial.print("String recebida: ");
    Serial.println(celula);
    Serial.println("");

    // Percorre todas as colunas e escreve os cabeçalhos
    char coluna = colunaInicial[0];
    for (size_t i = 0; i < cabecalhos.size(); i++) {
      String celulaAlvo = String(coluna) + "1";

      while ( escreverEmCelula(_boardID, celulaAlvo, cabecalhos[i]) == 0);

      coluna++; // Avança para a próxima coluna

      // Se passar de 'Z', volta para 'A' (opcional, apenas se necessário)
      if (coluna > 'Z') coluna = 'A';
    }
  }
}

void playRejectBeep() {
  Serial.println("Tocando bipe de REJEICAO...");
  tone(buzzer, 200);
  delay(150);
  noTone(150 / 2); 
  tone(buzzer, 100);
  delay(150);
  noTone(buzzer);
}

void playConfirmBeep() {
  Serial.println("Tocando bipe de CONFIRMACAO...");
  tone(buzzer, 1500); 
  delay(100);       
  noTone(buzzer);           
}