#include <WiFi.h>
#include <EEPROM.h> // Include the EEPROM library
#include <SPIFFS.h> // Add this line to include the SPIFFS library

#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include "twilio.hpp"
#include "time.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels

// create an OLED display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo myservo;  // create servo object to control a servo
Servo myservo2;
int servoPin = 18; //servo pin-  the orange wire of the bottom servo connects to this
int servoPin2 = 19;

// Values from Twilio (find them on the dashboard)
static const char *account_sid = "ACf6ef91be94314bc3fa11d330497f11d8";
static const char *auth_token = "1d455874d05a1b7d413f7585b4a83d2d";
// Phone number should start with "+<countrycode>"
static const char *from_number = "<+1>2298007791";

// You choose!
// Phone number should start with "+<countrycode>"
static const char *to_number = "<+91>9562102002";
static const char *message = "This is the SMMS! Please take your medicine!";

Twilio *twilio;

const char* ssid = "Aps";
const char* password = "#Aps0018";


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 19080 + 720;

AsyncWebServer server(80);

int pos = 0;    // variable to store the servo position
int h1 = 0;
int m1 = 0;
int h2 = 0;
int m2 = 0;

int count1;
int count2;

const int LED1 = 25;
const int LED2 = 26;
const int BUZZER = 27;

void SMSmsg() {
  twilio = new Twilio(account_sid, auth_token);
  String response;
  bool success = twilio->send_message(to_number, from_number, message, response);
  if (success) {
    Serial.println("Sent message successfully!");
  } else {
    Serial.println(response);
  }
}



void handleRootPage(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html");
}

void handleFormSubmit(AsyncWebServerRequest *request) {
  if (request->hasArg("h1") && request->hasArg("m1") && request->hasArg("h2") && request->hasArg("m2")) {
    h1 = request->arg("h1").toInt();
    m1 = request->arg("m1").toInt();
    h2 = request->arg("h2").toInt();
    m2 = request->arg("m2").toInt();

    // Save the values to EEPROM
    EEPROM.begin(512); // Initialize EEPROM
    EEPROM.put(0, h1);
    EEPROM.put(sizeof(h1), m1);
    EEPROM.put(sizeof(h1) + sizeof(m1), h2);
    EEPROM.put(sizeof(h1) + sizeof(m1) + sizeof(h2), m2);
    EEPROM.commit(); // Commit the changes to EEPROM

    request->send(200, "text/plain", "Values received and stored. The ESP32 will now restart.");
    ESP.restart(); // Restart the ESP32 to apply the new values
  } else {
    request->send(400, "text/plain", "Bad Request");
  }
}


void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // initialize OLED display with I2C address 0x3C
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }

  delay(1000);         // wait 1 second for initializing
  oled.clearDisplay(); // clear display

  oled.setTextSize(4);         // set text size
  oled.setTextColor(WHITE);    // set text color
  oled.setCursor(0, 10);       // set position to display
  oled.println(" SMMS"); // set text
  oled.display();              // display on OLED

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // Read the values from EEPROM and initialize the variables
  EEPROM.begin(512);
  EEPROM.get(0, h1);
  EEPROM.get(sizeof(h1), m1);
  EEPROM.get(sizeof(h1) + sizeof(m1), h2);
  EEPROM.get(sizeof(h1) + sizeof(m1) + sizeof(h2), m2);

  server.on("/", HTTP_GET, handleRootPage);
  server.on("/submit", HTTP_POST, handleFormSubmit);

  server.begin();

  myservo.attach(servoPin);  // attaches the servo on ESP32 pin
  myservo2.attach(servoPin2);

  Serial.print("This is hour 1: ");
  Serial.println(h1);
  Serial.print("This is minute 1: ");
  Serial.println(m1);
  Serial.print("This is hour 2: ");
  Serial.println(h2);
  Serial.print("This is minute 2: ");
  Serial.println(m2);

  count1 = 0;
  count2 = 0;

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

void loop() {
  // Your main code goes here
  // You can now use h1, m1, h2, and m2 as integers in the rest of your code
  // Init and get the time
  delay(1000);

  printLocalTime();


}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  if ((timeinfo.tm_hour == h1) && (timeinfo.tm_min == m1)) {
    Serial.println("LED1 ON!");
    digitalWrite(LED1, HIGH);

    oled.clearDisplay(); // clear display
    oled.setTextSize(2);         // set text size
    oled.println(" Take Medicine 1"); // set text
    oled.display();              // display on OLED

    if (count1 == 0) {
      digitalWrite(BUZZER, HIGH);
      Serial.println("Writing Servo1! ");
      for (int pos = 0; pos <= 180; pos += 1) {
        // in steps of 1 degree
        myservo.write(pos);

        delay(15); // waits 15ms to reach the position
      }
      digitalWrite(BUZZER, LOW);
      for (int pos = 180; pos >= 0; pos -= 1) {
        myservo.write(pos);

        delay(15); // waits 15ms to reach the position


      }
      SMSmsg();
      count1 = 1;
    }

    digitalWrite(LED1, LOW);
    Serial.println("LED1 OFF!");
  }

  else if ((timeinfo.tm_hour == h2) && (timeinfo.tm_min == m2)) {
    Serial.println("LED2 ON!");
    digitalWrite(LED2, HIGH);

    oled.clearDisplay(); // clear display
    oled.setTextSize(2);         // set text size
    oled.println(" Take Medicine 2"); // set text
    oled.display();              // display on OLED


    if (count2 == 0) {
      digitalWrite(BUZZER, HIGH);
      Serial.println("Writing Servo2! ");
      for (int pos = 0; pos <= 180; pos += 1) {
        // in steps of 1 degree
        myservo.write(pos);

        delay(15); // waits 15ms to reach the position
      }
      digitalWrite(BUZZER, LOW);
      for (int pos = 180; pos >= 0; pos -= 1) {
        myservo.write(pos);

        delay(15); // waits 15ms to reach the position


      }
      SMSmsg();
      count2 = 1;
    }
    digitalWrite(LED2, LOW);
    Serial.println("LED2 OFF!");
  }
  else {
    Serial.println("NOT EQUAL");
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);

    oled.clearDisplay(); // clear display
    oled.setTextSize(4);         // set text size
    oled.println(" SMMS"); // set text
    oled.display();              // display on OLED
  }
}
