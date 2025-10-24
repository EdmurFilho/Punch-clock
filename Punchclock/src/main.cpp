#include <SPI.h>
#include <MFRC522.h>

// 🟢 Definição dos pinos (para o SPI padrão do ESP32)
#define SS_PIN  5     // SDA no módulo RC522
#define RST_PIN 22    // RST no módulo RC522

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Cria o objeto do leitor

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, SS_PIN);   // SCK=18, MISO=19, MOSI=23, SS=5
  mfrc522.PCD_Init();              // Inicializa o RC522
  Serial.println("Aproxime o cartão RFID do leitor...");
}

void loop() {
  // Verifica se há um novo cartão
  if (!mfrc522.PICC_IsNewCardPresent()) return;

  // Verifica se o cartão pode ser lido
  if (!mfrc522.PICC_ReadCardSerial()) return;

  Serial.print("TAG UID: ");

  // Lê os bytes do UID e imprime em HEX
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Opcional: mostra o tipo do cartão
  MFRC522::PICC_Type tipo = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print("Tipo do cartão: ");
  Serial.println(mfrc522.PICC_GetTypeName(tipo));

  // Para evitar leituras repetidas rápidas
  delay(1000);
}
