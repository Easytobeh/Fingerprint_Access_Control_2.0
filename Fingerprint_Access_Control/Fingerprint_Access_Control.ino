/*Title: Fingerprint Access Control System
  Author: Junaid Oloruntobi
  Date: 15th of August, 2019. (Completion)
  ------------------------------------------
                    CODE  OVERVIEW
  ------------------------------------------
  This sketch is based on the R30X series of fingerprint sensor module. It is written for
  access for access control of a restricted area where the door lock used is an 12V magnetic
  solenoid door lock. The system has two options for access:
   password option
   Biometric fingerprint scan
   Capacitive touch sensor to open door from inside. Could be replaced with a pushbutton.
  Kindly refer to the documentation that comes with this code for more explanation

*/
#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <Wire.h> //required for I2C LCD
#include<LiquidCrystal_I2C.h>
#include<EEPROM.h>
#define RXpin 11
#define TXpin 12
#define relayPin 13
#define touchSensor 0
#define PIR A0
#define alarm A3

/***Funtion Prototypes***/
void doorOpen();
void noAccess();
void wrongPass();
void deletePrint();
void changePassKey();
void passcodeCheck();
void fingerPrintSettings();
void passCodeAccess();
void settingsOption();
void displayFunction();
void fingerprintAccess();
uint8_t getFingerprintEnroll(int id);

SoftwareSerial mySerial(RXpin, TXpin); //Fingerprint TX,RX pins respectively
LiquidCrystal_I2C lcd(0x27, 16, 2);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

//Within loop function for PIR check without disrupting other program
unsigned long previousMillis = 0; //stores last time PIR was checked
const long interval = 5000;       //Interval for checking PIR in ms

char passcode[4];  // stores passcode input from the user
char EEPROM_pass[4];  //holds passcode currently present on the EEEPROM
char temp_pass1[4], temp_pass2[4]; //holds passcode for comparison during passcode change routine

uint8_t id, p; //Integer variable for which the fingerprint module maps saved print template as in its memory. 0-255
//Also when retrieving a saved print, it returns the id number if found in its memory
int ID[3]; //stores id input entered by users when storiing a new print


char option;

int k = 0;
int j = 0;
int count = 0, i;


char customKey = 0;
const byte ROWS = 5; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {'A', 'B', '#', '*' },
  {'1', '2', '3', '↑' },
  {'4', '5', '6', '↓' },
  {'7', '8', '9', 'C' },
  {'←', '0', '→','E' }
};
byte rowPins[ROWS] = {10, 9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4, 5}; //connect to the column pinouts of the keypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  finger.begin(57600);

  pinMode(relayPin, OUTPUT);
  pinMode(PIR, INPUT);
  pinMode(alarm, OUTPUT);
  digitalWrite(alarm, LOW);
  digitalWrite(relayPin, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.home();
  lcd.print("Initializing...");
  delay(2000);
  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.println("  Fingerprint");
    lcd.setCursor(2, 1);
    lcd.print("Sensor OK");
    delay(2000);
  }
  else {
    lcd.clear();
    lcd.println("  Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("Sensor Not Found");
    delay(2000);
    while (1);
  }

  //copy the passcode from EEPROM into variable EEPROM_pass.
  //This variable holds the passcode throughout the program
  //except when changed
  for (j = 0; j < 4; j++)
    EEPROM_pass[j] = EEPROM.read(j);

  lcd.clear();
  lcd.print("    Ready!");
  delay(2000);
  Serial.println("EEPROM_pass is: ");
  Serial.print(EEPROM_pass);
  Serial.println("Passcode is: ");
  Serial.println(passcode);

}

void loop()
{
  displayFunction();
  //Check for print
  p = finger.getImage();

  //Check for Keypad Press
  customKey = customKeypad.getKey();

  //***************If Door open from inside*********
  if (digitalRead(touchSensor))
    doorOpen();

  //****************If Finger is placed*************
  if (p == FINGERPRINT_OK)
    fingerPrintAccess();

  //************If Keypad is pressed*********
  if (customKey)
    passCodeAccess();

  //*****RUNS SIMULTANEOUSLY WITH ABOVE CODE TO CHECK PIR AT PRESET INTERVAL*****/
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    if (digitalRead(PIR) == HIGH)
      digitalWrite(alarm, HIGH);
    else
      digitalWrite(alarm, LOW);
  }
}

