#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

const byte UID_SIZE = 4;

struct Funcionario {
    const char* uid_str; 
    const char* nome;    
};

const Funcionario LISTA_PESSOAS[] = {
    {"D3 EB 1E F4", "Edmur"},  
};

const int NUM_PESSOAS = sizeof(LISTA_PESSOAS) / sizeof(LISTA_PESSOAS[0]);

#define SS_PIN  5     // SDA no módulo RC522
#define RST_PIN 4    // RST no módulo RC522

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Cria o objeto do leitor

void setup() {
  lcd.init();                      
  lcd.backlight();
  
  Serial.begin(115200);
  SPI.begin(18, 19, 23, SS_PIN);   // SCK=18, MISO=19, MOSI=23, SS=5
  mfrc522.PCD_Init();              // Inicializa o RC522
  Serial.println("Aproxime o cartão mfrc522 do leitor...");
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid_lido = formatarUID(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print("TAG UID: ");
    Serial.print(uid_lido);
  }
  delay(1000);
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
            return LISTA_PESSOAS[i].nome; // Encontrou! Retorna o nome
        }
    }
    Serial.print("UID DESCONHECIDO: ");
    Serial.println(uid_procurado);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NAO REGISTRADO!");
    lcd.setCursor(0, 1);
    lcd.print("Cadastre a TAG");
    return nullptr; // UID não encontrado
}