#include <EEPROM.h> // Include EEPROM library for high score storage
 
// pin num
const int buttonPin = 7;
const int buzzerPin = 6;
const int rgbPins[3] = {9, 10, 11}; // blue green red because of the fault of the hardware
const int soundSensorPin = A0;
const int ledPins[3] = {2, 3, 4};
 
// game status
enum GameState {
  IDLE,
  WAITING_TO_SHOW_COLOR,
  SHOW_COLOR
};
 
GameState currentState = IDLE;
 
// time
unsigned long lastStateChange = 0;
const unsigned long baseReactionTime = 2000; // level 1 base reaction time
unsigned long reactionTime = baseReactionTime; // current reaction time
 
const unsigned long baseColorDisplayTime = 1500; // color display time level 1
unsigned long currentColorDisplayTime = baseColorDisplayTime; // current color display time
 
const unsigned long colorDisplayReductionPerLevel = 200; // reduce the display time
const unsigned long reactionTimeReductionPerLevel = 100; // reduce the reaction time
const unsigned long minReactionTime = 200; // min reaction time
const unsigned long minColorDisplayTime = 200; // min color display time
 
String currentColor = "Unknown";
 
// react time
unsigned long clapTime = 0;
unsigned long userReactionTime = 0;
 
// score level
unsigned int score = 0;
unsigned int level = 1;
const unsigned int maxLevel = 10;
const unsigned int pointsPerLevel = 10; // point need to advance to next level
 
// high score
const int highScoreAddress = 0; // EEPROM address to store high score
unsigned int highScore = 0;
 
// boolean clap true or false
bool previousSoundState = false;
 
// check if the user clapped during Green
bool clappedDuringGreen = false;
 
// long press boolean
unsigned long buttonPressStartTime = 0;
bool buttonIsPressed = false;
bool longPressDetected = false;
const unsigned long longPressDuration = 5000; // 5000 ms = 5 seconds
 
void setup() {
  // init button
  pinMode(buttonPin, INPUT_PULLUP);
 
  // init rgb led pins
  for(int i = 0; i < 3; i++) {
    pinMode(rgbPins[i], OUTPUT);
    digitalWrite(rgbPins[i], LOW);
  }
 
  // init leds
  for(int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
 
  // init buzzer
  pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin); // buzzer off init
 
  // init serial
  Serial.begin(9600);
 
  // init random number generator
  randomSeed(analogRead(0));
 
  // EEPROM get high score
  EEPROM.get(highScoreAddress, highScore);
 
  //serial init messages show
  Serial.println("=====================================");
  Serial.println(" Welcome to the Reaction Game!");
  Serial.println("=====================================");
  Serial.println("Press the button to start.");
  Serial.print("Level: ");
  Serial.println(level);
  Serial.print("Score: ");
  Serial.println(score);
  Serial.print("High Score: ");
  Serial.println(highScore);
}
 
