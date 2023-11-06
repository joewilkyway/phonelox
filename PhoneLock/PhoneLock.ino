#include <Arduino.h>
#include <RotaryEncoder.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Servo.h>

/*/------\*/
/*| Pins |*/
/*\------/*/
/* rotary encoder */
#define PIN_IN1 2
#define PIN_IN2 3
/* rotary encoders button */
#define PIN_BUTTON 4
/* servo*/
#define PIN_SERVO 7
/*Diplsay pins: clock=13, data=11, cs=10, dc= 9 , reset=8 */

/*/------------------------------------\*/
/*| Definitions for the Rotary Encoder |*/
/*\------------------------------------/*/

// A pointer to the dynamic created rotary encoder instance.
// (It's recommended by the author of the RotaryEncoder Library)
RotaryEncoder *encoder = nullptr;

// This will be called by the interrupt service routine
/* At the point of change, the state of the rotary encoder will be 
   stored within the encoder object */
void checkPosition()
{
  encoder->tick(); // Calling tick() to checks the state of the rotary encoder
}

/*/----------------------------------\*/
/*| Definitions for the OLED Display |*/
/*\----------------------------------/*/
U8G2_SH1106_128X64_NONAME_1_4W_SW_SPI u8g2(/*rotation=*/U8G2_R0, /*clock=*/13, /*data=*/11, /*cs=*/10, /*dc=*/ 9 , /* reset=*/8);

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

/*/-----------------------------------\*/
/*| Definitions for the potentiometer |*/
/*\-----------------------------------/*/
bool doorOpen = 0;

/*/---------------------------------\*/
/*| Definitions for the servo motor |*/
/*\---------------------------------/*/
Servo servo;

struct menuItem
{
  int16_t xOffset;		/* in pixel */
  int16_t yOffset;		/* in pixel */
  const char *value;			/* position, array index */
  const uint8_t *font;
};

struct menuItem menuItemTime[] =
{
  {-40, 0, "0", u8g2_font_helvR12_tf},
  {0, 0, "0", u8g2_font_helvR12_tf},
  {40, 0, "0", u8g2_font_helvR12_tf}
};
struct menuItem menuItemSLT = {0, -30, "Set Lock Time", u8g2_font_helvR10_tf};

struct menuItem menuItemSplashMessage[] = 
{
  {0, 10, "0", u8g2_font_helvR12_tf},
  {0, -10, "0", u8g2_font_helvR12_tf},
};

void setup() {
  Serial.begin(9600);
  /*/-------------------------------\*/
  /*| Initiating the Rotary Encoder |*/
  /*\-------------------------------/*/
  Serial.println("Initiating the rotary encoder");

  // Declaring the encoder object with the predefined pins
  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

  // register interrupt routine
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  // for the button
  pinMode(PIN_BUTTON,INPUT_PULLUP);

  // for the servo
  servo.attach(PIN_SERVO);
  servo.write(0);

  /*/-----------------------------\*/
  /*| Initiating the OLED Display |*/
  /*\-----------------------------/*/
  u8g2.begin();  
  u8g2.firstPage();
  // displaying a splash screen
  drawAlert("PhoneBox");
  delay(500);

}

void drawStrCentred(struct menuItem *item) {
  u8g2.setFont(item->font);
  u8g2.setFontPosCenter();
  int x = DISPLAY_WIDTH/2 - u8g2.getStrWidth(item->value)/2 + item->xOffset;
  int y = DISPLAY_HEIGHT/2 + item->yOffset;
  u8g2.drawStr(x, y, item->value);
}

void drawAlert(const char *string){
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR10_tf); //Setting helvetica font
    u8g2.drawButtonUTF8(64, 32+6, U8G2_BTN_SHADOW1|U8G2_BTN_HCENTER|U8G2_BTN_BW2, 0,  2,  2, string ); //Displaying the Alert
  } while ( u8g2.nextPage() );
}

