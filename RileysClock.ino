// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <SPI.h>
#include <WiFi101.h>

#include "network.h" // This file includes the network SSID and pass

RTC_DS3231 rtc;
Adafruit_IS31FL3731_Wing ledmatrix = Adafruit_IS31FL3731_Wing();

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};
static const uint8_t PROGMEM
  numbers[][5] = {
    {
      B01000000,
      B10100000,
      B10100000,
      B10100000,
      B01000000,
    },
    {
      B01000000,
      B01000000,
      B01000000,
      B01000000,
      B01000000,
    },
    {
      B11000000,
      B00100000,
      B01000000,
      B10000000,
      B11100000,
    },
    {
      B11000000,
      B00100000,
      B01000000,
      B00100000,
      B11000000,
    },
    {
      B10000000,
      B10100000,
      B11100000,
      B00100000,
      B00100000,
    },
    {
      B11100000,
      B10000000,
      B11000000,
      B00100000,
      B11100000,
    },
    {
      B01100000,
      B10000000,
      B11000000,
      B10100000,
      B01000000,
    },
    {
      B11100000,
      B00100000,
      B01000000,
      B01000000,
      B01000000,
    },
    {
      B01000000,
      B10100000,
      B01000000,
      B10100000,
      B01000000,
    },
    {
      B01000000,
      B10100000,
      B01100000,
      B00100000,
      B11000000,
    }  
  };
int status = WL_IDLE_STATUS;
WiFiServer server(80);
uint8_t incr = 0;
char message[100] = "";


void setup() {
  WiFi.setPins(8,7,4,2);
  Serial.begin(9600);

  Serial.println("Started serial");

  delay(3000); // wait for console opening

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  Serial.println("Started RTC");
  if (! ledmatrix.begin()) {
    Serial.println("IS31 not found");
    while (1);
  }
  Serial.println("Started matrix");

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    //delay(10000);
  }
  server.begin();
  // you're connected now, so print out the status:
  printWiFiStatus();
}

void loop() {
  incr++;
  if(incr > 23) incr = 0;
  
  ledmatrix.setFrame(incr % 8); // Draw to a fresh frame buffer
  ledmatrix.fillScreen(0);  // clear the screen

  //doSweep(incr); // animated light effect in background

  //ledmatrix.drawLine(0, 4, 14, 4, 255); // draw a line for screen alignment

  //getTime(); // Displays the time to serial
  //drawTime(4);  // Draw the actual time to the screen

  if(message[0] != 0) {
    ledmatrix.setCursor(6 - incr % 6,0);
    ledmatrix.setTextColor(32, 0);
    ledmatrix.setTextSize(1);
    ledmatrix.setTextWrap(false);
    if(incr % 6 == 0) {
      char first = message[0];
      for(uint8_t z = 0; z < sizeof(message); z++) {
        message[z] = message[z+1];
        if(message[z] == 0) break;
      }
    }
    ledmatrix.print(message);
  }
  else {
    drawTime(4);
  }

  serve();
  
  
  ledmatrix.displayFrame(incr % 8); // Swap out the frame buffer with the new drawing
}

void doSweep(uint8_t incr) {
  for (uint8_t x = 0; x < 16; x++) {
    for (uint8_t y = 0; y < 9; y++) {
      ledmatrix.drawPixel(x, y, (int)(sweep[(x+y+incr)%24] / 1));
    }
  }
}

void drawTime(uint8_t brightness) {
  DateTime now = rtc.now();
  uint8_t offset = 1;
  uint8_t hour = now.hour();
  
  if(hour >= 12) hour = hour - 12;
  if(hour >= 10) {
    ledmatrix.drawLine(0, 1, 0, 5, brightness);
    offset = 0;
  }
  ledmatrix.drawBitmap(2 - offset, 1, numbers[hour % 10], 3, 5, brightness);
  ledmatrix.drawBitmap(12 - offset, 1, numbers[now.minute() % 10], 3, 5, brightness);
  ledmatrix.drawBitmap(8 - offset, 1, numbers[(int)(now.minute() / 10)], 3, 5, brightness);
  if(now.second() % 2 == 0) {
    ledmatrix.drawPixel(6 - offset, 2, brightness);
    ledmatrix.drawPixel(6 - offset, 4, brightness);
  }
}

void getTime() {
    DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.println();
}

void serve() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          DateTime now = rtc.now();
          sprintf(message, "%04d-%02d-%02d %s %02d:%02d:%02d", now.year(), now.month(), now.day(), daysOfTheWeek[now.dayOfTheWeek()], now.hour(), now.minute(), now.second());
          client.print("<h1>");
          client.print(message);
          client.print("</h1>");
    
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  sprintf(message, "IP Address: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