void displayFunction() {
  lcd.clear();
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("Ent for settings");
  delay(50);
}

void changePassKey()
{
  lcd.clear();
  lcd.print("Ent->Change passcode");//Corresponding to letter E in keypad matrice
  lcd.setCursor(0, 1);
  lcd.print("Esc-> Exit"); //corresponding to letter C in keypad matrice
  option = customKeypad.waitForKey();
  if (option == 'E') {
    lcd.clear();
    lcd.print("Current passcode: ");
    lcd.setCursor(0, 1);
    lcd.blink();
    for (i = 0; i < 4; i++)
    {
      passcode[i] = customKeypad.waitForKey();
      lcd.print('*');
    }
    if (!(strncmp(passcode, EEPROM_pass, 4)))
    {
      lcd.clear();
      lcd.print("Enter New code");
      lcd.setCursor(0, 1);
      lcd.blink();
      for (i = 0; i < 4; i++)
      {
        option = customKeypad.waitForKey();
        temp_pass1[i] = option;
        lcd.print('*');
      }
      lcd.noBlink();
      lcd.clear();
      lcd.print("Enter code Again");
      lcd.setCursor(0, 1);
      lcd.blink();
      for (i = 0; i < 4; i++)
      {
        option = customKeypad.waitForKey();
        temp_pass2[i] = option;
        lcd.print('*');
      }
      i = 0;
      lcd.noBlink();
      lcd.clear();
      lcd.print(" Wait, Matching");
      lcd.setCursor(0, 1);
      lcd.print("  New Passcode ");
      delay(3000);
      if (!(strncmp(temp_pass1, temp_pass2, 4)))
      {
        lcd.clear();
        lcd.print("  Key Match  ");
        lcd.setCursor(0, 1);
        lcd.print("  Successful ");
        delay(1000);
        lcd.clear();
        lcd.print("Updating...");
        lcd.setCursor(0, 1);
        //Shows scrolling symbol on LCD
        for (int l = 0; l < 16; l++)
        { for (int m = 0; m < 5; m++)
          { lcd.setCursor(l, 1);
            lcd.write(m); //random symbol scroll
            delay(30);
          }
          delay(30);
        }
        //first set the current passcode in the EEPROM to 0000
        for (k = 0; k < EEPROM.length(); k++ )
          EEPROM.write(k, 0);

        //Sets the varaible holding the passcode in EEPROM to 0000
        for (k = 0; k < 4; k++)
          EEPROM_pass[k] = 0;

        //write the new passcode into EEPROM
        for (k = 0; k < 4; k++)
          EEPROM.write(k, temp_pass1[k]);

        //Also the variable holding the passcode in EEPROM to newly set code
        for (k = 0; k < 4; k++)
          EEPROM_pass[k] = temp_pass1[k];

        lcd.clear();
        lcd.print("Passcode Change");
        lcd.setCursor(2, 1);
        lcd.print("Successful!");
        delay(2000);
        lcd.clear();
        lcd.print("Exiting");
        for (k = 7; k < 15; k++)
        { lcd.setCursor(k, 0);
          lcd.print('.');
          delay(200);
        }
        delay(1000);
        return;
      }
      else
      { lcd.clear();
        lcd.print("New Passkey do");
        lcd.setCursor(0, 1);
        lcd.print("  not match ");
        delay(2000);
        lcd.clear();
        lcd.print("Exiting");
        for (k = 7; k < 15; k++)
        { lcd.setCursor(k, 0);
          lcd.print('.');
          delay(200);
        }
        delay(1000);
        // displayFunction();
      }
    }
    else
      wrongPass();
  }
  else if (option == 'C')
    return;
}//end change function

void wrongPass() {
  lcd.clear();
  lcd.noBlink();
  lcd.print("  Error!");
  delay(100);
  lcd.setCursor(0, 1);
  lcd.print("Wrong Passcode");
  delay(2000);
  //displayFunction();

}
/***********************************************/

