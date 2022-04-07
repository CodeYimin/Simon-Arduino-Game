#include <LiquidCrystal.h>

/***************************************
* CORE CONSTANTS
***************************************/
const int NUM_LEDS = 4;

/* The index of each analog value in the array
corresponds to the index of the button pressed. 
Eg. Analog input of 682 means button 2 was pressed */
const int BUTTON_ANALOG_VALS[NUM_LEDS] = { 512, 682, 767, 818 };

// Delay in ms between each poll for button state
const int BUTTON_POLLING_INTERVAL = 10;

/***************************************
* PINS CONSTANTS
***************************************/
const int LED_PINS[NUM_LEDS] = { 2, 3, 4, 5 };
const int PIEZO_PIN = 6;
const int BUTTONS_PIN = A0;
const int FLOATING_ANALOG_PIN = A1;

/***************************************
* GAME SETTING CONSTANTS
***************************************/

// The default tone played when corresponding button is pressed
const int TONES[NUM_LEDS] = { 330, 277, 440, 165 };

// The number of LED/Piezo flashes on corresponding event
const int GAME_START_FLASHES = 3;
const int GAME_OVER_FLASHES = 4;

// The tones played during each LED flash of corresponding event
const int GAME_START_TONES[GAME_START_FLASHES] = { 440, 440, 660 };
const int GAME_OVER_TONES[GAME_OVER_FLASHES] = { 117, 110, 104, 98 };

// The preset speed difficulties
const int GAME_SPEEDS[3] = {1, 5, 10};

/***************************************
* GAME VARIABLES
***************************************/

// Arduino's sequence playing speed during its turn, 
// ranging from 1-10, inclusive
int speed = 5;

// Correct sequence that the arduino played
int* sequence = {};
int sequenceLen = -1;

/***************************************
* LCD SETUP
***************************************/
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

/***************************************
* ARDUINO INITIAL STARTUP FUNCTION
***************************************/
void setup()
{ 
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
  * OTHER INITIALIZATION
  ***************************************/
  // Use random noise from floating analog pin to set as seed
  randomSeed(analogRead(FLOATING_ANALOG_PIN));
  
  // Setup serial monitor for development
  Serial.begin(9600);
  
  // Initialize lcd display with 16 columns and 2 rows
  lcd.begin(16, 2);
  
  /***************************************
  * BEGIN GAME CYCLE
  ***************************************/
  while (true)
  {
    playGame();
    delay(1000);
  }
}

// Loop function is unused but is mandatory to be declared
void loop() {};

/***************************************
* BUTTON INPUT FUNCTIONS
***************************************/

// Lock program flow, until user presses and releases a button.
// The button pressed is returned, and callback functions can be passed in
// which are called when the button is initially pressed and when the button
// is released.
int awaitButtonInput(void (*onPress)(int), void (*onRelease)(int))
{
  // If button was already held down prior to calling this function,
  // wait for them to let go of that button, because this should not
  // count as a full button input.
  if (getButtonPressed() != -1)
  {
    awaitButtonRelease();
  }
  
  // Wait for button press
  int buttonPressed = awaitButtonPress();
  onPress(buttonPressed);

  // Wait for button release
  awaitButtonRelease();
  onRelease(buttonPressed);
  
  return buttonPressed;
}

// Lock program flow until a button is pressed. Which button was pressed is returned.
int awaitButtonPress()
{
  while (true)
  {
    int buttonPressed = getButtonPressed();
    if (buttonPressed != -1)
    {
      return buttonPressed;
    }

    delay(BUTTON_POLLING_INTERVAL);
  }
}

// Lock program flow until no buttons are pressed
void awaitButtonRelease()
{
  while (true)
  {
    if (getButtonPressed() == -1)
    {
      return;
    }

    delay(BUTTON_POLLING_INTERVAL);
  } 
}

// Determine which button is pressed (-1 if none is being pressed)
int getButtonPressed()
{
  int buttonInput = analogRead(BUTTONS_PIN);
  int buttonIndex = find(BUTTON_ANALOG_VALS, NUM_LEDS, buttonInput);
  
  return buttonIndex;
}

/***************************************
* GAME CYCLE FUNCTIONS
***************************************/

// Initialize the sequence, allow player to set difficulty level,
// and finally start the first level.
void playGame()
{ 
  // Alert player that game is starting
  lcd.clear();
  lcd.print("Game starting!!!");
  LEDLightShow(GAME_START_TONES, GAME_START_FLASHES);
  delay(1000);
  
  // Ask player to set difficulty
  selectDifficulty();
  
  // Reset/Initialize the sequence
  sequence = generateSequence(1, NUM_LEDS);
  sequenceLen = 1;
  
  // Avoid instantly starting right after selecting difficulty
  delay(1000);
  
  // Keep playing levels with increasing sequence lengths until player loses
  while (true)
  {
    bool levelCompleted = playLevel();
    if (!levelCompleted)
    {
      break;
    }
    
    // Extend the sequence by 1 extra random color
    extendSequence(random(0, NUM_LEDS));
    
    // Wait a bit before moving on to next level
    delay(1000);
  }
  
  // Game over screen
  endGame();
}

// Prompt player to select a difficulty and set the game's difficulty
// according to their input.
void selectDifficulty()
{
  // Alert player of what's going on
  lcd.clear();
  lcd.print("Select a");
  lcd.setCursor(0, 1);
  lcd.print("difficulty");
  delay(2000);
  lcd.clear();
  lcd.print("0=EASY 1=MED");
  lcd.setCursor(0, 1);
  lcd.print("2=HARD 3=AUTO");

  // Player Input
  int buttonPressed = awaitButtonInput(turnOnLED, turnOffLED);
  if (buttonPressed == 3)
  {
    // Auto Difficulty
    speed = random(1, 11);
  }
  else
  {
    // Preset Difficulty
    speed = GAME_SPEEDS[buttonPressed];
  } 
}

// Tell user that the game is over and display the correct sequence
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

// Arduino plays the sequence, and then user is asked to replicate it.
// If the user enters a part of the sequence wrong, false is returned.
// If the user replicates the entire sequence correctly, true is returned.
bool playLevel()
{
  // Tell player that it is the arduino's turn
  lcd.clear();
  lcd.print("Level ");
  lcd.print(sequenceLen);
  lcd.setCursor(0, 1);
  lcd.print("Simon's turn");
  
  // Arduino plays the sequence
  playSequence(1150 - speed * 100, 230 - speed * 20);
  
  // Tell player that it is their turn
  lcd.setCursor(0, 1);
  lcd.print("YOUR TURN!!!");
  
  // Allow player to enter their replication of the sequence.
  for (int i = 0; i < sequenceLen; i++)
  {
    int playerInput = awaitButtonInput(turnOnLED, turnOffLED);
    
    // If player's input doesn't match original sequence's
    if (playerInput != sequence[i])
    { 
      return false;
    }
  }
  
  return true;
}

/***************************************
* SEQUENCE FUNCTIONS
***************************************/

// Light up LEDs and piezo in the sequence described
// in the sequence array. The sequence array contains the order
// of which LEDs gets turned on. onTime is the amount of time
// an LED stays on before it gets turned off. delayTime is the 
// delay between each flash.
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
* ARRAY UTILITY FUNCTIONS
***************************************/

// Used for development
// Prints an array to the serial monitor in a readable format
void printArray(const int* array, const int length)
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
int find(const int* array, const int length, const int searchFor) 
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
