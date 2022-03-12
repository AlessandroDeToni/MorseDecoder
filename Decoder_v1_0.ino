#include <U8g2lib.h> //Display library
#include <Wire.h>    //i2c library

// ===========================================================================================
//                                         VARIABLES
// ===========================================================================================

/*------------------------------------------DISPLAY-----------------------------------------*/
//Display constants
#define time_delay 500
//Display constructor
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
//contatore che viene incrementato per visualizzare i punti e le linee in sequenza sul display
int displayPosition = 0;

/*------------------------------------------BUTTON------------------------------------------*/
const int buttonPin = D6; //defines the name of the pin connected to the button
int buttonState = 0;  // value read from the button

/*------------------------------------------TIMING------------------------------------------*/
const int minDuration = 10;                              // Minimum length of a dot or gap. Anything less is a bounce or static
const int maxDotDuration = 500;                           // Maximum length of a dot. Minimum length of a dash
const int maxDashDuration = 1500;                         // The maximum length of a dash. Any more and it is considered stuck

/*const int minWordGap;                               // Anything less is considered a part of a word or character*/
const int minCharacterGap = 2000;                          // Anything less is considered part of a character
int gapStart = 0;
/*------------------------------------------INTERRUPT-----------------------------------------*/
// Variables for the interrupt
volatile bool stateNow = 1;                   // Current receive state - 0 = within a mark, 1 = within a gap
volatile unsigned long lastInterruptTime = 0; // Time of the last interupt in milliseconds
//volatile bool newDataFlag = 0;                // New mark received flag - 1 = there is new data to process
volatile bool lastMarkType = 0;               // Stores the last mark - 0 = dot, 1 = dash
volatile unsigned int lastMarkLength;         // Length of last mark in milliseconds

byte currentCharacter [5] = {0, 0, 0, 0, 0}; // Array of dots and dashes of the current received character
byte bufferPosition = 0;                      // The current bit/mark position received for the current character (currentCharacter array)
byte currentCharacterNumber;                  // Plain text character array number of current morse character
bool spaceFlag = 0;                          // A flag to control when to print a space 0 = no, 1= yes*/

/*------------------------------------------DECODIFICA-----------------------------------------*/

byte morseCode[][5] = {   //  1 = dot, 2 = dash, 0 = not used
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

char morsePlain[37] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S',
                       'T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9','?'};

char character;

void setup() {
  u8g2.begin();   //sets the display up
  u8g2_prepare(); //defines display configuration
  //Serial.begin(9600);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);

  // Attach an interrupt. Set to trigger on rising AND falling edges to catch beginning and end of marks
  attachInterrupt(digitalPinToInterrupt(buttonPin), morse_ISR, CHANGE);
}

void loop() {

  //Write to display:
  if (stateNow == 0) {
    //u8g2.clearBuffer();
    if (lastMarkType == 0) {
      currentCharacter[bufferPosition] = 1; //aggiorna il buffer per comunicare che l'i-esimo segnale è un punto
      u8g2.drawStr(displayPosition, 0, ".");
      u8g2.updateDisplay();
      displayPosition = displayPosition + 5;
      bufferPosition++;
      stateNow = 1;
    }
    else {
      currentCharacter[bufferPosition] = 2; //aggiorna il buffer per comunicare che l'i-esimo segnale è una linea
      u8g2.drawStr(displayPosition + 1, 0, "_");
      u8g2.updateDisplay();
      displayPosition = displayPosition + 5;
      bufferPosition++;
      stateNow = 1;
    }
  }

  if ((millis() - lastInterruptTime) > minCharacterGap && bufferPosition != 0) {
    decodesignal();
  }
}

// ===========================================================================================
//                                         FUNCTIONS
// ===========================================================================================

/*------------------------------------------DISPLAY-----------------------------------------*/
//Sets up the characteristics of the display (Font type, color, starting position etc.)
void u8g2_prepare() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

/*------------------------------------------INTERRUPT-----------------------------------------*/
//Handles the interrupt originated by the button so that we are able to write dot and dashes
ICACHE_RAM_ATTR void morse_ISR() { //ICACHE_RAM_ATTR needed to save the ISR in the internal RAM
  // This interrupt is triggered on all changes. It determines if a dot or dash has just been received.
  // Gaps/spaces are processed in other functions via the main loop.

  unsigned long interruptTime = millis();

  // Check if it is a bit of static/bounce
  if (interruptTime - lastInterruptTime >= minDuration) {

    // Are we starting or ending a dot/dash? - HIGH is off
    if (digitalRead(buttonPin) == LOW) { // We have just ended a dot or dash
      lastMarkLength = interruptTime - lastInterruptTime; // Get length in milliseconds

      // Was it a dot or a dash?
      if (lastMarkLength < maxDotDuration) {
        lastMarkType = 0;                 // It's a dot
      }
      else {
        lastMarkType = 1;                 // It's a dash
      }

      //newDataFlag = 1;                    // Flag there is a new mark to process
      stateNow = 0;                       // Set current state to nothing being currently sent
      //digitalWrite(STATUS_LED_PIN, LOW);  // turn the LED off
    }
    else { // We are just starting a dot/dash

      stateNow = 1; // Set current state to currently receving a mark
    }
  }

  lastInterruptTime = interruptTime;      // Remember interrupt time for next time ******** than here ********
}
/*------------------------------------------DECODIFICA-----------------------------------------*/
void decodesignal() {
  
  bool matchFound = 0; // Tracks if a known character is found
  // Loop through the known character codes and return the matching character or prosign if found
  for (int c = 0; c < 36; c++) {
    // There may be more efficient way to do this
    if (currentCharacter[0] == morseCode[c][0] &&
        currentCharacter[1] == morseCode[c][1] &&
        currentCharacter[2] == morseCode[c][2] &&
        currentCharacter[3] == morseCode[c][3] &&
        currentCharacter[4] == morseCode[c][4]){
      // It's a match
      currentCharacterNumber = c;  // Set number to match arrays
      matchFound = 1; // flag that a match has been found
      break;  // Break out of the loop
    }
  }

  if (matchFound != 1) {
    currentCharacterNumber = 37;
  }
  
  //u8g2.setCursor(0, 40);
  //u8g2.print(currentCharacterNumber);
  // Reset character array - Even if a match was not found, reset it
  //u8g2.setCursor(0, 40);
  for (int b = 0; b < 5; b++) {
    //u8g2.print(currentCharacter[b]);
    currentCharacter[b] = 0;
  }

  bufferPosition = 0; // Reset the buffer position back to 0
  displayPosition = 0; // Reset the display position back to 0
  character = morsePlain[currentCharacterNumber];
  u8g2.setCursor(0, 20);
  u8g2.print(character);
  //u8g2.setCursor(10, 20);
  //u8g2.print("ENTRATO");
  u8g2.updateDisplay();
  delay(3000);
  u8g2.clearDisplay();
  u8g2.home();
  

}