void settingsOption() {
  lcd.clear();
  lcd.print("Enter Passcode");
  lcd.setCursor(5, 1);

  while (i < 4) {
    customKey = customKeypad.waitForKey();
    passcode[i++] = customKey;
    lcd.print('*');
  }
  lcd.noCursor();
  delay(50);
  i = 0;
  if (!(strncmp(passcode, EEPROM_pass, 4)))//compare passcode input with memory
  {

    
    //if the up arrow is selected, it takes function back to this point.
    lcd.clear();
    lcd.print("F1:Code Settings");
    lcd.setCursor(0, 1);
    lcd.print("F2:Print Setting");
    customKey = customKeypad.waitForKey();
    switch (customKey)
    {
      case 'A':
        changePassKey();
        break;
      case 'B':
        fingerPrintSettings();
        break;
      case 'C':
        return;
      default:
        {
          lcd.clear();
          lcd.setCursor(5, 0);
          lcd.print("Not");
          lcd.setCursor(2, 1);
          lcd.print("Supported!");
          delay(2000);
          return;
        }
    }
  }
  else
  { lcd.clear();
    lcd.print("Wrong Passcode");
    delay(2000);
    return;
  }
}
/**********************************/
void fingerPrintSettings() {
   digitalWrite(alarm, LOW);
  lcd.clear();
  lcd.print("F1:New Fingerprint");
  lcd.setCursor(0,1);
  lcd.print("F2:Delete Print");
  customKey=customKeypad.waitForKey();
  if (customKey == 'A')
  addNewPrint();
  
  else if (customKey == 'B')
  deletePrint();

  else if(customKey == 'C')
  return;
  
  else 
  {lcd.clear();
  lcd.print("Not Supported");
  delay(2000);}
}


void addNewPrint(){
  p = -1;
  lcd.clear();
  lcd.print("Place finger");
  while (p != FINGERPRINT_OK)
    p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Image taken");
    delay(1000);
    lcd.clear();
    lcd.print(" Remove Finger");
    delay(1000);
    p = finger.image2Tz(1);
    if (p == FINGERPRINT_OK) //image conversion success!
    {
      p = -1;
      lcd.clear();
      lcd.print("Place same ");
      lcd.setCursor(0, 1);
      lcd.print("finger again ");

      //Scans finger a second time
      //wait until print is detected
      while (p != FINGERPRINT_OK)
        p = finger.getImage();
      if (p == FINGERPRINT_OK)//if scanned == success
      {
        lcd.clear();
        lcd.print("Image taken");
        delay(1000);
        lcd.clear();
        lcd.print(" Remove Finger");
        delay(1000);
        p = finger.image2Tz(2);//convert image
        if (p == FINGERPRINT_OK) //Image conversion is OK
        {
          p = finger.createModel();
          if (p == FINGERPRINT_OK)
          {
            lcd.clear();
            lcd.print("Prints matched!");
            delay(1000);
            lcd.clear();
            lcd.print("Press I.D number");
            lcd.setCursor(0, 1);
            lcd.print("between 1-127");
            customKey = customKeypad.waitForKey();
            if (customKey == 'C' || customKey == '0') //If user presses the ESC key
              return;              //exit this function
            else
            {
              lcd.clear();
              lcd.print("Ent to continue");
              lcd.setCursor(0, 1);
              lcd.print("I.D Number:");
              lcd.print(customKey);

              //If user doesn't press ENT, keep getting Keypad Input until maximum number of 3 digits is reached
              //ID is required to store any number between 1-127 as input
              for (count = 0; count < 2; count++) //allows maximum entry of 3 digits
              { ID[count] = customKey;
                customKey = customKeypad.waitForKey();
                if (customKey != 'E') //if user doesn't press Ent key
                { lcd.print(customKey);
                  continue;
                }
              }
              id = uint8_t(ID); //set the content of ID array as the I.D to save with

              //Search if fingerprint exist
              p = finger.fingerFastSearch();
              if (p == FINGERPRINT_OK) //found a matching print
              {
                lcd.clear();
                lcd.print("Print Exists");
                lcd.setCursor(0, 1);
                lcd.print("cannot store");
                delay(2000);
              }
              else if (p == FINGERPRINT_PACKETRECIEVEERR)
              {
                lcd.clear();
                lcd.print(" Communication");
                lcd.setCursor(4, 1);
                lcd.print("error");
                delay(2000);
                return;
              }
              else if (p == FINGERPRINT_NOTFOUND) //No match go ahead and save
              {
                p = finger.storeModel(id);
                if (p == FINGERPRINT_OK)
                {
                  lcd.clear();
                  lcd.setCursor(4, 0);
                  lcd.print(" Stored");
                  delay(2000);
                }
                else
                {
                  //Serial.println("Unknown error");
                  lcd.clear();
                  lcd.print(" Error Storing");
                  lcd.setCursor(4, 1);
                  lcd.print("Print");
                  delay(2000);
                  return;
                }
              }
              else {
                lcd.clear();
                lcd.setCursor(4, 0);
                lcd.print("Unknown");
                lcd.setCursor(4, 1);
                lcd.print("error");
                delay(2000);
                return;
              }
            }
          }
          else
          { //if creating model !=OK
            lcd.clear();
            lcd.print("Fingerprints did");
            lcd.setCursor(2, 1);
            lcd.print("not Match");
            delay(2000);
          }
        }
        else {//If second image capture !=OK
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("Unknown");
          lcd.setCursor(5, 1);
          lcd.print("Error");
          delay(1000);
        }
      }
      else {//If second image conversion !=OK
        lcd.clear();
        lcd.setCursor(3, 0);
        lcd.print("Unknown");
        lcd.setCursor(5, 1);
        lcd.print("Error");
        delay(1000);
      }
    }
    else {//if first image conversion !=OK

      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Unknown");
      lcd.setCursor(5, 1);
      lcd.print("Error");
      delay(1000);
    }
  }
  else {//if first image capture !=OK
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Unknown");
    lcd.setCursor(5, 1);
    lcd.print("Error");
    delay(1000);
  }
}

