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

/*------------------------------------------BUTTON------------------------------------------*/
const int buttonPin = D6; //defines the name of the pin connected to the button
int buttonState = 0;  // value read from the button

/*------------------------------------------TIMING------------------------------------------*/
const int minDuration = 50;                              // Minimum length of a dot or gap. Anything less is a bounce or static
const int maxDotDuration = 500;                           // Maximum length of a dot. Minimum length of a dash
const int maxDashDuration = 1500;                         // The maximum length of a dash. Any more and it is considered stuck

/*const int minWordGap;                               // Anything less is considered a part of a word or character
const int minCharacterGap;                          // Anything less is considered part of a character*/

/*------------------------------------------INTERRUPT-----------------------------------------*/
// Variables for the interrupt
volatile bool stateNow = 1;                   // Current receive state - 0 = within a mark, 1 = within a gap
volatile unsigned long lastInterruptTime = 0; // Time of the last interupt in milliseconds
//volatile bool newDataFlag = 0;                // New mark received flag - 1 = there is new data to process
volatile bool lastMarkType = 0;               // Stores the last mark - 0 = dot, 1 = dash
volatile unsigned int lastMarkLength;         // Length of last mark in milliseconds

/*byte currentCharacter [5] = {0,0,0,0,0}; // Array of dots and dashes of the current received character
byte bufferPosition = 0;                      // The current bit/mark position received for the current character (currentCharacter array)
byte currentCharacterNumber;                  // Plain text character array number of current morse character
bool spaceFlag = 0;                          // A flag to control when to print a space 0 = no, 1= yes*/

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
 if(stateNow == 0){
    u8g2.clearBuffer();
    if (lastMarkType == 0){
      u8g2.drawStr(0,0, ".");
      u8g2.sendBuffer();
      delay(time_delay);
      }
    else {
      u8g2.drawStr(0,0, "_");  
      u8g2.sendBuffer();
      delay(time_delay);
    }
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
      stateNow = 1; // Set current state to currently recieving a mark
      //digitalWrite(STATUS_LED_PIN, HIGH); // turn the LED on
    }
  }
  
  lastInterruptTime = interruptTime;      // Remember interrupt time for next time ******** than here ********
}
