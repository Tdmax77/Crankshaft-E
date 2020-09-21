/*20200921 spostato calcolo offset su encoder ma ci sono provlemi:
 *   il valore di offset viene risommato la prima volta che spengo e riaccendo il display
 *   offset viene chiesto 3 volte.
 *   
 * 
  Encoder: domanda un offset alla prima accensione
  lo riceve nell ack e lo somma al valore letto dall'encoder
  poi lo rispedisce al display per farlo visualizzare
  modulo nrf24
  CE   9
  SCN  10
  mosi 11
  miso 12
  sck  13
  pulsante 1 4
  pulsante 2 5
  pulsante 3 6
  pulsante 4 7

*/
#include <RF24.h>
#include <nRF24L01.h>
#include <SPI.h>
#include "printf.h"

#define DEBUG  //if not commented out, serial.print is active!

/* encoder*/
#define encoderPin1  3
#define encoderPin2  2
#define risoluzioneEncoder  0.05 /* INSERIRE QUI VALORE ENCODER uguale anche nel programma COTO 360/7200=0.05, se usiamo in quadratura va 360/28800
  http://www.sciamannalucio.it/arduino-encoder-conta-impulsi-giri-motore/
   tecnicamente adesso la risoluzione si ottiene con 360/7200 perchè (vedi sezione encoder) usiamo un solo fronte per il conteggio e non 4 */
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
float Angolo = 0;
long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;
/* endocder */

/*Variabili network */
RF24 radio(9, 10);               // nRF24L01 (CE,CSN)
const uint64_t add1 = 0xf0f0f0f0e1LL;
char msg[20];
//int AngoloLetto;
int ValOffset;
boolean transmissionState;
static unsigned long previousSuccessfulTransmission;


//questa struttura manda i dati dall'encoder al display
struct EncoderData {
  bool offsetRequest = true; //se prima accensione richiederà l'offset
  //long encoderValueTX; //= encoderValue;
  float valoreangolocorretto;
};
EncoderData Data;

//questa struttura definisce l'ack payload
struct AckPayload {
  //int AngoloLetto; //valore letto sul volano
  int ValOffset;
  bool isRestarted = true; // se display rileva chisura encoder ridomanda offset
};
AckPayload Ack;

/*Variabili network */


/***********************************************************************************************************/
/*************************** SETUP  ************************************************************************/
/***********************************************************************************************************/


void setup() {

  /* Setup network */
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();               // Allow optional ack payloads
  // radio.setPALevel(RF24_PA_MAX);
  radio.setPALevel(RF24_PA_MIN);
  radio.enableDynamicPayloads();
  // radio.setRetries(5, 5);                  // 5x250us delay (blocking!!), max. 5 retries
  radio.openWritingPipe(add1);       // Both radios listen on the same pipes by default, and switch when writing
  /* Setup network */

  /* Debug*/
#ifdef DEBUG
  Serial.begin (115200);
  printf_begin();
  radio.printDetails();
  delay(200);
#endif

#ifndef DEBUG
  Serial.end(); // make sure, serial is off!
  //UCSR0B = 0b00000000;
#endif

  /*Debug*/

  /* encoder*/
  pinMode(encoderPin1, INPUT_PULLUP);
  pinMode(encoderPin2, INPUT_PULLUP);
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
  /* encoder*/

}




/***********************************************************************************************************/
/*************************** LOOP  ************************************************************************/
/***********************************************************************************************************/


void loop() {
#ifdef DEBUG
    delay(1000);
    debug3();
   
#endif

  //Data.encoderValueTX = encoderValue;
  Data.valoreangolocorretto = (encoderValue * risoluzioneEncoder) + Ack.ValOffset ; //AngoloLetto;
  if (radio.write(&Data, sizeof(struct EncoderData)))                     // se radio attiva trasmetto
  {
    //debug1();
    if (radio.isAckPayloadAvailable())                                    // leggo ack
    {
      radio.read(&Ack, sizeof(struct AckPayload));
     // debug2();
      previousSuccessfulTransmission = millis();
    }
  }

  /* ****************************************************** CONTROLLO RICEZIONE DATI ************************************/
  if (millis() - previousSuccessfulTransmission > 1500)                  //se maggiore di tot non ricevuto ack
  {
    transmissionState = false;
#ifdef DEBUG
    Serial.println("Data transmission error, check receiver!");
#endif
  }
  else                                                                                // rivecuto
  {
    transmissionState = true;
#ifdef DEBUG
    Serial.println("Data successfully transmitted");
#endif
  }
  /* ****************************************************** CONTROLLO RICEZIONE DATI ************************************/

 if (Ack.isRestarted == false && Data.offsetRequest == true)   // se Display azzera il contatore dopo aver inserito l'offest, azzero anche il dato
  { 
     Data.offsetRequest = false;
    radio.write(&Data, sizeof(struct EncoderData));
    //mandare dato con stato
   
  }
}



/***********************************************************************************************************/
/***********************************************************************************************************/
/***********************************************************************************************************/
/***********************************************************************************************************/
/***********************************************************************************************************/


/* routine encoder, non toccare!*/
void updateEncoder() {
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b1101 /*|| sum == 0b0100 || sum == 0b0010 || sum == 0b1011*/) encoderValue ++; // modificato per non lavorare in quadratura
  if (sum == 0b1110 /*|| sum == 0b0111 || sum == 0b0001 || sum == 0b1000*/) encoderValue --; // modificato per non lavorare in quadratura

  lastEncoded = encoded; //store this value for next time
}

/* routine encoder, non toccare!*/

void debug1() {
#ifdef DEBUG
  Serial.println("DATI TRASMESSI");
  Serial.print("Data.valoreangolocorretto     ");
  Serial.println(Data.valoreangolocorretto);
  Serial.print("encoderValue    ");
  Serial.println(encoderValue);
  Serial.print("Data.offsetRequest      ");
  Serial.println(Data.offsetRequest);
  Serial.println("");
  Serial.println("");
#endif
}


void debug2() {
#ifdef DEBUG
  Serial.println("DATI RICEVUTI");
  Serial.print("Ack.AngoloLetto    ");
  Serial.println(Ack.ValOffset);
  Serial.print("Ack.isRestarted        ");
  Serial.println(Ack.isRestarted);
  Serial.println("");
  Serial.println("");
#endif
}

void debug3() {
#ifdef DEBUG
  Serial.println("DATI TRASMESSI");
  Serial.print("Data.valoreangolocorretto     ");
  Serial.println(Data.valoreangolocorretto);
   Serial.print("risoluzioneEmcoder     ");
  Serial.println(risoluzioneEncoder);
  Serial.print("encoderValue    ");
  Serial.println(encoderValue);
  Serial.print("Ack.ValOffset      ");
  Serial.println(Ack.ValOffset);
  Serial.print("Data.offsetRequest      ");
  Serial.println(Data.offsetRequest);
  Serial.println("DATI RICEVUTI");
  Serial.print("Ack.isRestarted        ");
  Serial.println(Ack.isRestarted);
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("");

#endif
}
