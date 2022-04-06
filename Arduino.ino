#include <LiquidCrystal.h>

/***************************************
* CORE CONSTANTS
***************************************/
const int NUM_LEDS = 4;

/* The index of each analog value in the array
corresponds to the index of the button pressed. 
Eg. Analog input of 682 means button 2 was pressed */
const int BUTTON_ANALOG_VALS[NUM_LEDS] = { 512, 682, 767, 818 };

/***************************************
* PINS CONSTANTS
***************************************/
const int LED_PINS[NUM_LEDS] = { 2, 3, 4, 5 };
const int PIEZO_PIN = 6;
const int BUTTONS_PIN = A0;

/***************************************
* GAME CONSTANTS
***************************************/

// The default tone played when corresponding button is pressed
const int TONES[NUM_LEDS] = { 330, 277, 440, 165 };

// The number of LED/Piezo flashes on game event
const int GAME_START_FLASHES = 3;
const int GAME_OVER_FLASHES = 4;

const int GAME_START_TONES[GAME_START_FLASHES] = { 440, 440, 660 };
const int GAME_OVER_TONES[GAME_OVER_FLASHES] = { 117, 110, 104, 98 };

const int GAME_SPEEDS[3] = {1, 5, 10};

/***************************************
* GAME VARIABLES (Constantly updating)
***************************************/

bool gameOn = false;

// Speed of sequence played, ranging from 1-10, inclusive
int speed = 5;

// Player States
bool playerSelecting = false;
bool playerTurn = false;
bool buttonHeld = false;

// Original sequence
int* sequence = {};
int sequenceLen = -1;

// User input attempt
int userInput = -1;
int userInputIndex = -1;

/***************************************
* LED SETUP
***************************************/
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

/***************************************
* ARDUINO INITIAL STARTUP FUNCTION
***************************************/
void setup()
{ 
  /***************************************
  * MISCELLANEOUS INITIALIZATION
  ***************************************/
  // Use random noise from floating analog pin to set as seed
  randomSeed(analogRead(A1));
  
  // Setup serial monitor
  Serial.begin(9600);
  
  // Initialize lcd display with 16 columns and 2 rows
  lcd.begin(16, 2);
  
  /***************************************
  * INITIALIZE INPUT & OUTPUT PINS
  ***************************************/
  pinMode(BUTTONS_PIN, INPUT);
  pinMode(PIEZO_PIN, OUTPUT);
  
  // LED pins
  for (int i = 0; i < NUM_LEDS; i++)
  {
    pinMode(LED_PINS[i], OUTPUT);
  }
  
  /***************************************
  * GAME CYCLE
  ***************************************/
  startGame();
}

/***************************************
* ARDUINO INFINITE LOOP FUNCTION
***************************************/
void loop()
{
  // Get the index of the button that was pressed.
  // -1 if no button is pressed.
  int buttonIndex = getButtonPressed();
  
  /***************************************
  * Button events
  ***************************************/
  
  // Button Press Event
  if (buttonIndex != -1 && !buttonHeld)
  {
    buttonHeld = true;
    onButtonPress(buttonIndex);
  }
  
  // Button Release Event
  if (buttonIndex == -1)
  {
   	if (buttonHeld)
    {
      buttonHeld = false;
      onButtonRelease(); 
    }
  }
  
  // Reduce the amount of times Arduino is updated per second
  // by waiting before next update.
  delay(100);
}

/***************************************
* Event handlers
***************************************/

void onButtonPress(int buttonIndex)
{
  if (playerTurn || playerSelecting)
  {
    // Light up the LED corresponding to the pressed button
    turnOnLED(buttonIndex);
  }
  
  if (playerSelecting)
  {
    // If player selected auto (random) difficulty
    if (buttonIndex == 3)
    {
      speed = random(1, 11);
    }
    // Player selected a set difficulty
    else 
    {
      speed = GAME_SPEEDS[buttonIndex];
    }
  }
  
  if (playerTurn)
  {
    // Save the user's input
    userInputIndex++; 
    userInput = buttonIndex;
  }
}

void onButtonRelease()
{ 
  if (playerTurn || playerSelecting)
  {
    turnOffLEDs();
  }
  
  if (playerSelecting)
  {
    // Player already selected mode, so he is no longer selecting
    playerSelecting = false;
    
    // Begin the first level
    delay(1000);
    beginLevel();
  }
  
  if (playerTurn)
  {
    // Handle edge case where user held down button
    // before their turn, and then released once the turn
    // started, which should ideally have no effect
    if (userInput == -1)
    {
      return; 
    }
    
    // On game over (user pressed a wrong button)
    if (userInput != sequence[userInputIndex])
    {
      playerTurn = false;

      endGame();
      delay(1000);

      // Restart the game
      startGame();
    }
    // On level complete
    else if (userInputIndex + 1 == sequenceLen)
    {
      playerTurn = false;

      delay(1000);
      nextLevel();
    }
  }
}

