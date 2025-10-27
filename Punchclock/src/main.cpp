#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);
const byte UID_SIZE = 4;

struct Registros {
    char uid_str[12];
    char nome[10];  
    bool estado;
};

Registros LISTA_PESSOAS[] = {
    {"D3 EB 1E F4", "Edmur", 0},  
};

const int MAX_PESSOAS = 50;
Registros LISTA_PESSOAS[MAX_PESSOAS];
int numPessoas = 0;

#define ModeSwitch 34
#define CLOCK 22
#define DATA 21

const char keymap[] = {
  0, 0,  0,  0,  0,  0,  0,  0,
  0, 0,  0,  0,  0,  0, '`', 0,
  0, 0 , 0 , 0,  0, 'q','1', 0,
  0, 0, 'z','s','a','w','2', 0,
  0,'c','x','d','e','4','3', 0,
  0,' ','v','f','t','r','5', 0,
  0,'n','b','h','g','y','6', 0,
  0, 0, 'm','j','u','7','8', 0,
  0,',','k','i','o','0','9', 0,
  0,'.','/','l',';','p','-', 0,
  0, 0,'\'', 0,'[', '=', 0, 0,
  0, 0,13, ']', 0, '\\', 0, 0,
  0, 0, 0, 0, 0, 0, 127, 0,
  0,'1', 0,'4','7', 0, 0, 0,
  '0','.','2','5','6','8', 0, 0,
  0,'+','3','-','*','9', 0, 0,
  0, 0, 0, 0 };

uint8_t lastscan;

#define SS_PIN  5     
#define RST_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Cria o objeto do leitor

bool UIDvalido = true;
int indice;

String formatarUID(byte *buffer, byte bufferSize);
const char* buscarNomePorUID(String uid_procurado);

void setup(){
  lcd.init();                      
  lcd.backlight();
  
  pinMode(ModeSwitch, INPUT_PULLUP)
  pinMode(CLOCK, INPUT_PULLUP); // Define o pino CLOCK como entrada com pull-up.
  pinMode(DATA, INPUT_PULLUP);

  bitSet(PCICR, PCIE2);
  bitSet(PCMSK2, CLOCK);

  Serial.begin(115200);
  SPI.begin(18, 19, 23, SS_PIN);   
  mfrc522.PCD_Init();              
  Serial.println("Aproxime o cartão mfrc522 do leitor...");
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid_lido = formatarUID(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print("TAG UID: ");
    Serial.print(uid_lido);
    const char* pessoa = buscarNomePorUID(uid_lido);
    if(UIDvalido){
      bool acao = LISTA_PESSOAS[indice].estado;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(pessoa);
      Serial.println(pessoa);
      lcd.setCursor(10,1);
      lcd.print(!acao ? "entrou" : "saiu");
      Serial.println(!acao ? "entrou" : "saiu");
      LISTA_PESSOAS[indice].estado = !LISTA_PESSOAS[indice].estado;
      delay(1000);
      lcd.clear();
    }else{
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("TAG INVALIDO!");
      Serial.println("TAG INVALIDO!");
      delay(1000);
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
    for (int i = 0; i < MAX_PESSOAS; i++) {
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