void loop() {
  unsigned long currentTime = millis();
 
  // ---- Long Press Detection Start ----
  // Check if button is pressed
  if (digitalRead(buttonPin) == LOW) { // Button is pressed
    if (!buttonIsPressed) { // New press detected
      buttonIsPressed = true;
      buttonPressStartTime = currentTime;
      longPressDetected = false;
    } else { // Button is being held down
      if (!longPressDetected && (currentTime - buttonPressStartTime >= longPressDuration)) {
        longPressDetected = true;
        Serial.println("\nLong button press detected. Resetting the game...");
        resetGame();
      }
    }
  } else { // Button is not pressed
    buttonIsPressed = false;
    longPressDetected = false;
  }
  // ---- Long Press Detection End ----
 
  // Check if button is pressed and game is idle (Short Press)
  if (digitalRead(buttonPin) == LOW && currentState == IDLE && !longPressDetected) {

    currentState = WAITING_TO_SHOW_COLOR;
    lastStateChange = currentTime;
    Serial.println("\nGame Started! Get ready...");
    Serial.print("Level: ");
    Serial.println(level);
    delay(500); // Debounce delay
  }
 
  switch(currentState) {
    case IDLE:
      // Do nothing, wait for button press
      break;
 
    case WAITING_TO_SHOW_COLOR:
      // Turn off RGB LED during reaction time
      turnOffRGB();
      if (currentTime - lastStateChange >= reactionTime) {
        setRandomColor();
        lastStateChange = currentTime;
        currentState = SHOW_COLOR;
      }
      break;
 
    case SHOW_COLOR:
      // RGB LED is already on with the set color
      if (currentTime - lastStateChange >= currentColorDisplayTime) {
        // Time to hide the color and handle post-display logic
        if (currentColor == "Red" || currentColor == "Blue") {
          givePassivePoints();
        } else if (currentColor == "Green" && !clappedDuringGreen) {
          // User did not clap during Green color
          gameOver();
          break; // Exit the case to prevent further processing
        }
        turnOffRGB();
        currentColor = "Unknown";
        lastStateChange = currentTime;
        currentState = WAITING_TO_SHOW_COLOR;
        Serial.println("\nTime's up! Next round...");
      }
 
      // Detect clap using rising edge
      int soundValue = analogRead(soundSensorPin);
      bool currentSoundState = soundValue > 500; // Threshold
 
      if (currentSoundState && !previousSoundState) {
        // Clap detected
        clapTime = currentTime;
        userReactionTime = clapTime - lastStateChange;
        Serial.println("\nClap detected.");
        Serial.println("Reaction time: " + String(userReactionTime) + " ms");
 
        if (currentColor == "Green") {
          // User clapped during Green color
          clappedDuringGreen = true; // Set the flag
          // Correct clap, give reward based on reaction time
          giveReward(userReactionTime);
 
          // Turn off RGB LED
          turnOffRGB();
 
          // Set a new state to wait before showing the next color
          lastStateChange = currentTime;
          currentState = WAITING_TO_SHOW_COLOR;
        }
        else if (currentColor == "Red" || currentColor == "Blue") {
          // Incorrect clap, end game
          gameOver();
        }
        else {
          // Optional: Handle claps when color is unknown or other colors
          Serial.println("Clap detected, but no valid color is active.");
        }
        // Prevent multiple detections within the same color display period
        delay(1000);
      }
      previousSoundState = currentSoundState;
      break;
  }
}
 
// Function to set a random color for the RGB LED
void setRandomColor() {
  // Turn off all colors first
  turnOffRGB();
 
  // Randomly choose a color
  int color = random(0, 3); // 0: Blue, 1: Green, 2: Red
  switch(color) {
    case 0:
      digitalWrite(rgbPins[0], HIGH); // Blue
      currentColor = "Blue";
      Serial.println("Color set to Blue");
      break;
    case 1:
      digitalWrite(rgbPins[1], HIGH); // Green
      currentColor = "Green";
      clappedDuringGreen = false; // Initialize flag for Green
      Serial.println("Color set to Green");
      break;
    case 2:
      digitalWrite(rgbPins[2], HIGH); // Red
      currentColor = "Red";
      Serial.println("Color set to Red");
      break;
  }
}
 
// Function to give a reward by lighting up LEDs based on reaction time
void giveReward(unsigned long reactionTimeUser) {
  Serial.println("Giving reward based on reaction time.");
 
  if (reactionTimeUser < 1000) { // Fast reaction
    Serial.println("Fast reaction! Lighting up 3 LEDs.");
    // all led on
    for(int i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], HIGH);
      Serial.print("LED ");
      Serial.print(ledPins[i]);
      Serial.println(" turned ON.");
      delay(100); // Optional: slight delay between LEDs
    }
    delay(500); // Duration to keep LEDs on
    for(int i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], LOW);
      Serial.print("LED ");
      Serial.print(ledPins[i]);
      Serial.println(" turned OFF.");
      delay(100); // Optional: slight delay between LEDs
    }
    score += 3;
    Serial.println("Score +3. Total Score: " + String(score));
  }
  else if (reactionTimeUser < 2000) { // Medium reaction
    Serial.println("Medium reaction! Lighting up 2 LEDs.");
    // Light up first 2 LEDs
    for(int i = 0; i < 2; i++) {
      digitalWrite(ledPins[i], HIGH);
      Serial.print("LED ");
      Serial.print(ledPins[i]);
      Serial.println(" turned ON.");
      delay(100); // Optional: slight delay between LEDs
    }
    delay(500); // Duration to keep LEDs on
    for(int i = 0; i < 2; i++) {
      digitalWrite(ledPins[i], LOW);
      Serial.print("LED ");
      Serial.print(ledPins[i]);
      Serial.println(" turned OFF.");
      delay(100); // Optional: slight delay between LEDs
    }
    score += 2;
    Serial.println("Score +2. Total Score: " + String(score));
  }
  else { // Slow reaction
    Serial.println("Slow reaction! Lighting up 1 LED.");
    // Light up first LED
    digitalWrite(ledPins[0], HIGH);
    Serial.print("LED ");
    Serial.print(ledPins[0]);
    Serial.println(" turned ON.");
    delay(500); // Duration to keep LED on
    digitalWrite(ledPins[0], LOW);
    Serial.print("LED ");
    Serial.print(ledPins[0]);
    Serial.println(" turned OFF.");
    score += 1;
    Serial.println("Score +1. Total Score: " + String(score));
  }
 
  // Check if it's time to advance to the next level
  if (score >= pointsPerLevel * level && level < maxLevel) {
    level++;
    updateLevel();
  }
}
 
