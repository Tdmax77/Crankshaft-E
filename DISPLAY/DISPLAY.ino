/*  20200921 spostato calcolo offset su encoder ma ci sono provlemi:
 *   il valore di offset viene risommato la prima volta che spengo e riaccendo il display
 *   offset viene chiesto 3 volte.
 *   
 *   
  Ricevitore per Encoder: riceve un segnale grezzo dall'encoder (numero di step) e lo converte in un angolo
  da visualizzare sul display .

  modulo nrf24
  CE   9
  SCN  10
  mosi 11
  miso 12
  sck  13
  pulsante 1 7
  const int buttonOk = 6;
  const int buttonUpPin = 4
  const int buttonDownPin = 5

*/
#include <RF24.h>
#include <nRF24L01.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include "printf.h"

#define risoluzioneEncoder  0.05 // INSERIRE QUI VALORE ENCODER uguale anche ne programma kren 360/7200=0.05 ma adesso va 360/2850

#define DEBUG  //if not commented out, serial.print is active!

/* variabile display*/
LiquidCrystal_I2C lcd(0x27, 16, 2);
long encoderValuePrev;  //serve a creare la routine per aggoirnare il display solo quando serve

/* variabile display*/

/*variabile network */
RF24 radio(9, 10);              // nRF24L01 (CE,CSN)
const uint64_t add1 = 0xf0f0f0f0e1LL;
char msg[20];
static unsigned long previousSuccessfulTransmission;
boolean transmissionState = true;       // per laggiornamento del display dopo che ha perso il segnale
boolean PretransmissionState = false;  // per conttrollare la prerdita/ripresa del segnale

/* conversione Angolo */
float Angolo;
volatile long encoderValue;
/* conversione Angolo */

//questa struttura manda i dati dall'encoder al display
struct EncoderData {
  bool offsetRequest = true; //se prima accensione richiederà l'offset
  //long encoderValueTX; // = encoderValue;
  float valoreangolocorretto;
};
EncoderData Data;

//questa struttura definisce l'ack payload
struct AckPayload {
  //int AngoloLetto; //valore letto sul volano
  int ValOffset; //valore letto sul volano
  bool isRestarted = true; // se display rileva chisura encoder ridomanda offset
};

AckPayload Ack;
/*variabile network */



/*Variabili Offset*/
const int buttonOkPin = 4;
const int buttonUpPin = 3;  //numero pin a cui è collegato il pulsante di UP
const int buttonDownPin = 2;  //numero pin a cui è collegato il pulsnte di DOWN
int buttonOkState;
int buttonUpState;  //stato attuale del pulsante di UP
int buttonDownState;  //stato attuale del pulsante di Down
long UpDebounceTime;  //Variabilie di appoggio per calcolare il tempo di cambio di stato del pulsante di UP
long DownDebounceTime;  //Variabilie di appoggio per calcolare il tempo di cambio di stato del pulsante di Down
long timerButtonPushed; //tempo che indica da quanto è stato premuto il pulsante
long timerPauseRepeat;  //tempo che indica da quanto è stata fatta l'ultima ripetizione
long time_add_10 = 1200;  //costante per attivare ripetizione da +10  era 1000
long time_add_100 = 9000; //costante per attivare ripetizione da +100  era 5000
long time_pause = 250;  //costante che determina il tempo di ripetizione di UP e di DOWN
long debounceDelay = 80; //Tempo di debounce per i pulsanti UP e DOWN
boolean repeatEnable = LOW; //memoria che indica il repeat del pulsante attivo

int var = 0;  //variabile da aumentare o diminuire come valore
const int varMax = 720; //limite massimo valore della variabile
const int varMin = 0; //limite minimo valore della variabile

int readingUp = 0;  //Lettura ingresso digitale del pulsante di UP
int readingDown = 0;  //Lettura ingresso digitale del pulsante di Down

/*Variabili Offset*/

/*Variabili Pulsanti */
int pulsante_1 = 0;
//const int buttonOkpin = 6;
//const int buttonUpPin = 4;  //numero pin a cui è collegato il pulsante di UP
//const int buttonDownPin = 5;  //numero pin a cui è collegato il pulsnte di DOWN
/*Variabili Pulsanti */