/***************************************
* GAME CYCLE FUNCTIONS
***************************************/
void startGame()
{ 
  // Initialize/reset the sequence variables
  sequenceLen = 1;
  sequence = generateSequence(sequenceLen, NUM_LEDS);
  
  // Alert player that game is starting
  lcd.clear();
  lcd.print("Game starting!!!");
  LEDLightShow(GAME_START_TONES, GAME_START_FLASHES);
  delay(1000);
  
  // Create difficulty selection menu
  lcd.clear();
  lcd.print("Select a");
  lcd.setCursor(0, 1);
  lcd.print("difficulty");
  delay(2000);
  
  lcd.clear();
  lcd.print("0=EASY 1=MED");
  lcd.setCursor(0, 1);
  lcd.print("2=HARD 3=AUTO");
  
  // Allow player to begin selecting game mode
  playerSelecting = true;
}

void endGame() 
{
  // Display to player that the game is over
  lcd.clear();
  lcd.print("Game over!!!");
  LEDLightShow(GAME_OVER_TONES, GAME_OVER_FLASHES);
  delay(1000);

  // Display to player the correct sequence
  lcd.clear();
  lcd.print("The correct");
  lcd.setCursor(0, 1);
  lcd.print("sequence was: ");
  playSequence(1000, 250);
}

void nextLevel()
{
  // Add an additional, random, step to the sequence
  extendSequence(random(0, NUM_LEDS));
  
  // Begin the new level with the extra step added
  beginLevel();
}

bool beginLevel()
{
  // Reset the user input state
  userInputIndex = -1;
  userInput = -1;
  
  // Display to player that it is the arduino's turn
  lcd.clear();
  lcd.print("Level ");
  lcd.print(sequenceLen);
  lcd.setCursor(0, 1);
  lcd.print("Simon's turn");
  
  // Arduino's turn to play the sequence
  playSequence(1150 - speed * 100, 230 - speed * 20);
  
  // Display to player that it is their turn to replicate 
  // the sequence
  lcd.setCursor(0, 1);
  lcd.print("YOUR TURN!!!");
  
  // Begin the player's turn to input sequence
  playerTurn = true;
}

// Light up LEDs and piezo in the sequence described
// in the sequence array. The sequence array contains the order
// of which LEDs gets turned on. onTime is the amount of time
// an LED stays on before it gets turned off. delayTime is the 
// delay between each flash
void playSequence(int onTime, int delayTime)
{
  for (int i = 0; i < sequenceLen; i++)
  {
    turnOnLED(sequence[i]);
    delay(onTime);
    
    turnOffLED(sequence[i]);
    delay(delayTime);
  }
}

// Generate a random sequence of a specified length
int* generateSequence(int length, int numLEDs)
{
  int* sequence = new int[length];
  for (int i = 0; i < length; i++) 
  {
    sequence[i] = random(0, numLEDs);
  }
  return sequence;
}

// Adds an element to the end of the sequence array
void extendSequence(int num)
{
  int* newSequence = new int[sequenceLen + 1];
  
  // Copy all previous values from original sequence to new sequence
  for (int i = 0; i < sequenceLen; i++)
  {
    newSequence[i] = sequence[i];
  }
  // Insert new value into last index
  newSequence[sequenceLen] = num;
  
  // Set the original array to the new array
  sequence = newSequence;
  sequenceLen++;
}

/***************************************
* LED UTILITY FUNCTIONS
***************************************/

// Flash all LEDs with a custom tone sequence.
// A designated tone is played each time the LEDs flash.
void LEDLightShow(const int* tones, const int tonesLength)
{
  for (int i = 0; i < tonesLength; i++)
  {
    turnOnLEDs(tones[i]);
    delay(500);
    turnOffLEDs();
    delay(500);
  }
}

// Turn off all LEDs
void turnOffLEDs()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    turnOffLED(i);
  }
}

// Turn on all LEDs with the default tone
void turnOnLEDs()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    turnOnLED(i);
  }
}

// Turn on all LEDs with a custom tone
void turnOnLEDs(int tone)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    turnOnLED(i, tone);
  }
}

// Turn on LED and play default tone assigned to it
void turnOnLED(int index)
{
  turnOnLED(index, TONES[index]);
}

// Turn on LED with custom tone
void turnOnLED(int index, int playTone)
{
  digitalWrite(LED_PINS[index], HIGH);
  tone(PIEZO_PIN, playTone);
}

// Turn off an LED by its index. Also turns off the piezo speaker.
void turnOffLED(int index)
{
  digitalWrite(LED_PINS[index], LOW);
  noTone(PIEZO_PIN);
}

/***************************************
* MISC FUNCTIONS
***************************************/

// Determine which button is pressed (-1 if none is being pressed)
int getButtonPressed()
{
  int buttonInput = analogRead(BUTTONS_PIN);
  int buttonIndex = find(BUTTON_ANALOG_VALS, NUM_LEDS, buttonInput);
  
  return buttonIndex;
}

/***************************************
* ARRAY UTILITY FUNCTIONS
***************************************/

// Used for development
// Prints an array to the serial monitor in a readable format
void printArray(int* array, int length)
{
  Serial.print("{");
  for (int i = 0; i < length; i++)
  {
    Serial.print(array[i]);
    Serial.print(",");
  }
  Serial.print("}");
}

// Find the index of the first occurence of an element
// in an array. Returns -1 if element is not found.
int find(const int* array, const int length, int searchFor) 
{
  for (int i = 0; i < length; i++) 
  {
    if (array[i] == searchFor) 
    {
      return i;
    }
  }
  return -1;
}
