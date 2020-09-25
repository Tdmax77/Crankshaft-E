/*  Questo programma va su un Nano con display 1602 e modulo radio nrf24l01.
    Funzionamento:
    Flusso trasmissione dati:

    Encoder > Data.offsetRequet          > Display
            > Data.valoreangolocorretto  >
            < Ack.offset_impostato       <
            < Ack.ValOffset              <

    Encoder appena acceso ha la variabile Data.offsetRequest a 1 ovvero manda una richiesta di inserimento offset al display;
    Display visualizza la richiesta, registra un dato var che inserisco io e lo spedisce come Ack.Valoffset all'Encoder, poi spedisce anche un Ack.offset_impostato a 1 (era a zero) ad encoder
    Quando encoder riceve il Ack.offsetimpostato a 1 allora manda a 0 Data.offsetRequest (e quindi non chiederà più un offset).
    Il programma procede visualizzando il dato Data.valoreangolocorretto che è la somma di Ack.Valoffset + il valore letto dall'encoder.
    il calcolo lo fa l'encoder, in tal modo se dovessi spegnere il display o perdere il segnale, il dato non cambierà.
    Se l'encoder viene spento, display avvisa che non lo riceve (anche se va fuori campo).
    Quando si riaccende l'encoder, la variabile Data.offsetRequest parte a 1 quindi chiede di reinserire un valore di offset.


  Connessioni Display:

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
long encoderValuePrev;  //serve a creare la routine per aggiornare il display solo quando serve

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
  float valoreangolocorretto; // valore che verrà visualizzato ovvero la somma del valore dell'encoder + l'offset che ho inserito
};
EncoderData Data;

//questa struttura definisce l'ack payload ovvero i dati dal display all'encoder
struct AckPayload {
  int ValOffset = 0; //valore letto sul volano e impostato dal display
  bool offset_impostato = false; // se display rileva chisura encoder ridomanda offset
};
AckPayload Ack;
/*variabile network */



/*Variabili Offset servono ad inserire il valore usando i 3 pulsanti*/
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
/*Variabili Pulsanti */

bool IMPOSTATO_DA_DISPLAY = false;



/**********************************************************************************************************/
/***********************   SETUP   ************************************************************************/
/**********************************************************************************************************/
void setup() {


  /* Setup network */
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(1);
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
  delay(3000);
  lcd.clear();
  /* parte display i2c */

  /* Debug*/
#ifdef DEBUG
  Serial.begin(9600);
  printf_begin();
  radio.printDetails();
#endif

#ifndef DEBUG
  Serial.end(); // make sure, serial is off!
#endif
  /* Debug*/


  /*Parte Variabile Offset*/
  pinMode (buttonUpPin, INPUT);   //impostazione buttonUpPin come ingresso
  pinMode (buttonDownPin, INPUT); //impostazione buttonDownPin come ingresso
  pinMode (buttonOkPin, INPUT);   //impostazione buttonOkPin come ingresso
  /*Parte Variabile Offset*/
}





















































/**********************************************************************************************************/
/***********************   LOOP   ************************************************************************/
/**********************************************************************************************************/

void loop()
{
  switch (radio.available()) {
    case  false:
      lcd.setCursor(0, 0);
      lcd.print("                 ");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
      lcd.setCursor(0, 1);
      lcd.print("                 ");
      lcd.setCursor(0, 0);
      lcd.print("ENCODER SPENTO");

      //display_no_conn();
      break;
    default:
      PROCESSO();

  }
}











/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/

void PROCESSO() {
  /* messaggistica di controllo ************************************************************/
  //check_Transmission();
  /* messaggistica di controllo ************************************************************/
  radio.read(&Data, sizeof(struct EncoderData));
  int richiestaoffset = Data.offsetRequest;

  switch (richiestaoffset) {
    case true:
      testo_richiesta_inserimento_offset();
      while (digitalRead(buttonOkPin) == LOW )
      {
        PROCEDURA_OFFSET();
      }
#ifdef DEBUG
      Serial.println("procedura offset finita  ");
#endif

      Ack.offset_impostato = true;
      Ack.ValOffset = var;
      radio.writeAckPayload(1, &Ack, sizeof(struct AckPayload));
      delay(5);
      break;

    default:
      radio.writeAckPayload(1, &Ack, sizeof(struct AckPayload));    // mando il valore di offset come ack !!!
      delay (5);  //permette la spedizione del segnale garantendo il tempo di propagazione (in teoria da aliverti channel)
      previousSuccessfulTransmission = millis();
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
/*  //     radio.writeAckPayload(1, &Ack, sizeof(struct AckPayload));    // mando il valore di offset come ack !!!
  //     delay (5);  //permette la spedizione del segnale garantendo il tempo di propagazione (in teoria da aliverti channel)
  //     previousSuccessfulTransmission = millis();

#ifdef DEBUG
  Serial.println ("scrivo dati radio");
  Serial.print("Ack.ValOffset     ");
  Serial.println(Ack.ValOffset);
  Serial.print("Valore impostato Ack.offset_impostato    ");
  Serial.println(Ack.offset_impostato);
#endif

  /*  while (Data.offsetRequest == true && Ack.offset_impostato == false)
    {
      testo_richiesta_inserimento_offset();
      while (digitalRead(buttonOkPin) == LOW )
      {
        PROCEDURA_OFFSET();
      }
    #ifdef DEBUG
      Serial.println("procedura offset finita  ");
    #endif

      Ack.offset_impostato = true;
      Ack.ValOffset = var;
      radio.writeAckPayload(1, &Ack, sizeof(struct AckPayload));
      delay(5);
    }
  */
  /*
#ifdef DEBUG
  Serial.println (" in questo momento ho finito l'offset e mandato ack quindi dovrei mostrare l'angolo sul display");
  //display_angolo();
#endif

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
  */
}






/* messaggistica di controllo ************************************************************/
void check_Transmission() {

  if (millis() - previousSuccessfulTransmission > 500)                   // se non ricevo niente entro tot millisecondi
  {
    transmissionState = false;
#ifdef DEBUG
    Serial.println("Data transmission error, check Transmitter!");
#endif
    do {
      display_no_conn();
    } while (transmissionState = false) ;
  }
  else
  {
    transmissionState = true;   // se ricevo conrrettamente il segnale
    // display_angolo();
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






void PROCEDURA_OFFSET() // mi restituisce un valore var che ho inserito come offset
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
      //delay(200);

    }

  } else {
    timerButtonPushed = millis();
    timerPauseRepeat = millis();
    repeatEnable = LOW;
    //   Serial.println ("cambio la variabile offset impostato a 1");
    //Ack.offset_impostato = true;
    //Ack.ValOffset = var;
    //radio.writeAckPayload(1, &Ack, sizeof(struct AckPayload));
    //delay(5);

  }

  /*#ifdef DEBUG
    Serial.println("");
    Serial.print("Valore impostato su var    ");
    Serial.println(var);
    Serial.print("Valore impostato Ack.offset_impostato    ");
    Serial.println(Ack.offset_impostato);
    Serial.print("Valore impostato  Data.offserRequest   ");
    Serial.println(Data.offsetRequest);
    Serial.println("");
    Serial.println("");
    Serial.println("");
    #endif
  */
}


void debug() {

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
  Serial.print("Valore impostato Ack.offset_impostato    ");
  Serial.println(Ack.offset_impostato);
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

void testo_richiesta_inserimento_offset() {
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
  lcd.print("angolocorretto");
  //lcd.print(Data.valoreangolocorretto);
  lcd.setCursor(10, 1);
  lcd.print("Gradi");
}
