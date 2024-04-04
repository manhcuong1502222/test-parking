//    _____                      __  ____             __   _             _____            __
//   / ___/____ ___  ____ ______/ /_/ __ \____ ______/ /__(_)___  ____ _/ ___/__  _______/ /____  ____ ___
//   \__ \/ __ `__ \/ __ `/ ___/ __/ /_/ / __ `/ ___/ //_/ / __ \/ __ `/\__ \/ / / / ___/ __/ _ \/ __ `__ \
//  ___/ / / / / / / /_/ / /  / /_/ ____/ /_/ / /  / ,< / / / / / /_/ /___/ / /_/ (__  ) /_/  __/ / / / / /
// /____/_/ /_/ /_/\__,_/_/   \__/_/    \__,_/_/  /_/|_/_/_/ /_/\__, //____/\__, /____/\__/\___/_/ /_/ /_/
//                                                            /____/      /____/
/**
   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS 1    SDA(SS)      ** custom, take a unused pin, only HIGH/LOW required *
   SPI SS 2    SDA(SS)      ** custom, take a unused pin, only HIGH/LOW required *
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

/* Arduino UNO */
/* RFID Readers */
#define RST_PIN   9
#define SS_1_PIN  7
#define SS_2_PIN  8

// List of Tags UIDs that are allowed to open the puzzle
byte tagarray[][4] = {
  {0xAD, 0x96, 0x38, 0xA4},
  {0xD6, 0x31, 0xA5, 0xB9},
  {0xC3, 0x1E, 0xBE, 0x44},     //ID
};

// Inlocking status :
bool access = false;

#define num_Readers 2
byte ssPins[] = {SS_1_PIN, SS_2_PIN};
MFRC522 mfrc522[num_Readers];   // Create MFRC522 instance.
//////

/* Servo Motors */
#define SERVO_1_PIN 5
#define SERVO_2_PIN 6

//#define num_Servos  2
byte servoPins[] = {SERVO_1_PIN, SERVO_2_PIN};
Servo myServo[num_Readers];
/////

//Sensor, Led, Buzzer,...
int SENSOR_PIN_[2] = {A1, A2};
bool data[2] = {false, false};
bool in = true;
bool out = true;

#define LED_R     3 //define red LED
#define LED_G     4 //define green LED pin

#define BUZZER    2 //buzzer pin

void setup()
{
  Serial.begin(9600);   // Initiate a serial communication
  while (!Serial);
  SPI.begin();      // Initiate  SPI bus

  pinMode(LED_R, OUTPUT);
  digitalWrite(LED_R, HIGH);
  pinMode(LED_G, OUTPUT);
  digitalWrite(LED_G, LOW);
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);

  /* looking for MFRC522 readers */
  for (uint8_t reader = 0; reader < num_Readers; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);    // Initiate MFRC522
    delay(5);
    Serial.print(F("Reader "));
    Serial.print(reader + 1);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
    //mfrc522[reader].PCD_SetAntennaGain(mfrc522[reader].RxGain_max);

    myServo[reader].attach(servoPins[reader]); //servo pin
    myServo[reader].write(90); //servo start position

    pinMode(SENSOR_PIN_[reader], INPUT);
  }

  Serial.println("Put your card to the reader...");
  Serial.println();

}
void loop()
{
  for (uint8_t reader = 0; reader < num_Readers; reader++) {
    // Looking for new cards
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
      Serial.print(F("Reader "));
      Serial.print(reader + 1);

      //    Serial.println(mfrc522[reader].uid.uidByte);
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card UID:"));
      dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      //      Serial.println(dump_byte_array_(mfrc522[0].uid.uidByte, mfrc522[1].uid.uidByte, mfrc522[0].uid.size, mfrc522[1].uid.size));
      Serial.println();

      for (int x = 0; x < sizeof(tagarray); x++)                  // tagarray's row
      {
        for (int i = 0; i < mfrc522[reader].uid.size; i++)        //tagarray's columns
        {
          if ( mfrc522[reader].uid.uidByte[i] != tagarray[x][i])  //Comparing the UID in the buffer to the UID in the tag array.
          {
            DenyingTag();
            break;
          }
          else
          {
            if (i == mfrc522[reader].uid.size - 1)                // Test if we browesed the whole UID.
            {
              AllowTag();
            }
            else
            {
              continue;                                           // We still didn't reach the last cell/column : continue testing!
            }
          }
        }
        if (access) break;                                        // If the Tag is allowed, quit the test.
      }
      if (access)
      {
        if (reader == 0)
        {
          OpenBarrier(reader);
          //          data[reader] =
          Serial.print("Xe dang qua barrier ");
          Serial.println(reader + 1);

          in = false;
        }
        if (reader == 1)
        {
          digitalWrite(LED_G, HIGH);
          digitalWrite(LED_R, LOW);
          out = false;
        }
        access = false;
      }
      else
      {
        Warning();
      }
      /*Serial.print(F("PICC type: "));
        MFRC522::PICC_Type piccType = mfrc522[reader].PICC_GetType(mfrc522[reader].uid.sak);
        Serial.println(mfrc522[reader].PICC_GetTypeName(piccType));*/
      // Halt PICC
      mfrc522[reader].PICC_HaltA();
      // Stop encryption on PCD
      mfrc522[reader].PCD_StopCrypto1();
    } //if (mfrc522[reader].PICC_IsNewC..
    //    data[reader] = digitalRead(SENSOR_PIN_[reader]);
    //    if (!in && digitalRead(SENSOR_PIN_[reader]))
    //    {
    //
    //    }
    //    Serial.println(digitalRead(SENSOR_PIN_[0]));
  } //for(uint8_t reader..
  if (!in && !digitalRead(SENSOR_PIN_[0]))
  {
    data[0] = true;
    delay(10);
  }
  if (!in && data[0] && digitalRead(SENSOR_PIN_[0]))
  {
    data[0] = false;
    in = true;
    //            data = 1;
    CloseBarrier(0);
    digitalWrite(LED_R, HIGH);
    Serial.print("Xe da vao!");
    Serial.println("!");
    //      access = false;
  }
  if (!out && !digitalRead(SENSOR_PIN_[1]))
  {
    OpenBarrier(1);
    out = true;
    delay(2500);
    CloseBarrier(1);
  }
  if (out && !digitalRead(SENSOR_PIN_[1]))
  {
    Warning();
  }
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void DenyingTag()
{
  Serial.println(" Access denied");
  access = false;
}

void AllowTag()
{
  Serial.println("Authorized access");
  access = true;
}

void OpenBarrier(int reader)
{
  Serial.println("Welcome! The barrier is now open");
  myServo[reader].write(0);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_R, LOW);
  delay(50);
}

void CloseBarrier(int reader)
{
  myServo[reader].write(90);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  delay(50);
}

void Warning()
{
  Serial.println("This Tag isn't allowed!");
  for (int flash = 0; flash < 5; flash++)
  {
    tone(BUZZER, 300);
    digitalWrite(LED_R, LOW);
    delay(100);
    noTone(BUZZER);
    digitalWrite(LED_R, HIGH);
    delay(100);
  }
}
