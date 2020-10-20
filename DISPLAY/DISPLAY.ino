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
  pulsante 1 non utilizzato = 7;
  const int buttonOk = 6;
  const int buttonUpPin = 4;
  const int buttonDownPin = 5;

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
/* variabile display*/

/*variabile network */
RF24 radio(9, 10);              // nRF24L01 (CE,CSN)
const uint64_t add1 = 0xf0f0f0f0e1LL;
char msg[20];
static unsigned long previousSuccessfulTransmission;
boolean transmissionState = true;       // per laggiornamento del display dopo che ha perso il segnale
boolean PretransmissionState = false;   // per conttrollare la prerdita/ripresa del segnale

/* conversione Angolo */
float Angolo;
/* conversione Angolo */

//questa struttura manda i dati dall'encoder al display
struct EncoderData {
  bool offsetRequest = true; //se prima accensione richiederà l'offset
  float valoreangolocorretto; // valore che verrà visualizzato ovvero la somma del valore dell'encoder + l'offset che ho inserito
  bool cw = true; //se prima accensione richiederà se il motore è CW o CCW
};
EncoderData Data;

//questa struttura definisce l'ack payload ovvero i dati dal display all'encoder
struct AckPayload {
  int ValOffset = 0; //valore letto sul volano e impostato dal display
  bool offset_impostato = false; // se display rileva chisura encoder ridomanda offset
  bool cwi = true;
};
AckPayload Ack;

float valoreangolocorrettoPrev;
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

/*Variabili Offset servono ad inserire il valore usando i 3 pulsanti*/

/*Variabili Pulsanti */
int pulsante_1 = 0; // non utilizzato
/*Variabili Pulsanti */



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
<<<<<<< HEAD

=======
>>>>>>> parent of 3095eb3... FUNZIONA TUTTO
  switch (radio.available()) {  // verifico se ho l'encoder acceso
    case  false:   // se non ho segnale scrivo sul display No connection (funziona bene)
      display_no_conn();
      break;

    default:
<<<<<<< HEAD
      if (mostraangolo == 1) {
        //quando riprendo il segnale visualizzo l'ultimo dato dell'angolo (non si aggiorna se non cambia altrimenti)
        lcd.setCursor(0, 0);
        if (Data.cw) lcd.print("CW     Angolo:  ");
        else lcd.print   ("CCW    Angolo:  ");
        lcd.setCursor(10, 1);
        lcd.print("Gradi");
        lcd.setCursor(0, 1);
        lcd.print("          ");
        lcd.setCursor(1, 1);
        lcd.print(Data.valoreangolocorretto);
        mostraangolo = 0;

      }

      radio.read(&Data, sizeof(struct EncoderData));  //leggo i dati
      int richiestaoffset = Data.offsetRequest;   //lo switch non va con boolean devo quindi convertire in int
      int offsetimpostatovariabile = 0;           // seconda condizione necessaria per non richiedere doppio ok alla procedura di offset ... (mistero)
      Serial.print ("prima dello switch CW ");
      Serial.println (Data.cw);
=======
      radio.read(&Data, sizeof(struct EncoderData));  //leggo i dati
      int richiestaoffset = Data.offsetRequest;

>>>>>>> parent of 3095eb3... FUNZIONA TUTTO
      Serial.print ("prima dello switch off req  ");
      Serial.println (Data.offsetRequest);
      Serial.print ("prima dello switch off impost  ");
      Serial.println (Ack.offset_impostato);
<<<<<<< HEAD
      Serial.print ("variabile intera richiestaoffset   ");
      Serial.println(richiestaoffset);
      Serial.print ("variabile intera offsetimpostatovariabile   ");
      Serial.println(offsetimpostatovariabile);
      Serial.println ("  ");
      Serial.println ("  ");




      switch (richiestaoffset && !offsetimpostatovariabile) {                      // se è richiesto l'offset (prima accensione di encoder) eseguo procedura
=======
      switch (richiestaoffset) {                      // se è richiesto l'offset (prima accensione di encoder) eseguo procedura
>>>>>>> parent of 3095eb3... FUNZIONA TUTTO
        case true:
          testo_richiesta_inserimento_cw();
          while (digitalRead(buttonOkPin) == LOW )
          {
            PROCEDURA_CW();
          }
          testo_richiesta_inserimento_offset();
<<<<<<< HEAD
          offsetimpostatovariabile = 1;

=======
>>>>>>> parent of 3095eb3... FUNZIONA TUTTO
          while (digitalRead(buttonOkPin) == LOW )
          {
            PROCEDURA_OFFSET();
            Serial.println ("sono nel while dopo la procedura  ");
            Serial.print ("prima dello switch off req  ");
            Serial.println (Data.offsetRequest);
            Serial.print ("prima dello switch off impost  ");
            Serial.println (Ack.offset_impostato);
          }
          Ack.offset_impostato = true;
          Ack.ValOffset = var;
          //Ack.cwi = Data.cw;
          radio.writeAckPayload(1, &Ack, sizeof(struct AckPayload));
          radio.read(&Data, sizeof(struct EncoderData));
          delay(5);
          break;
          Serial.print ("sono dopo il break  ");
          Serial.print ("prima dello switch off req  ");
          Serial.println (Data.offsetRequest);
          Serial.print ("prima dello switch off impost  ");
          Serial.println (Ack.offset_impostato);

        default:
        Serial.print ("sononel default ");
          Serial.print ("prima dello switch off req  ");
          Serial.println (Data.offsetRequest);
          Serial.print ("prima dello switch off impost  ");
          Serial.println (Ack.offset_impostato);
          if (Data.valoreangolocorretto != valoreangolocorrettoPrev){  //ridisegno il display unicamente se il dato dell'angolo cambia (evito flashamenti )
            valoreangolocorrettoPrev = Data.valoreangolocorretto;
            display_angolo();
          }
          
          break;
      }
  }
}