void doorOpen() {
  lcd.clear();
  digitalWrite(alarm, LOW);
  lcd.print("Access Granted");
  digitalWrite(relayPin, HIGH);
  delay(5000);
  digitalWrite(relayPin, LOW);
}

void noAccess() {
  lcd.clear();
  digitalWrite(alarm, LOW);
  lcd.print("Access Denied");
  digitalWrite(relayPin, LOW);
  delay(2000);
}

void fingerPrintAccess() {
  p = finger.image2Tz();
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Remove Finger");
    delay(1000);
    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK)
      doorOpen();
    else
      noAccess();
  }
  else
  {
    lcd.clear();
    lcd.print("Unknown Error");
    delay(1000);
  }
}

void passCodeAccess() {
  //Means Settings option is selected
  if (customKey == 'E') {
    lcd.clear();
    lcd.print("Entering Setting");
    delay(2000);
    settingsOption();
  }

  else { //Regard as passcode input option to grant user access
    i = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    while (i < 3) {
      passcode[i] = customKey;
      Serial.println(customKey);
      lcd.print('*');
      customKey = customKeypad.waitForKey();
      i++;
    } passcode[i] = customKey;
    lcd.noCursor();
    delay(50);
    i = 0;
    Serial.println("Passcode is: ");
    Serial.print(passcode);

    if (!(strncmp(passcode, EEPROM_pass, 4)))//compare passcode input with memory
      doorOpen();
    else
      noAccess();
  }
}


void deletePrint()
{
  lcd.clear();
  lcd.print("ID to delete");
  lcd.setCursor(0, 1);
  for (count = 0; count < 2; count++) //allows maximum entry of 3 digits
  { customKey = customKeypad.waitForKey();
    ID[count] = customKey;
    lcd.print(customKey);
    if (customKey == 'E') //in case user presses Enter key
      break;
  }
  id = uint8_t(ID);
  for (count = 0; count < 2; count++)
    ID[count] = " ";

  lcd.clear();
  lcd.print("Do you wish to");
  lcd.setCursor(0, 1);
  lcd.print("Proceed");
  customKey = customKeypad.waitForKey();
  if (customKey == 'E') {
    lcd.print("deleting I.D:");
    lcd.setCursor(0, 1);
    lcd.print(customKey);
    uint8_t p = -1;
    p = finger.deleteModel(id);
    delay(2000);
    if (p == FINGERPRINT_OK)
    {
      lcd.clear();
      lcd.print("Delete Success!");
      delay(2000);
    }
    else
    {
      lcd.clear();
      lcd.print("Delete Error!");
      delay(2000);
    }
  }
  else
    return;
}