void showSetLockTime(int cursorXPos){
  u8g2.firstPage();
  do {
    drawStrCentred(&menuItemSLT); //Set lock time text at the top
    //Displaying the position/hours to be set
    for(int i; i > 3; i++)
    {
      drawStrCentred(&menuItemTime[i]);
    }
    
    u8g2.setFont(u8g2_font_helvR08_tf);
    // drawStrCentred(displayWidth/8 + 20, 43, "Hrs");
    // drawStrCentred(displayWidth/2 + 20, 43, "Mins");
    // drawStrCentred(112 + 20, 43, "Secs");

    //Displaying the cursor
    u8g2.drawTriangle(cursorXPos, 28, cursorXPos - 5, 33, cursorXPos + 5, 33);
    u8g2.drawTriangle(cursorXPos, 56, cursorXPos - 4, 51, cursorXPos + 4, 51);
  } while ( u8g2.nextPage() );
}

char setLockTime(){
  /*Prompts user for the lock time and returns an int array of {hours,minutes,seconds}*/

  //Initializing local variables
  int pos = 0;
  char lockDuration[3] = {/*Hours=*/0, /*Minutes=*/0, /*Seconds=*/0}; 
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
    //Run the setLockTime Screen
    for (int i; i > 3; i++)
    {
      // Converting the numbers into strings
      const char timeStr[4];
      sprintf(timeStr, "%d", lockDuration[i]); 
      menuItemTime[i].value = timeStr;
    }

    showSetLockTime(cursorXPos);

    // Reading the button
    buttonState = digitalRead(PIN_BUTTON);

    // This is the logic for the switch on the rotary encoder
    if (buttonState == 0) {
        buttonCounter++;
        Serial.println("Forloop activated, this is the buttonCounter:");
        Serial.println(buttonCounter,DEC);
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
      char (*lockDurationPtr)[3] = &lockDuration;
      return lockDurationPtr;
    }
    else {
      lockDuration[buttonCounter] = pos;
    }
  }
}

void showLockSplash(){
  const char *messages[] = 
  {
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
  //Random number based on millis()
  randomSeed(millis());
  //This is used to select a random message
  unsigned char randomInt = random(sizeof(messages)/sizeof(char*));
  unsigned char sizeOfString = strlen(messages[randomInt]);
  unsigned char overflowLength = 17;

  // Storing the current message (string literal) as currentMessage
  const char* currentMessage = messages[randomInt];

  // If the string is longer than 17 it gets split
  //Takes about 75ms
  if (sizeOfString > overflowLength)
  {
    // Splitting the string into two using pointer arithmetics for speed
    char* firstPart = new char[overflowLength + 1]; 
    strncpy(firstPart, currentMessage, overflowLength);
    firstPart[overflowLength] = '\0';

    const char* secondPart = currentMessage + overflowLength;

    u8g2.firstPage();
    do {
    // Printing the strings centred
    menuItemSplashMessage[0].value = firstPart;
    menuItemSplashMessage[1].value = secondPart;
    drawStrCentred(&menuItemSplashMessage[0]);
    drawStrCentred(&menuItemSplashMessage[1]);
    } while ( u8g2.nextPage() );
    delete[] firstPart;
  }
  // If the string is shorter than 12
  else
  {
    u8g2.firstPage();
    do {
      // Display string
      menuItemSplashMessage[0].value = currentMessage;
      drawStrCentred(&menuItemSplashMessage[0]);
    } while ( u8g2.nextPage() );
  }
}

void lock(char lockHours, char lockMinutes, char lockSeconds){
  //initial time
  unsigned long t1 = millis();
  //lock time in seconds
  int lockTime = lockHours*60*60 + lockMinutes*60 +lockSeconds;
  Serial.println("This is the lock duration(s):");
  Serial.println(lockTime);
  bool lockEngaged = false;

  //Check if door is open
  //If so, ask for it to be closed & check
  while(doorOpen){
    drawAlert("Please close door");
    //Add some sort of doorOpenCheck()
  }

  // Displaying engaging lock text
  drawAlert("Engaging Lock");

  // Engaging servo lock
  for (int x = 0; x <= 90; x++){
    servo.write(x);
    delay(5); 
  }
  lockEngaged = true;
  while(lockEngaged){
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
      lockEngaged = false;
    }
    // Every 10 seconds display lockSplash
    else if((t2-t1) % 10000 == 0){
      showLockSplash();
    }

  }
}

void loop() {
  // put your main code here, to run repeatedly:
  char* lockTime = setLockTime();
  lock(lockTime[0],lockTime[1],lockTime[2]);
}

