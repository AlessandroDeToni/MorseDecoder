#include <U8g2lib.h> //Display library
#include <Wire.h>    //i2c library

// ===========================================================================================
//                                         VARIABLES
// ===========================================================================================

/*------------------------------------------DISPLAY-----------------------------------------*/
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); //Costruttore display
int upperDisplayPosition = 0;                                    //contatore per visualizzare i punti e le linee in sequenza sul display
int lowerDisplayPosition = 0;                                    //contatore per visualizzare le lettere in sequenza sul display

/*------------------------------------------BUTTON------------------------------------------*/
const int buttonPin = D6; //Nom
int buttonState = 0;  //Stato del bottone 1=premuto 0=rilasciato

/*------------------------------------------TIMING------------------------------------------*/
const int minDuration = 10;                               // Minima durata di un punto. In questo modo attuiamo il debouncing del bottone
const int maxDotDuration = 250;                           // Massima durata di un punto. Minima durata di una linea
const int maxDashDuration = 1000;                         // Massima durata di una linea

const int minCharacterGap = 2000;                         // Tempi maggiori di questa quantità significano che la lettera può essere decodificata
/*------------------------------------------INTERRUPT-----------------------------------------*/
volatile bool receivedSomething = 0;                      // Definisce in che stato è la ricezione: 0 = non ho ricevuto nulla, 1 = ho ricevuto un punto o linea
                                                          // che il pulsante venga premuto
volatile unsigned long lastInterruptTime = 0;             // Istante relativo all'ultimo interrupt in mS
volatile bool lastMarkType = 0;                           // Ricorda l'ultimo segnale: 0 = punto, 1 = linea
volatile unsigned int lastMarkLength;                     // Lunghezza dell'ultimo segnale in mS

byte currentCharacter [5] = {0, 0, 0, 0, 0};              // Ricorda gli ultimi 5 segnali ricevuti: 1 = punto, 2 = linea, 0 = non utilizzato
byte bufferPosition = 0;                                  // Posizione di currentCharacter
byte currentCharacterNumber;                              // Numero utile alla decodifica del carattere inviato. Viene utilizzato in morsePlain
                                                          // per selezionare la lettera corrispondente

/*------------------------------------------DECODIFICA-----------------------------------------*/

byte morseCode[][5] = {   //  1 = punto, 2 = linea, 0 = non utilizzato
  {1, 2, 0, 0, 0}, //  [0]    A   .-
  {2, 1, 1, 1, 0}, //  [1]    B   -...
  {2, 1, 2, 1, 0}, //  [2]    C   -.-.
  {2, 1, 1, 0, 0}, //  [3]    D   -..
  {1, 0, 0, 0, 0}, //  [4]    E   .
  {1, 1, 2, 1, 0}, //  [5]    F   ..-.
  {2, 2, 1, 0, 0}, //  [6]    G   --.
  {1, 1, 1, 1, 0}, //  [7]    H   ....
  {1, 1, 0, 0, 0}, //  [8]    I   ..
  {1, 2, 2, 2, 0}, //  [9]    J   .---
  {2, 1, 2, 0, 0}, //  [10]   K   -.-
  {1, 2, 1, 1, 0}, //  [11]   L   .-..
  {2, 2, 0, 0, 0}, //  [12]   M   --
  {2, 1, 0, 0, 0}, //  [13]   N   -.
  {2, 2, 2, 0, 0}, //  [14]   O   ---
  {1, 2, 2, 1, 0}, //  [15]   P   .--.
  {2, 2, 1, 2, 0}, //  [16]   Q   --.-
  {1, 2, 1, 0, 0}, //  [17]   R   .-.
  {1, 1, 1, 0, 0}, //  [18]   S   ...
  {2, 0, 0, 0, 0}, //  [19]   T   -
  {1, 1, 2, 0, 0}, //  [20]   U   ..-
  {1, 1, 1, 2, 0}, //  [21]   V   ...-
  {1, 2, 2, 0, 0}, //  [22]   W   .--
  {2, 1, 1, 2, 0}, //  [23]   X   -..-
  {2, 1, 2, 2, 0}, //  [24]   Y   -.--
  {2, 2, 1, 1, 0}, //  [25]   Z   --..
  {2, 2, 2, 2, 2}, //  [26]   0   -----
  {1, 2, 2, 2, 2}, //  [27]   1   .----
  {1, 1, 2, 2, 2}, //  [28]   2   ..---
  {1, 1, 1, 2, 2}, //  [29]   3   ...--
  {1, 1, 1, 1, 2}, //  [30]   4   ....-
  {1, 1, 1, 1, 1}, //  [31]   5   .....
  {2, 1, 1, 1, 1}, //  [32]   6   -....
  {2, 2, 1, 1, 1}, //  [33]   7   --...
  {2, 2, 2, 1, 1}, //  [34]   8   ---..
  {2, 2, 2, 2, 1}, //  [35]   9   ----.
};

char morsePlain[37] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S', //alfabeto per decodifica
                       'T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9','?'};

void setup() {
 
  u8g2.begin();   //inizializza il display
  u8g2_prepare(); //definisce le caratteristiche del display (per es. il font)
  
  pinMode(buttonPin, INPUT); // inizializza il pin per la lettura del bottone (D6) come INPUT

  attachInterrupt(digitalPinToInterrupt(buttonPin), morse_ISR, CHANGE); //Definisce morse_ISR come funzione risolutrice di INTERRUPT. 
                                                                        //Viene lanciato un interrupt ad ogni volta che su D6 vi è una variazione
}

