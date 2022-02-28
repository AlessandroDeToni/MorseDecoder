

/*Visto che ci interessano solo numeri e lettere, possiamo rimuovere qualche riga e 
ridurre la matrice del code morse a 5 colonne come segue:*/

byte morseCode[][5]={     //  1 = dot, 2 = dash, 0 = not used
  {1,2,0,0,0},    //  [0]    A   .-
  {2,1,1,1,0},    //  [1]    B   -...
  {2,1,2,1,0},    //  [2]    C   -.-.
  {2,1,1,0,0},    //  [3]    D   -..
  {1,0,0,0,0},    //  [4]    E   .
  {1,1,2,1,0},    //  [5]    F   ..-.
  {2,2,1,0,0},    //  [6]    G   --.
  {1,1,1,1,0},    //  [7]    H   ....
  {1,1,0,0,0},    //  [8]    I   ..
  {1,2,2,2,0},    //  [9]    J   .---
  {2,1,2,0,0},    //  [10]   K   -.-
  {1,2,1,1,0},    //  [11]   L   .-..
  {2,2,0,0,0},    //  [12]   M   --
  {2,1,0,0,0},    //  [13]   N   -.
  {2,2,2,0,0},    //  [14]   O   ---
  {1,2,2,1,0},    //  [15]   P   .--.
  {2,2,1,2,0},    //  [16]   Q   --.-
  {1,2,1,0,0},    //  [17]   R   .-.
  {1,1,1,0,0},    //  [18]   S   ...
  {2,0,0,0,0},    //  [19]   T   -
  {1,1,2,0,0},    //  [20]   U   ..-
  {1,1,1,2,0},    //  [21]   V   ...-
  {1,2,2,0,0},    //  [22]   W   .--
  {2,1,1,2,0},    //  [23]   X   -..-
  {2,1,2,2,0},    //  [24]   Y   -.--
  {2,2,1,1,0},    //  [25]   Z   --..
  {2,2,2,2,2},    //  [26]   0   -----
  {1,2,2,2,2},    //  [27]   1   .----
  {1,1,2,2,2},    //  [28]   2   ..---
  {1,1,1,2,2},    //  [29]   3   ...--
  {1,1,1,1,2},    //  [30]   4   ....-
  {1,1,1,1,1},    //  [31]   5   .....
  {2,1,1,1,1},    //  [32]   6   -....
  {2,2,1,1,1},    //  [33]   7   --...
  {2,2,2,1,1},    //  [34]   8   ---..
  {2,2,2,2,1},    //  [35]   9   ----.
  };

void setup() {

  // Map input and output pins
  pinMode(INPUT_PIN, INPUT);            // Set pin as input for the morse input pin
  pinMode(STATUS_LED_PIN, OUTPUT);      // Set pin as output for the status LED
  pinMode(SPEED_POT_PIN, INPUT);        // Set pin as input for the analog speed pot
  pinMode(AUTO_MODE_PIN, INPUT_PULLUP); // Set pin as input for the auto/manual mode switch

  lastMode = digitalRead(AUTO_MODE_PIN); // Set the mode;
  
  // Attach an interrupt. Set to trigger on rising AND falling edges to catch beginning and end of marks
  attachInterrupt(digitalPinToInterrupt(INPUT_PIN), morse_ISR, CHANGE);

  // zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
  // ******** Initialise LCD - Use method for your library and connection ********
  // For HD44780 library (with a backpack?) **************
  //lcd.begin(LCD_COLS, LCD_ROWS);
  
  // Initialise the LCD and turn on backlight for LiquidCrystal library
  // lcd.begin();        // Use with I2C backpack
  // lcd.backlight();    // Use with I2C backpack 

  // For LiquidCrystal and HD44780 libraries without a backpack
  lcd.begin(LCD_COLS, LCD_ROWS);   // Use when not using a backpack
  // zzzzzzzzzzzzzzzzzzzzzzzzzzzzz End of LCD initialisation zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz

  // Create custom LCD characters (.- and space) for morse
  lcd.createChar(0, space);
  lcd.createChar(1, dot);
  lcd.createChar(2, dash);
  
  welcomeScreen();  // Display the welcome screen

  currentMode = digitalRead(AUTO_MODE_PIN);
  setTiming(); // Check and update timing values. As dotDuration and lastDotDuration are set differently all
  //displaySetSpeed(0); // Update display
  //lastDotDuration = map(analogRead(SPEED_POT_PIN), 0, 1023, 20, 150);  // Set it to the value from the pot - May not need this
}

void loop() {

    // Get the value and map it to a useful range 20 - 150
    dotDuration = map(analogRead(SPEED_POT_PIN), 0, 1023, 20, 150);  

    // If the speed has changed update timing values
    if (dotDuration != lastDotDuration) { 
      setTiming();
    }
  }

  // Check for dodgy data
  //validationCheck();
    
  // Check if enough time has elapsed after a dot or dash for a character or word to end.
  if (stateNow == 0) {            // If nothing is currently being sent
    dotsAndDashes();              // Check if there are any new dots or dashes and process
    endOfCharacter();             // If enough time has elapsed to end a character and there is something in the buffer, convert and display it
    endOfWord();                  // Print a space if enough time has elapsed and one has not already been printed
  }
}

