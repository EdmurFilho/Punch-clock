#include <Arduino.h>
const long BAUD_RATE = 115200; 

void setup() {
  // 1. Inicializa a comunicação serial na velocidade definida.
  Serial.begin(BAUD_RATE);
  
  // O ESP32 pode ser rápido, mas é bom dar um pequeno atraso.
  delay(100); 
  
  Serial.println("\n--- ESP32 Inciado ---");
  Serial.print("Taxa de comunicacao: ");
  Serial.println(BAUD_RATE);
  Serial.println("Pronto para receber comandos.");
}

void loop() {
  // 2. Verifica se há dados disponíveis para leitura vindos do CH340/PC.
  if (Serial.available() > 0) {
    // Leitura dos dados: lê toda a string até encontrar uma quebra de linha ('\n').
    // Certifique-se de que o Monitor Serial está configurado para 'Newline' ou 'Carriage return'.
    String comando = Serial.readStringUntil('\n'); 
    
    // Remove espaços em branco antes e depois.
    comando.trim(); 
    
    Serial.print("Voce enviou: ");
    Serial.println(comando);
    
    // Você pode adicionar lógica de comando aqui. Exemplo:
    if (comando == "STATUS") {
      Serial.println("Status: Funcionando em 3.3V.");
    }
  }

  // 3. Envia uma mensagem de status a cada 1 segundo (transmissão do ESP32).
  static unsigned long ultimoEnvio = 0;
  if (millis() - ultimoEnvio >= 1000) {
    Serial.print("Contagem: ");
    Serial.println(millis() / 1000); // Mostra o tempo em segundos
    ultimoEnvio = millis();
  }
}