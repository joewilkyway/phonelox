#include <Arduino.h>
#include <RotaryEncoder.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Servo.h>

/*/------\*/
/*| Pins |*/
/*\------/*/
#define PIN_IN1 3 //Rotary Encoder
#define PIN_IN2 2 //Rotary Encoder
#define PIN_BUTTON 4 //Rotary Encoder Button
#define PIN_SERVO 7 // Servo Pin
#define PIN_POTENTIOMETER A0
#define THRESHOLD 670

/*/------------------------------------\*/
/*| Definitions for the Rotary Encoder |*/
/*\------------------------------------/*/
RotaryEncoder *encoder = nullptr; // Pointer to the rotary encoder instance.

void checkPosition(){
  /* Called by ISR, updates 'encoder' object */
  encoder->tick(); // Calling tick() to checks the state of the rotary encoder
}

/*/----------------------------------\*/
/*| Definitions for the OLED Display |*/
/*\----------------------------------/*/
U8G2_SH1106_128X64_NONAME_1_4W_SW_SPI u8g2(/*rotation=*/U8G2_R0, /*clock=*/13, /*data=*/11, /*cs=*/10, /*dc=*/ 9 , /* reset=*/8);

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

/*/---------------------------------\*/
/*| Definitions for the servo motor |*/
/*\---------------------------------/*/
Servo servo;

struct menuItem
{
  // menuItem defines the position, text(value) and font to be displayed
  // Used by drawStrCentred()
  int16_t xOffset;		/* x offset from centre of screen */
  int16_t yOffset;		/* y offset from centre of screen */
  const char *value;  /* This is the text to be displayed */
  const uint8_t *font;  /* Pass U8g2lib fonts*/
};

menuItem menuItemTime[] =
{
  // This is the lock time that gets changed
  // Used by drawStrCentred()
  {-48, 12, "0", u8g2_font_helvR12_tf}, //Hours
  {0, 12, "0", u8g2_font_helvR12_tf}, //Minutes
  {48, 12, "0", u8g2_font_helvR12_tf} //Seconds
};

menuItem menuItemTimeLabel[] =
{
  // Lock time labels 'Hr','Min','s'
  // Used by drawStrCentred()
  {menuItemTime[0].xOffset + 14, 12, "Hr", u8g2_font_helvR08_tf},
  {menuItemTime[1].xOffset + 16, 12, "Min", u8g2_font_helvR08_tf},
  {menuItemTime[2].xOffset + 11, 12, "S", u8g2_font_helvR08_tf}
};

menuItem menuItemSLT = {0, -22, "Set Lock Time", u8g2_font_helvR12_tf}; //Set lock time text


menuItem menuItemSplashMessage[] = 
{
  /* This defines the positions and fonts for
     the humorous text used in showLockSplash() */
  {0, -10, "0", u8g2_font_helvR12_tf},
  {0, 10, "0", u8g2_font_helvR12_tf},
};


const char *messages[] = 
  {
    // Humorous messages:
    "Eating your phone...",
    "Reading your messages...",
    "Waiting...",
    "I am your self control",
    "Hacking the mainframe...",
    "Locked and loaded",
    "Are you watching?",
    "Reducing your screen time...",
    "Hopefully this works...",
    "Delicious silicon...",
    "La La La...",
    "Hold tight!",
    "New phone who dis"
  };

void setup() {
  Serial.begin(9600);
  /*/-------------------------------\*/
  /*| Initiating the Rotary Encoder |*/
  /*\-------------------------------/*/

  // Declaring the encoder object with the predefined pins
  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

  // register interrupt routine
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  
  pinMode(PIN_BUTTON,INPUT_PULLUP); // for the button

  // for the servo
  servo.attach(PIN_SERVO);
  servo.write(0);

  /*/-----------------------------\*/
  /*| Initiating the OLED Display |*/
  /*\-----------------------------/*/
  u8g2.begin();  
  // displaying a splash screen
  drawAlert("PhoneLox");
  delay(1000);

}

void drawStrCentred(struct menuItem *item) {
  /*Takes a menuItem as a parameter, and draws it centred
    on the screen.*/
  u8g2.setFont(item->font);
  u8g2.setFontPosCenter();
  int x = DISPLAY_WIDTH/2 - u8g2.getStrWidth(item->value)/2 + item->xOffset;
  int y = DISPLAY_HEIGHT/2 + item->yOffset;
  u8g2.drawStr(x, y, item->value);
}

void drawAlert(const char *string){
  /*Pass a string literal as an argument,
    string is displayed on the screen*/
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR08_tf); //Setting helvetica font
    u8g2.drawButtonUTF8(64, 32+6, U8G2_BTN_SHADOW1|U8G2_BTN_HCENTER|U8G2_BTN_BW2, 0,  2,  2, string ); //Displaying the Alert
  } while ( u8g2.nextPage() );
}

void showSetLockTime(int cursorXPos){
  /* Pass the cursors x position and this will
     display the set lock time screen. */
  u8g2.firstPage();
  do {
    drawStrCentred(&menuItemSLT); //Set lock time text at the top 
    for(int i = 0; i < 3; i++)
    {
      //Displaying the position/hours to be set
      drawStrCentred(&menuItemTime[i]);
      drawStrCentred(&menuItemTimeLabel[i]);
    }

    //Displaying the cursor
    u8g2.drawTriangle(cursorXPos, 28, cursorXPos - 5, 33, cursorXPos + 5, 33);
    u8g2.drawTriangle(cursorXPos, 56, cursorXPos - 4, 51, cursorXPos + 4, 51);
  } while ( u8g2.nextPage() );
}