// -------------------------------------------------------------------------------------------
//                                 Funzioni per decodifica e stampa a video
// -------------------------------------------------------------------------------------------

void setTiming() {

  // Check if speed has changed. If it has recalculate timing values and update set speed
  // The length of dashes and gaps are set here as a multiple of the dot length
  // We are not using the exact morse specs to allow for variations in operator performance

  // Official morse specifications:
  // -----------------------------
  //  Dashes = 3 x dot length
  //  Gaps between dots and dashes within a character = 1 x dot length
  //  Gaps between characters = 3 x dot length
  //  Gaps between words = 7 x dot length

  // Minimum duration - This method sets it to a ratio of the dot length, but doesn't track so well in auto mode
  /*minDuration = 0.5 * dotDuration;        // Minimum length of a dot or gap. Anything less is a bounce or static
  if (minDuration < 10) {
    minDuration = 10;                     // Ensure minimum duration always at least 10 milliseconds
  }*/
  minDuration = 10;                       // Alternatively, just a minimum length of a dot or gap in millis.
  
  maxDotDuration = 1.7 * dotDuration;     // Maximum length of a dot. Minimum length of a dash  2
  maxDashDuration = 15 * dotDuration;     // The maximum length of a dash. Any more and it is considered stuck  12
  minWordGap = 5.5 * dotDuration;         // Anything less is considered a part of a word 5
  minCharacterGap = 1.8 * dotDuration;    // Anything less is considered part of a character  2

  displaySetSpeed(dotDuration);           // Update speed on display

  lastDotDuration = dotDuration;          // Store current value for next check 
}



void dotsAndDashes() {
  // Print any dots or dashes that have not already been displayed and update buffer
  
  if (newDataFlag == 1) {

    // Record and display dots and dashes
    if (lastMarkType == 0) {                    // It's a dot
      displayMark(1);                           // display dot
      currentCharacter[bufferPosition] = 1;     // Add it to the current character array
      displayActualSpeed(lastMarkLength);       // Display actual length of dot
    } 
    else {                                      // It's a dash
      displayMark(2);                           // display dash
      currentCharacter[bufferPosition] = 2;     //  Add it to the current character array      
      //lastMarkLength = lastMarkLength / 2.8;    // If it is a dash, divide by 2.8. Theoretically it should be 3, but this works better
    }

    //displayActualSpeed(lastMarkLength);         // Display last mark length, divided by three for dashes
    bufferPosition++;                           // Update buffer position ready for next mark
    newDataFlag = 0;                            // Reset back to zero until next mark received

    // If it is in auto mode, update dotDuration and speed
    if (currentMode == 1) {                     // If auto mode on
      if (lastMarkType == 1) {                  // Dashes
        // Using dash lengths helps speed adjustment when their is an immediate changes in speed
        lastMarkLength = lastMarkLength / 3;    // If it is a dash, divide by 3.
      }
      dotDuration = ((smoothingFactor * lastDotDuration) + ((10 - smoothingFactor) * lastMarkLength)) / 10; // Smooth timing
      setTiming();                              // Update other timing values
    }
  }
}

void endOfCharacter() { 
  // If enough time has elapsed to end a character and there is something in the buffer, work
  // out what it is and print it

  bool matchFound = 0; // Tracks if a known character is found  

  // Check if enough time has elapsed and there is something in the buffer
  if ((millis() - lastInterruptTime) > minCharacterGap && bufferPosition != 0) {

    // Loop through the known character codes and return the matching character or prosign if found
    for (byte c = 0; c < 36; c++) {
      // There may be more efficient way to do this
      if (currentCharacter[0] == morseCode[c][0] &&
        currentCharacter[1] == morseCode[c][1] &&
        currentCharacter[2] == morseCode[c][2] &&
        currentCharacter[3] == morseCode[c][3] &&
        currentCharacter[4] == morseCode[c][4] &&
        currentCharacter[5] == morseCode[c][5]) {
          // It's a match
          currentCharacterNumber = c;  // Set number to match arrays
          matchFound = 1; // flag that a match has been found
          break;  // Break out of the loop
      }
    }

    if (matchFound != 1) {
      // If the character was not found, use the question mark character and display that instead
      currentCharacterNumber = 46;
    }

    // Process character 
      updateTextDisplay(currentCharacterNumber); // Display it as a character
      displayMark(0);   // Add a space to the morse line
      spaceFlag = 1;    // Set the space flag so a space will be printed later if there is a sufficient gap

    // Reset character array - Even if a match was not found, reset it
    for (byte b=0; b<5; b++) {
      currentCharacter[b] = 0;
    }
    
    bufferPosition = 0; // Reset the buffer position back to 0  
  }
}

