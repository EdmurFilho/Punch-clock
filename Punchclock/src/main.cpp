#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <string.h>

LiquidCrystal_I2C lcd(0x27,16,2);
const byte UID_SIZE = 4;

struct Registros {
    char uid_str[12];
    char nome[10];  
    bool estado;
};

const int MAX_PESSOAS = 50;
Registros LISTA_PESSOAS[MAX_PESSOAS];

int numPessoas = 0;

#define ModeSwitch 34
#define CLOCK 16
#define DATA 17

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

volatile uint8_t bit_count = 0;
volatile uint32_t data_buffer = 0;
volatile unsigned long last_interrupt_time = 0;

volatile uint8_t scancode = 0;
volatile bool new_data_available = false;

char my_string[10];
int string_index = 0;

void ICACHE_RAM_ATTR ps2_interrupt();

#define SS_PIN  5     
#define RST_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Cria o objeto do leitor
void Confirm_UID();

bool UIDvalido = true;
int indice;

String formatarUID(byte *buffer, byte bufferSize);
const char* buscarNomePorUID(String uid_procurado);

void setup(){
  lcd.init();                      
  lcd.backlight();
  
  pinMode(ModeSwitch, INPUT);
  pinMode(CLOCK, INPUT_PULLUP); // Define o pino CLOCK como entrada com pull-up.
  pinMode(DATA, INPUT_PULLUP);
 
  attachInterrupt(digitalPinToInterrupt(CLOCK), ps2_interrupt, FALLING);
  my_string[0] = '\0';

  Serial.begin(115200);
  SPI.begin(18, 19, 23, SS_PIN);   
  mfrc522.PCD_Init();              
  Serial.println("Aproxime o cartão mfrc522 do leitor...");
}

uint8_t last_processed_scan = 0;

void loop() {
  bool Mode = digitalRead(ModeSwitch);
  if (!Mode && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Confirm_UID();
  }
  while (Mode){
    char* completed_string = handleKeyboardInput(lcd);
    if (completed_string != nullptr) {
      Serial.print("String recebida no loop: ");
      Serial.println(completed_string);
      Serial.print("registrando, apresente o TAG");
      lcd.setCursor(0,0);
      lcd.print("Apresente a TAG!");
      if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()){
        String uid_lido = formatarUID(mfrc522.uid.uidByte, mfrc522.uid.size);
        adicionarPessoa(uid_lido, completed_string);
        delay(500);
        lcd.clear();
      }
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

void ICACHE_RAM_ATTR ps2_interrupt() {
  bool data_bit = digitalRead(DATA);
  last_interrupt_time = millis();
  bit_count++;

  switch (bit_count) {
    case 1:
      data_buffer = 0;
      if (data_bit != 0) bit_count = 0;
      break;
    case 2: case 3: case 4: case 5:
    case 6: case 7: case 8: case 9:
      data_buffer |= (data_bit << (bit_count - 2));
      break;
    case 10:
      break;
    case 11:
      if (data_bit == 1) {
        scancode = (uint8_t)data_buffer;
        new_data_available = true;
      }
      bit_count = 0;
      break;
    default:
      bit_count = 0;
      break;
  }
}

void Confirm_UID(){
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

char* handleKeyboardInput(LiquidCrystal_I2C &lcd) {
  static char my_string[10];
  static int string_index = 0;
  static uint8_t last_processed_scan = 0;

  // Lógica de Timeout (exatamente como antes)
  if (bit_count > 0 && (millis() - last_interrupt_time > 50)) {
    bit_count = 0;
  }

  if (new_data_available) {
    uint8_t current_scancode;

    noInterrupts();
    current_scancode = scancode;
    new_data_available = false;
    interrupts();

    if (current_scancode != 0xF0 && last_processed_scan != 0xF0) {
      char c = keymap[current_scancode];
      
      if (c == 127) { // Backspace
        if (string_index > 0) {
          string_index--;
          my_string[string_index] = '\0';
          lcd.setCursor(string_index, 1);
          lcd.print(" ");
          lcd.setCursor(string_index, 1);
        }
      }
      else if (c == 13) { // Enter
        Serial.print("Nome digitado (dentro da funcao): ");
        Serial.println(my_string);
        
        string_index = 0; 
        return my_string; 
      }
      else if (c != 0 && string_index < 10) { // Caractere normal
        my_string[string_index] = c;
        string_index++;
        my_string[string_index] = '\0';
        
        lcd.setCursor(0, 1);
        lcd.print(my_string);
      }
    }
    last_processed_scan = current_scancode;
  }

  // Se 'Enter' não foi pressionado, retorna nulo.
  return nullptr;
}

bool adicionarPessoa(const String& uid, const char* nome) {
  if (numPessoas >= MAX_PESSOAS) {
    Serial.println("ERRO: Limite de registros atingido. Nao pode adicionar.");
    return false;
  }

  int indice = numPessoas;

  // 1. COPIANDO O UID (de String para char[])
  // Usamos .c_str() para obter o ponteiro char* da String
  strncpy(LISTA_PESSOAS[indice].uid_str, uid.c_str(), sizeof(LISTA_PESSOAS[0].uid_str) - 1);
  LISTA_PESSOAS[indice].uid_str[sizeof(LISTA_PESSOAS[0].uid_str) - 1] = '\0'; // Garante terminação nula

  // 2. Copiando o NOME (de const char* para char[])
  strncpy(LISTA_PESSOAS[indice].nome, nome, sizeof(LISTA_PESSOAS[0].nome) - 1);
  LISTA_PESSOAS[indice].nome[sizeof(LISTA_PESSOAS[0].nome) - 1] = '\0';

  // 3. Definir o estado e incrementar o contador
  LISTA_PESSOAS[indice].estado = false;
  numPessoas++;

  Serial.print("Pessoa adicionada: ");
  Serial.println(nome);
  return true;
}