int setLockTime(){
  /*Prompts user for the lock time and returns an int array of {hours,minutes,seconds}*/

  //Initializing local variables
  char pos = 0;
  int lockDuration[3] = {/*Hours=*/0, /*Minutes=*/0, /*Seconds=*/0}; 
  bool buttonState = 1;
  char buttonCounter, cursorXPos;
  buttonCounter = cursorXPos = 0;
  //Setting position of rotary encoder to 0
  encoder->setPosition(0);
  while(true){
    //Obtaining encoder position stored as pos
    pos = encoder->getPosition();

    // Defines where the triangle cursors x position should be using branchless coding to save storage
    cursorXPos = (buttonCounter == 0)*(DISPLAY_WIDTH/8) + (buttonCounter == 1)*(DISPLAY_WIDTH/2) + (buttonCounter == 2)*(112);
    
    char str[3][4];
    // Turning the hours, minutes and seconds set by user
    // Into strings and storing them in the respective menuItems
    for(int i = 0; i < 3; i++){
      sprintf(str[i], "%d", lockDuration[i]); 
      menuItemTime[i].value = str[i];
    }

    //Run the setLockTime Screen
    showSetLockTime(cursorXPos);

    // Reading the button
    buttonState = digitalRead(PIN_BUTTON);

    // This is the logic for the switch on the rotary encoder
    if (buttonState == 0) {
        buttonCounter++;
        encoder->setPosition(0);
        while(buttonState == 0){
          buttonState = digitalRead(PIN_BUTTON);
        }
    }
    // Making sure values of hours, minutes and seconds cannot exceed 60
    else if (pos < 0) {
      encoder->setPosition(0);
    }
    else if (pos > 60){
      encoder->setPosition(60);
    }

    //Setting time to position based on how many times the button has been pressed
    if (buttonCounter > 2){
      int (*lockDurationPtr)[3] = &lockDuration;
      return lockDurationPtr;
    }
    else {
      lockDuration[buttonCounter] = pos;
    }
  }
}

void showLockSplash(){
  /*Takes no arguments and
    displays *messages[] on the screen*/

  randomSeed(millis()); //Random number based on elapsed time
  //This is used to select a random message
  unsigned char randomInt = random(sizeof(messages)/sizeof(char*));
  unsigned char sizeOfString = strlen(messages[randomInt]);
  unsigned char overflowLength = 15;

  // Storing the current message (string literal) as currentMessage
  const char* currentMessage = messages[randomInt];

  // If the string is longer than overflowLength it gets split
  //Takes about 75ms
  if (sizeOfString > overflowLength){
    // Splitting the string into two using pointer arithmetics for speed
    char* firstPart = new char[overflowLength + 1]; 
    strncpy(firstPart, currentMessage, overflowLength);
    firstPart[overflowLength] = '\0';

    const char* secondPart = currentMessage + overflowLength;

    u8g2.firstPage();
    do {
    // Printing the strings centred
    menuItemSplashMessage[0].yOffset = -10;
    menuItemSplashMessage[0].value = firstPart;
    menuItemSplashMessage[1].value = secondPart;
    drawStrCentred(&menuItemSplashMessage[0]);
    drawStrCentred(&menuItemSplashMessage[1]);
    } while ( u8g2.nextPage() );
    delete[] firstPart;
  }
  // If the string is shorter than overflowLength
  else{
    u8g2.firstPage();
    do {
      // Display string
      menuItemSplashMessage[0].yOffset = 0;
      menuItemSplashMessage[0].value = currentMessage;
      drawStrCentred(&menuItemSplashMessage[0]);
    } while ( u8g2.nextPage() );
  }
}

bool doorOpen(){
  /* Checks whether the door is open,
    if so returns true */
  int sensorValue = analogRead(PIN_POTENTIOMETER); // Read the potentiometer
  return (sensorValue > THRESHOLD); //true if door is open
}

void lock(char lockHours, char lockMinutes, char lockSeconds){
  /*Takes three parameters:
  - char lockHours 
  - char lockMinutes
  - char lockSeconds
   Locks the phoneLox for that amount of time*/

  //initial time
  unsigned long t1 = millis();
  //lock time in seconds
  int lockTime = lockHours*60*60 + lockMinutes*60 +lockSeconds;

  //Check if door is open
  //If so, ask for it to be closed & check
  bool doorIsOpen = doorOpen();
  drawAlert("Please close door");
  // Loops whilst the the door is open, and prevents other code running
  while(doorIsOpen){
    doorIsOpen = doorOpen();
    delay(1);
  }

  // Displaying engaging lock text
  drawAlert("Engaging Lock");
  // Engaging servo lock
  for (int x = 0; x <= 90; x++){
    servo.write(x);
    delay(5); 
  }

  while(true){
    unsigned long t2 = millis(); 
    //If the lockTime equals time elapsed
    if(lockTime <= (t2-t1)/1000){
      //Unlock Door
      for (int x = 90; x > 0; x--){
        servo.write(x);
        delay(5);
      }
      // Displaying disengaging lock text
      drawAlert("Please take your phone :)");
      delay(1000);
      return;
    }
    // Every 10 seconds display lockSplash
    else if((t2-t1) % 10000 == 0){
      showLockSplash();
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  int* lockTime = setLockTime();
  lock(lockTime[0],lockTime[1],lockTime[2]);
}