void endOfWord() {
  // Print a space if enough time has elapsed and one has not already been printed
  
  if ((millis() - lastInterruptTime) > minWordGap && spaceFlag == 1) {
    
    // Add a space to the text line
    updateTextDisplay(99);  // 99 is the magic space number

    // Add a space to the morse line - There will now be two; one for end of character and one for end of word
    displayMark(0);

    spaceFlag = 0;          // Reset flag so spaces are not continuously printed
  } 
}

void welcomeScreen() {
  // Display a welcome message on the display on power up

  // Set cursor to top left
  lcd.home();

  lcd.setCursor(0, 0);
  lcd.print(F(""));                 // Text for top row
  lcd.setCursor(0, 1);
  lcd.print(F("     Morse Code"));  // Text for second row
  lcd.setCursor(0, 2);
  lcd.print(F("      Decoder")); // Text for third row
  lcd.setCursor(0, 1);
  lcd.print(F(""));                 // Text for last row

  delay (1000);                     // Show for a while

  lcd.clear();                      // Clear the screen ready for the real stuff
}

void updateTextDisplay (byte charToDisplay) {
  // Print characters one at a time on one line from left to right. When the line is full print 
  // a copy of the line on the line above. Clear the  line and continue printing on it from
  // left to right until it is once again full.
  // 99 is a space. All other numbers refer to the morse characters

  String currentCharacter;    // Stores each part of the plain text. Used to support prosigns that have multiple characters

  // Check if it is a space or something else and set currentCharacter value
  if (charToDisplay == 99) {                // It's a space
    currentCharacter = " ";
  } 
  else {                                    // It's something else
    currentCharacter = morsePlain[charToDisplay];  // Get the string of the current character number
  }

  // Prosigns have multiple characters so loop through string
  for (int a = 0; a < currentCharacter.length(); a++) { 

    // Check if we are at the end of the row or we've hit a new line character.
    // If so move the text up, clear the line and start again
    if (lastCharLoc > 19) {
      // Update the top line of text
      lcd.setCursor(0, 1);
      lcd.print(completeLineText);          // Display text from the scrolling line
      completeLineText = "";                // Reset scrolling line text back to empty
      lastCharLoc = 0;                      // Reset scrolling line cursor to first position
  
      // Clear the scrolling line
      lcd.setCursor(0, 2);                  // Select the scrolling line
      lcd.print(F("                    ")); // Clear the line - 20 spaces
      lcd.setCursor(0, 2);                  // Set cursor back to the start of the line
    }
    
    // Set the cursor to the current location on the scrolling line
    lcd.setCursor(lastCharLoc, 2);
    
    lcd.print(currentCharacter[a]);         // Print this character
    completeLineText = completeLineText + currentCharacter[a]; // Add latest character to the line string
    
    lastCharLoc++;                          // Remember where we are on the line
  }
}

void displayMark(char markToDisplay) {
  // Display dots, dashes and spaces on bottom line of the display.
  // Start at the right hand side of the line and scroll left.

  // Move everything in the buffer along 1 place
  for (byte b=0; b<19; b++) {
    morseDisplayBuffer[b] = morseDisplayBuffer[b+1];
  }
  
  // Add the new character to the end of the array
  morseDisplayBuffer[19] = markToDisplay;

  // Update display
  for (byte d=0; d<20; d++) {
    
    lcd.setCursor(d, 3); // Set cursor to correct place on the last line
    
    // Display latest dot, dash or space
    lcd.write(morseDisplayBuffer[d]);
  }
}

// ===========================================================================================
//                                         INTERRUPT
// ===========================================================================================

void morse_ISR() {
  // This interrupt is triggered on all changes. It determines if a dot or dash has just been received. 
  // Gaps/spaces are processed in other functions via the main loop.

  unsigned long interruptTime = millis();

  // Check if it is a bit of static/bounce
  if (interruptTime - lastInterruptTime >= minDuration) {
 
    // Are we starting or ending a dot/dash? - HIGH is off
    if (digitalRead(INPUT_PIN) == HIGH) { // We have just ended a dot or dash
  
      lastMarkLength = interruptTime - lastInterruptTime; // Get length in milliseconds
      
      // Was it a dot or a dash?
      if (lastMarkLength < maxDotDuration) { 
        lastMarkType = 0;                 // It's a dot
      }
      else { 
        lastMarkType = 1;                 // It's a dash
      }  
         
      newDataFlag = 1;                    // Flag there is a new mark to process
      stateNow = 0;                       // Set current state to nothing being currently sent
      digitalWrite(STATUS_LED_PIN, LOW);  // turn the LED off     
    }
    else { // We are just starting a dot/dash
      stateNow = 1; // Set current state to currently recieving a mark
      digitalWrite(STATUS_LED_PIN, HIGH); // turn the LED on
    }
    //lastInterruptTime = interruptTime;    // Remember interrupt time for next time ******** Seems to work better with inteference here ********
  }
  
  lastInterruptTime = interruptTime;      // Remember interrupt time for next time ******** than here ********
}