/**********************************************************************************************************/
/***********************   SETUP   ************************************************************************/
/**********************************************************************************************************/
void setup() {


  /* Setup network */
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();               // Allow optional ack payloads
  //radio.setPALevel(RF24_PA_MAX);
  radio.setPALevel(RF24_PA_MIN);
  radio.enableDynamicPayloads();
  // radio.setRetries(5, 5);                  // 5x250us delay (blocking!!), max. 5 retries
  radio.openReadingPipe(1, add1);
  radio.startListening();                 // Start listening
  /* Setup network */

  /* parte display i2c */
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Crankshaft");
  lcd.setCursor(0, 1);
  lcd.print("Wireless reader");
  delay(2000);
  lcd.clear();
  /* parte display i2c */

  /* Debug*/
#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
  radio.printDetails();
  delay(2000);
#endif

#ifndef DEBUG
  Serial.end(); // make sure, serial is off!
  //UCSR0B = 0b00000000;
#endif
  /* Debug*/


  /*Parte Variabile Offset*/
  pinMode (buttonUpPin, INPUT); //impostazione buttonUpPin come ingresso
  pinMode (buttonDownPin, INPUT); //impostazione buttonDownPin come ingresso
  pinMode (buttonOkPin, INPUT);
  /*Parte Variabile Offset*/

}

/**********************************************************************************************************/
/***********************   LOOP   ************************************************************************/
/**********************************************************************************************************/

void loop()
{

  Ack.ValOffset = var; //* 20;                                             // imposto il valore ValOffset in gradi da spedire //moltiplicando l'angolo inserito in offset * gli step per grado 7200/20

  if (radio.available())
  {
    radio.read(&Data, sizeof(struct EncoderData));
    radio.writeAckPayload(1, &Ack, sizeof(struct AckPayload));            // mando il valore di offset come ack !!!
    previousSuccessfulTransmission = millis();
  }


  /* messaggistica di controllo ************************************************************/
  check_Transmission();
  /* messaggistica di controllo ************************************************************/

  encoderValue = Data.valoreangolocorretto;
  //Angolo = (encoderValue * risoluzioneEncoder) + ( var * 20 );
  if (encoderValue != encoderValuePrev )       
  {
    encoderValuePrev = encoderValue;
    display_angolo();
    delay(5);
  }

if (Data.offsetRequest == true)
  {
    offset(); // scrive richiesta offset
    while (digitalRead(buttonOkPin) == LOW)
    {
      OFFSET();
      Ack.isRestarted = false;
    }
  }
  Data.offsetRequest =false;
  Ack.isRestarted = false;  // dopo aver impostato
  display_angolo();
  
if (transmissionState == true && PretransmissionState == false) {   //aggiunto per mostrare angolo dopo aver riagganciato il segnale
   
   lcd.setCursor(0, 0);
   lcd.print("                 ");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
   lcd.setCursor(0, 1);
   lcd.print("                 ");
   lcd.setCursor(4, 0);
   lcd.print("Angolo:");
   lcd.setCursor(1, 1);
   lcd.print(Data.valoreangolocorretto);
   lcd.setCursor(10, 1);
   lcd.print("Gradi");
   delay(200);

  }
  PretransmissionState = transmissionState;

 
}










/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/

  /* messaggistica di controllo ************************************************************/
void check_Transmission(){
  if (millis() - previousSuccessfulTransmission > 3500)                   // se non ricevo niente entro tot millisecondi
  {
    transmissionState = false;
#ifdef DEBUG
    Serial.println("Data transmission error, check Transmitter!");
#endif
    display_no_conn();
    delay(400);
  }
  else
  {
    transmissionState = true;   // se ricevo conrrettamente il segnale
    //display_angolo();
#ifdef DEBUG
    debug1();
#endif
  }
}
  /* messaggistica di controllo ************************************************************/