// function to give point if no clap during red and blue
void givePassivePoints() {
  Serial.println("No clap detected during " + currentColor + ". Gaining 2 points.");
 
  // all led on
  for(int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
  delay(500); // led durations
  for(int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
  }
 
  score += 2;
  Serial.println("Passive Score +2. Total Score: " + String(score));
 
  // check if ok to advance to next level
  if (score >= pointsPerLevel * level && level < maxLevel) {
    level++;
    updateLevel();
  }
}
 
// update level and adjust reaction time and color display time
void updateLevel() {
  Serial.println("\nCongratulations! You've advanced to Level " + String(level) + "!");
 
  // Decrease reaction time to increase difficulty, but set a minimum limit
  reactionTime = baseReactionTime - (level - 1) * reactionTimeReductionPerLevel;
  if (reactionTime < minReactionTime) { // min reaction time
    reactionTime = minReactionTime;
  }
 
  // decrease color display time
  currentColorDisplayTime = baseColorDisplayTime - (level - 1) * colorDisplayReductionPerLevel;
  if (currentColorDisplayTime < minColorDisplayTime) { // min color display time
    currentColorDisplayTime = minColorDisplayTime;
  }
 
  Serial.print("New Reaction Time: ");
  Serial.println(reactionTime);
  Serial.print("New Color Display Time: ");
  Serial.println(currentColorDisplayTime);
 
  // level up feedback
  tone(buzzerPin, 1500, 300); // 1500 Hz for 300 ms
  delay(350);
  noTone(buzzerPin);
 
  Serial.print("Score: ");
  Serial.println(score);
  Serial.print("High Score: ");
  Serial.println(highScore);
}
 
// reset game
void resetGame() {
  // Reset game state and variables
  currentState = IDLE;
  turnOffRGB();
  score = 0;
  level = 1;
  reactionTime = baseReactionTime;
  currentColorDisplayTime = baseColorDisplayTime;
  clappedDuringGreen = false;
 
  // sound feedback
  tone(buzzerPin, 1200, 300); // 1200 Hz for 300 ms
  delay(350);
  noTone(buzzerPin);
 
  // reset messages (arduino output)
  Serial.println("\nGame has been reset. Press the button to play again.");
  Serial.print("Level: ");
  Serial.println(level);
  Serial.print("Score: ");
  Serial.println(score);
  Serial.print("High Score: ");
  Serial.println(highScore);
}
 
// end game then buzzer sound on 
void gameOver() {
  Serial.println("\nIncorrect clap! Game Over.");
  Serial.println("Final Score: " + String(score));
  Serial.println("Level Reached: " + String(level));
 
  // buzzer setup
  for(int i = 0; i < 3; i++) {
    tone(buzzerPin, 1000, 300); // 1000 Hz frequency for 300 ms
    delay(400);                   // Wait 
    noTone(buzzerPin);            // stop
  }
 
  // check and update high score
  if (score > highScore) {
    highScore = score;
    EEPROM.put(highScoreAddress, highScore); // Save new high score to EEPROM
    Serial.println("New High Score! " + String(highScore));
  }
 
  // reset game
  currentState = IDLE;
  turnOffRGB();
  score = 0;
  level = 1;
  reactionTime = baseReactionTime;
  currentColorDisplayTime = baseColorDisplayTime;
  clappedDuringGreen = false;
 
  Serial.println("\nGame has been reset. Press the button to play again.");
}
 
// off rgb color
void turnOffRGB() {
  for(int i = 0; i < 3; i++) {
    digitalWrite(rgbPins[i], LOW);
  }
}