void loop() {

  //Scrivi sul display un punto o una linea:
  if (receivedSomething == 1) { 
    if (lastMarkType == 0) {
      currentCharacter[bufferPosition] = 1; //aggiorna il buffer per comunicare che l'i-esimo segnale è un punto
      u8g2.drawStr(upperDisplayPosition, 0, "."); //inserisce nel buffer interno del display un punto in posizione (upperDisplayPosition, 0)
      u8g2.updateDisplay();
      upperDisplayPosition = upperDisplayPosition + 10; //Sposta la posizione di 10 pixel 
      bufferPosition++;
      receivedSomething = 0;
    }
    else {
      currentCharacter[bufferPosition] = 2; //aggiorna il buffer per comunicare che l'i-esimo segnale è una linea
      u8g2.drawStr(upperDisplayPosition + 1, 0, "-"); //inserisce nel buffer interno del display una linea in posizione (upperDisplayPosition, 0)
      u8g2.updateDisplay();
      upperDisplayPosition = upperDisplayPosition + 14; //Sposta la posizione di 14 pixel 
      bufferPosition++;
      receivedSomething = 0;
    }
  }

  //Se sono passati più di minCharacterGap mS da quando il tasto è stato premuto, la serie di punti e linee viene decodificata
  if ((millis() - lastInterruptTime) > minCharacterGap && bufferPosition != 0) {
    decodesignal();
  }
}

// ===========================================================================================
//                                         FUNCTIONS
// ===========================================================================================

/*------------------------------------------DISPLAY-----------------------------------------*/
//Setta le caratteristiche del display (Font, colore, posizione di inzio ecc.)
void u8g2_prepare() {
  u8g2.setFont(u8g2_font_courB24_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

/*------------------------------------------INTERRUPT-----------------------------------------*/
//Risponde all'interrupt generato dal bottone e comunica se si tratta di un punto o di una linea
ICACHE_RAM_ATTR void morse_ISR() { //ICACHE_RAM_ATTR necessaro per salvere la ISR nella ram interna (IRAM)
  
  unsigned long interruptTime = millis();

  // Controllo che sia effettivamente una pressione o sia un interrupt dovuto al bouncing del bottone
  
  if (interruptTime - lastInterruptTime >= minDuration) {

    // Controlla se il bottone è stato rilasciato, da questo capiamo l'istante da cui calcolare la durata del segnale
    if (digitalRead(buttonPin) == LOW) {  //Il punto o la linea è appena terminato
      lastMarkLength = interruptTime - lastInterruptTime; // Calcolo della durata del segnale

      // E' un punto o una linea?
      if (lastMarkLength < maxDotDuration) {
        lastMarkType = 0;                 // E' un punto
      }
      else {
        lastMarkType = 1;                 // E' una linea
      }
      receivedSomething = 1;              // Un punto o una linea è stato ricevuto, può essere messo a display
    }
    else {                                // Stiamo iniziando un punto o una linea
      receivedSomething = 0;              // Non è stato ricevuto nulla
    }
  }
  lastInterruptTime = interruptTime;      // Ricorda l'ultimo instante in cui è stato lanciato un interrupt
}
/*------------------------------------------DECODIFICA-----------------------------------------*/
void decodesignal() {
  
  bool matchFound = 0; // Ricorda se è stata trovata una corrispondenza tra i segnali e un carattere
  
  //Cicla tra le righe della tabella contenente il codice morse, se trova 
  //una corrispondenza salva il numero della riga in CurrentCharacterNumber
  for (int c = 0; c < 36; c++) {
    // There may be more efficient way to do this
    if (currentCharacter[0] == morseCode[c][0] &&
        currentCharacter[1] == morseCode[c][1] &&
        currentCharacter[2] == morseCode[c][2] &&
        currentCharacter[3] == morseCode[c][3] &&
        currentCharacter[4] == morseCode[c][4]){
      // Corrispondenza trovata
      currentCharacterNumber = c;  
      matchFound = 1; 
      break;  // Esce dal loop
    }
  }
  
  if (matchFound != 1) {   // Corrispondenza non trovata
    currentCharacterNumber = 37;
  }
  
  // Reinizializza il buffer di segnali 
  for (int b = 0; b < 5; b++) {
    currentCharacter[b] = 0;
  }

  // Scrittura del carattere
  bufferPosition = 0;                               // Resetta la posizione del buffer di segnali
  upperDisplayPosition = 0;                         // Resetta la posizione della parte superiore del display
  u8g2.setCursor(lowerDisplayPosition, 30);         // Setta la posizione bassa del display in cui print dovrà scrivere
  u8g2.print(morsePlain[currentCharacterNumber]);   // scrittura della lettera decodificata
  u8g2.updateDisplay();

  // Pulizia della parte alta del display
  u8g2.setDrawColor(0);                             // 0 significa: "cancella pixel"
  u8g2.drawBox(0,0,128,29);                         // Disegno di una casella che cancella tutta la parte superiore dello schermo
  u8g2.updateDisplay();
  
  u8g2.setDrawColor(1);                             // Ripristino del display in modalità scrittura
  lowerDisplayPosition = lowerDisplayPosition + 20; // Aggiornamento della posizione bassa di scrittura
}