/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/

<<<<<<< HEAD
//messaggistica di controllo ************************************************************
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
=======
/* messaggistica di controllo ************************************************************
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
>>>>>>> parent of 3095eb3... FUNZIONA TUTTO
  }
  /*  else
    {
    transmissionState = true;   // se ricevo conrrettamente il segnale
    // display_angolo();
    }*/
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
      delay(200);
    }
  } else {
    timerButtonPushed = millis();
    timerPauseRepeat = millis();
    repeatEnable = LOW;
  }

}

void PROCEDURA_CW() {//
#ifdef DEBUG
  Serial.println("");
  Serial.println("SONO NELLA PROCEDURA CW()");
  Serial.println("");
#endif
  readButtonState();  //Lettura stato buttons con controllo antirimbalzo

  if (buttonUpState == HIGH) {
    Ack.cwi = true;
    lcd.setCursor(0, 0);
    lcd.print("Motore impostato");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
    lcd.setCursor(0, 1);
    lcd.print("   Orario       ");
    delay(200);
  }
  if (buttonDownState == HIGH) {
    Ack.cwi = false;
    lcd.setCursor(0, 0);
    lcd.print("Motore impostato");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
    lcd.setCursor(0, 1);
    lcd.print("   Antiorario   ");
    delay(200);
  }
  Serial.print ("prima dello switch Data.CW ");
  Serial.println (Data.cw);
  Serial.print ("prima dello switch Ack.cwi ");
  Serial.println (Ack.cwi);
}



void display_no_conn() {

  lcd.setCursor(0, 0);
  lcd.println("NO CONNECTION   ");
  lcd.setCursor(0, 1);
  lcd.println("Check Encoder   ");
}

void testo_richiesta_inserimento_offset() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("INSERIRE ANGOLO ");
  lcd.setCursor(0, 1);
  lcd.println("LETTO SUL VOLANO");
}

void testo_richiesta_inserimento_cw() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println(" INSERIRE SENSO ");
  lcd.setCursor(0, 1);
  lcd.println(" DI ROTAZIONE   ");
}


void display_angolo() {
<<<<<<< HEAD
  if (Data.valoreangolocorretto != valoreangolocorrettoPrev)//ridisegno il display unicamente se il dato dell'angolo cambia (evito flashamenti )
  {
    valoreangolocorrettoPrev = Data.valoreangolocorretto;
    lcd.setCursor(0, 0);
    if (Data.cw) {
      lcd.print ("CW     Angolo:  ");
    }

    else {
      lcd.print   ("CCW    Angolo:  ");
    }

    lcd.setCursor(10, 1);
    lcd.print("Gradi ");
    lcd.setCursor(0, 1);
    lcd.print("          ");
    lcd.setCursor(1, 1);
    lcd.print(Data.valoreangolocorretto);
  }
=======
  // lcd.setCursor(0, 0);
  // lcd.print("                 ");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
  lcd.setCursor(0, 0);
  lcd.print("    Angolo:     ");
  lcd.setCursor(10, 1);
  lcd.print("Gradi");
  lcd.setCursor(0, 1);
  lcd.print("          ");
  lcd.setCursor(1, 1);
  lcd.print(Data.valoreangolocorretto);
>>>>>>> parent of 3095eb3... FUNZIONA TUTTO

}