void readButtonState() {

  int readingUp = digitalRead(buttonUpPin); //Lettura ingresso digitale del pulsante di UP
  int readingDown = digitalRead(buttonDownPin); //Lettura ingresso digitale del pulsante di Down

  if (readingUp == HIGH) {
    if ((millis() - UpDebounceTime) > debounceDelay) {
      buttonUpState = HIGH;
    }
  } else {
    buttonUpState = LOW;
    UpDebounceTime = millis();
  }

  if (readingDown == HIGH) {
    if ((millis() - DownDebounceTime) > debounceDelay) {
      buttonDownState = HIGH;
    }
  } else {
    buttonDownState = LOW;
    DownDebounceTime = millis();
  }

}






void OFFSET() // mi restituisce un valore var che ho inserito come offset
{
#ifdef DEBUG
  Serial.println("");
  Serial.println("SONO NELLA PROCEDURA OFFSET()");
  Serial.println("");
#endif
  readButtonState();  //Lettura stato buttons con controllo antirimbalzo

  if (buttonUpState == HIGH || buttonDownState == HIGH) {
    if ((repeatEnable == HIGH && ((millis() - timerPauseRepeat) > time_pause)) || repeatEnable == LOW) {
      if ((millis() - timerButtonPushed) > time_add_10) {
        if ((millis() - timerButtonPushed) > time_add_100) {
          if (buttonUpState == HIGH) var = var + 100;
          if (buttonDownState == HIGH) var = var - 100;
        } else {
          int resto = 0;
          if (buttonUpState == HIGH) resto = 10 - (var % 10);
          if (buttonDownState == HIGH) resto = (var % 10);
          if (resto == 0) {
            if (buttonUpState == HIGH) var = var + 10;
            if (buttonDownState == HIGH) var = var - 10;
          } else {
            if (buttonUpState == HIGH) var = var + resto;
            if (buttonDownState == HIGH) var = var - resto;
          }
        }
      } else {
        if (buttonUpState == HIGH) var++;
        if (buttonDownState == HIGH) var--;
      }
      timerPauseRepeat = millis();
      repeatEnable = HIGH;
      if (var > varMax) var = varMax;
      if (var < varMin) var = varMin;
      lcd.setCursor(0, 0);
      lcd.print("                 ");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
      lcd.setCursor(0, 1);
      lcd.print("                 ");
      lcd.setCursor(0, 0);
      lcd.print("Inserire Offset:");
      lcd.setCursor(1, 1);
      lcd.print(var);
      lcd.setCursor(10, 1);
      lcd.print("Gradi");
      delay(200);

    }

  } else {
    timerButtonPushed = millis();
    timerPauseRepeat = millis();
    repeatEnable = LOW;
  }

#ifdef DEBUG
  Serial.println("");
  Serial.print("Valore impostato su var    ");
  Serial.println(var);
  Serial.print("Valore impostato Ack.isRestarted       ");
  Serial.println(Ack.isRestarted);
  Serial.print("Valore impostato  Data.offserRequest   ");
  Serial.println(Data.offsetRequest);
  Serial.println("");
  Serial.println("");
  Serial.println("");
#endif

//delay(500);

}


void debug1() {

  Serial.println("Data successfully received");
  Serial.println("DATI RICEVUTI");
  Serial.print("Data.valoreangolocorretto    ");
  Serial.println(Data.valoreangolocorretto);
  Serial.print("Data.offsetRequest      ");
  Serial.println(Data.offsetRequest);
  Serial.println("");
  Serial.println("");
  Serial.println("DATI INVIATI ");
  Serial.print("Ack.ValOffset     ");
  Serial.println(Ack.ValOffset);
  Serial.print("Ack.isRestarted     ");
  Serial.println(Ack.isRestarted);
  Serial.println("");
  Serial.println("");

}

void display_no_conn() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("NO CONNECTION    ");
  lcd.setCursor(0, 1);
  lcd.println("Check Encoder   ");
}

void offset() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("INSERIRE VALORE ");
  lcd.setCursor(0, 1);
  lcd.println("ANGOLO VOLANO   ");
}

void display_angolo() {
  lcd.setCursor(0, 0);
  lcd.print("                 ");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
  lcd.setCursor(0, 1);
  lcd.print("                 ");
  lcd.setCursor(4, 0);
  lcd.print("Angolo:");
  lcd.setCursor(1, 1);
  lcd.print(Data.valoreangolocorretto);
  lcd.setCursor(10, 1);
  lcd.print("Gradi");
}
