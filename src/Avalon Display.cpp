/*
   1_Inkplate_WiFi_HTTP example for e-radionica.com Inkplate 6
   For this example you will need USB cable, Inkplate 6 and stable WiFi Internet connection
   Select "Inkplate 6(ESP32)" from Tools -> Board menu.
   Don't have "Inkplate 6(ESP32)" option? Follow our tutorial and add it: 
   https://e-radionica.com/en/blog/add-inkplate-6-to-arduino-ide/

   This example will show you how to connect to WiFi network, get data from Internet and display that data on epaper.
   This example is NOT on to how to parse HTML data from Internet - it will just print HTML on the screen. 

   In quotation marks you will need write your WiFi SSID and WiFi password in order to connect to your WiFi network.

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: http://forum.e-radionica.com/en/
   15 July 2020 by e-radionica.com
*/

#include "Inkplate.h"               //Include Inkplate library to the sketch
#include <WiFi.h>                   //Include ESP32 WiFi library to our sketch
#include <HTTPClient.h>             //Include HTTP library to this sketch
#include "ArduinoJson.h"
#include "Arduino.h"
#include "ArduinoWebsockets.h"

// Injected now as build_flags for all common examples
//#define ssid ""                     //Name of the WiFi network (SSID) that you want to connect Inkplate to
//#define pass ""                     //Password of that WiFi network

const char* websockets_server_host = "openplotter.local";
const uint16_t websockets_server_port = 3000; // Port of the WebSocket server

using namespace websockets; // Use the websockets namespace to avoid typing it all the time

Inkplate display(INKPLATE_1BIT);    //Create an object on Inkplate library and also set library into 1 Bit mode (Monochrome)
boolean heartbeat = false; // Variable to check if the heartbeat is received

WebsocketsClient wsclient; // Create a WebsocketsClient object


void initDisplayText() {
  Serial.println("Initializing Display...wipe...draw...print...display");
  display.clearDisplay();
  display.display();

  // Draw the mid line
  display.drawThickLine(280, 0, 280, 800, 1, 20);

  display.setTextSize(3);

  display.setTextColor(WHITE, BLACK);
  display.cp437(true); // Use full charset
  display.setCursor(273, 10);
  display.write(0x03);
  display.setCursor(273, 70);
  display.print("I");
  display.setCursor(273, 100);
  display.print("n");
  display.setCursor(273, 130);
  display.print("k");
  display.setCursor(273, 160);
  display.print("p");
  display.setCursor(273, 190);
  display.print("l");
  display.setCursor(273, 220);
  display.print("a");
  display.setCursor(273, 250);
  display.print("t");
  display.setCursor(273, 280);
  display.print("e");
  display.setCursor(273, 340);
  display.print("4");
  display.setCursor(273, 400);
  display.print("A");
  display.setCursor(273, 430);
  display.print("v");
  display.setCursor(273, 460);
  display.print("a");
  display.setCursor(273, 490);
  display.print("l");
  display.setCursor(273, 520);
  display.print("o");
  display.setCursor(273, 550);
  display.print("n");
  display.setCursor(273, 610);
  display.print("2");
  display.setCursor(273, 640);
  display.print("0");
  display.setCursor(273, 670);
  display.print("2");
  display.setCursor(273, 700);
  display.print("5");


  display.setTextColor(BLACK, WHITE);
  display.setTextSize(3);
  display.setCursor(3, 0);
  display.print("Tiefe in m");
  display.setCursor(303, 0);
  display.print("COG in Grad");

  display.drawThickLine(0, 158, 600, 158, 1, 1);
  display.setCursor(3, 160);
  display.print("STW in kn");
  display.setCursor(303, 160);
  display.print("SOG in kn");

  display.drawThickLine(0, 318, 600, 318, 1, 1);
  display.setCursor(5, 320);
  display.print("Luftdruck");
  display.setCursor(305, 320);
  display.print("Position");

  display.drawThickLine(0, 478, 600, 478, 1, 1);
  display.setCursor(3, 480);
  display.print("TWA in Grad");
  display.setCursor(303, 480);
  display.print("AWA in Grad");

  display.drawThickLine(0, 638, 600, 638, 1, 1);
  display.setCursor(3, 640);
  display.print("TWS in kn");
  display.setCursor(303, 640);
  display.print("AWS in kn");

  //display.setTextSize(4);
  //display.print("(c)2025 Inkplate 4 Avalon");
  display.display();
  display.setTextSize(10); // Remeber: At Position is setTextSize(4) and then back to 9

  Serial.println("Finished Display Init");
}



void wsEventHandler(WebsocketsEvent event, String data) {
 if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connection Opened");
        display.println("Connection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connection Closed");
        display.println("Connection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
        display.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
        display.println("Got a Pong!");
    }
    display.partialUpdate();
}

void  HandleNavigationPosition(JsonObject valueObject) {
  int position_x = 303;
  int position_y = 345;

  double latitude = valueObject["value"]["latitude"];                               // 245.01020000000003
  Serial.printf("latitude: %f", latitude);  
  double longitude = valueObject["value"]["longitude"];                               // 245.01020000000003
  Serial.printf("longitude: %f", longitude);
  
  // Print values
  display.setCursor(position_x, position_y);
  display.setTextSize(3);
  display.printf("Lat: %f",latitude);
  display.setCursor(position_x, position_y + 25);
  display.printf("Lon: %f", longitude);
}
void HandlePressureTrend(JsonObject valueObject) {
  int position_x = 3;
  int position_y = 425;

  double severity = valueObject["value"]["severity"];                               // 245.01020000000003
  Serial.printf("severity: %f", severity);
  const char *tendency = valueObject["value"]["tendency"];                               // 245.01020000000003
  Serial.printf("tendency: %s", tendency);  
  const char *changerate = valueObject["value"]["changerate"];                               // 245.01020000000003
  Serial.printf("changerate: %s", changerate);
  Serial.println("Printing Barometer Trend");
  display.setCursor(position_x, position_y);
  display.setTextSize(3);
  display.printf("T: %1.0f %s", severity, tendency);
  display.setCursor(position_x, position_y + 25);
  display.printf("R: %s", changerate);
}
void HandleSpeed(double value, int position_x, int position_y) {
  display.setCursor(position_x, position_y);
  value = value * 1.94384;
  display.printf("%2.1f", value);
  Serial.printf("Ausgabe: %2.1f \n", value);
}
void HandleDEG(double value, int position_x, int position_y) {
  display.setCursor(position_x, position_y);
  value = value * RAD_TO_DEG;
  display.printf("%3.0f", value);
  Serial.printf("Ausgabe: %3.0f \n", value);
}

// handleMessage
void wsMessageHandler(WebsocketsMessage message){
  Serial.print("WebSocket message received: ");
  Serial.println(message.data());
  const int MessageLen = message.length(); // Get the length of the message
  Serial.printf("Message length: %d\n", MessageLen);

  //Heartbeat
  display.setCursor(273,10);
  display.setTextSize(3);
  display.setTextColor(WHITE, BLACK);
  if (heartbeat) {
    display.write(0x03); // Print the heart symbol
    heartbeat = false; // Reset heartbeat flag
  } else {
    display.write(0x07); // Print the heart symbol
    heartbeat = true; // Set heartbeat flag
  }
  display.setTextColor(BLACK, WHITE);
  
  if (message.isContinuation()) {
    Serial.println("Received a continuation message, ignoring.");
    return; // Ignore continuation messages
  }
  if (message.isPartial()) {
    Serial.println("Received a partial message, ignoring.");
    return; // Ignore partial messages
  }
  try
  {
    Serial.println("Deserializing JSON message...");
    // Stream& input -- richtige Länge eintragen.
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, message.data());
    if (!doc["name"].isNull())
    {
      Serial.println("Received JSON with 'name' key. As first message, this is ok.");
      return;
    }
    if (!doc["updates"].isNull() && doc["updates"].size() > 0) {
      Serial.println("Received JSON with 'updates' key.");
      JsonObject update = doc["updates"][0];
      
      if (update["timestamp"].isNull()) {
        Serial.println("Received update without timestamp. - This is not expected.");
        return;
      }
      const char *timestamp = update["timestamp"];                  // "2024-04-26T18:24:41.384Z"
      Serial.printf("Timestamp: %s ", timestamp);

      JsonObject valueObject = update["values"][0];
      const char *path = valueObject["path"];                      // "navigation.position"
      Serial.printf("Path: %s ", path);

      if (strcmp(path, "navigation.position") == 0) {
        HandleNavigationPosition(valueObject);
      } else if (strcmp(path, "environment.barometer.trend") == 0) {
        HandlePressureTrend(valueObject);
      } else {
        double value = valueObject["value"];                               // 245.01020000000003
        Serial.printf("Value: %f ", value);
        display.setTextSize(10); // Set text size to 10 for large values
        if (strcmp(path, "environment.outside.pressure") == 0) {
          display.setCursor(3, 345);
          value = value / 100;
          display.setTextSize(8);
          display.printf("%4.0f", value);
          Serial.printf("Ausgabe: %4.0f \n", value);
        } else if (strcmp(path, "environment.wind.angleTrueGround") == 0) {
          HandleDEG(value, 3, 505);
        } else if (strcmp(path, "environment.wind.speedOverGround") == 0) {
          HandleSpeed(value, 3, 665);
        } else if (strcmp(path, "environment.wind.angleApparent") == 0) {
          HandleDEG(value, 303, 505);
        } else if (strcmp(path, "environment.wind.speedApparent") == 0) {
          HandleSpeed(value, 303, 665);
        }  else if (strcmp(path, "environment.depth.belowSurface") == 0) {
          display.setCursor(3, 25);
          display.printf("%2.1f", value);
          Serial.printf("Ausgabe: %2.1f \n", value);
        } else if (strcmp(path, "navigation.courseOverGroundTrue") == 0) {
          HandleDEG(value, 303, 25);
        } else if (strcmp(path, "navigation.speedThroughWater") == 0) {
          HandleSpeed(value, 3, 185);
        } else if (strcmp(path, "navigation.speedOverGround") == 0) {
          HandleSpeed(value, 303, 185);
        }
      }
      display.partialUpdate();
    } else {
      Serial.println("Received JSON without 'updates' key or empty updates array.");
    }
    } catch (const std::exception& e) {
      Serial.print("Error printing JSON: ");
      Serial.println(e.what());
      return;
    }      
}

void setup() {
  Serial.begin(115200);             //Start serial communication
  Serial.println("Starting...");

  display.begin();                  //Init Inkplate library (you should call this function ONLY ONCE)
  display.clearDisplay();           //Clear frame buffer of display
  display.setRotation(1);
  display.display();                //Put clear image on display
  display.setTextSize(3);           //Set text scaling to two (text will be two times bigger)
  display.setCursor(0, 0);          //Set print position
  display.setTextColor(BLACK, WHITE);                 //Set text color to black and background color to white
  display.println("Scanning for WiFi networks...");    //Write text
  Serial.println("Scanning for WiFi networks...");    //Write text to serial monitor
  display.display();                                  //Send everything to display (refresh display)

  int n = WiFi.scanNetworks();                        //Start searching WiFi networks and put the nubmer of found WiFi networks in variable n
  if (n == 0) {                                       //If you did not find any network, show the message and stop the program.
    display.print("No WiFi networks found!");
    Serial.println("No WiFi networks found!");
    display.partialUpdate();
    while (true);
  } else {
    if (n > 10) n = 0;                                //If you did find, print name (SSID), encryption and signal strength of first 10 networks
    for (int i = 0; i < n; i++) {
      display.print(WiFi.SSID(i));
      display.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? 'O' : '*');
      display.print('\n');
      display.print(WiFi.RSSI(i), DEC);
      Serial.println(WiFi.SSID(i));
    }
    display.partialUpdate();   
    delay(2000);                       //(Partial) refresh thescreen
  }

  //display.clearDisplay();                             //Clear everything in frame buffer
  //display.setCursor(0,0);                             //Set print cursor to new position
  display.print('\r\n');
  display.printf("Connecting to %s\r\n", WIFI_SSID);
  Serial.printf("Connecting to %s\r\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);                             //Try to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);                                      //While it is connecting to network, display dot every second, just to know that Inkplate is alive.
    display.print('.');
    display.partialUpdate();
  }
  display.println("WiFi connected");                         //If it's connected, notify user
  Serial.println("WiFi connected");                         //If it's connected, notify user
  display.partialUpdate();

  // Initialize the WebSocket client
  wsclient.onMessage(wsMessageHandler);
  wsclient.onEvent(wsEventHandler);
  if (wsclient.connect("openplotter.local", 3000, "/signalk/v1/stream?subscribe=none")) { // Connect to the WebSocket server
    Serial.println("WebSocket connected");
    display.println("WebSocket connected");
    /*if (wsclient.send("{\"context\": \"x\", \"unsubscribe\": [{\"path\": \"*\"}]}")) {
      Serial.println("Unsubscribed from all paths");
      display.println("Unsubscribed from all paths");
      display.partialUpdate();                             //Partial update the screen
     } // Unsubscribe from all paths
      */
  } else {
    Serial.println("WebSocket connection failed");
    display.println("WebSocket connection failed");
  }
  display.partialUpdate();                             //Partial update the screen

  if (wsclient.send("{\"context\":\"vessels.self\",\"subscribe\":[{\"path\":\"environment.depth.belowSurface\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"navigation.courseOverGroundTrue\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"navigation.speedThroughWater\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"navigation.speedOverGround\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"environment.outside.pressure\",\"period\":10000,\"format\":\"delta\",\"policy\":\"fixed\",\"minPeriod\":1000},{\"path\":\"environment.barometer.trend\",\"period\":10000,\"format\":\"delta\",\"policy\":\"fixed\",\"minPeriod\":1000},{\"path\":\"navigation.position\",\"period\":3000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"environment.wind.angleTrueGround\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"environment.wind.angleApparent\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"environment.wind.speedOverGround\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000},{\"path\":\"environment.wind.speedApparent\",\"period\":1000,\"format\":\"delta\",\"policy\":\"ideal\",\"minPeriod\":1000}]}")) { // Send the JSON file to the WebSocket server
    Serial.println("JSON subscribe file sent successfully");
    display.println("JSON subscribe file sent successfully");
  } else {
    Serial.println("Failed to send JSON subscribe file");
    display.println("Failed to send JSON subscribe file");
  }

  display.partialUpdate(); 

  delay(1000);                                        //Wait for a second
  display.clearDisplay();                             //Clear everything in frame buffer
  display.setCursor(50, 300);
  display.setTextSize(8);                             //Set print cursor to new position
  display.println("Willkommen");                       
  display.setCursor(30, 400);
  display.println("auf Avalon!");
  display.display();                                  //Refresh the screen
  delay(3000);                                        //Wait for 2 seconds
  display.clearDisplay();                             //Clear everything in frame buffer

  Serial.println("End Connection Stuff");
  initDisplayText(); 
}


void loop() {
  //Serial.println("Looping..."); // Print to serial monitor

 //watchdog
 unsigned long lastWatchdog = millis();
 const unsigned long watchdogInterval = 600000; // 10 minutes in milliseconds
  if (millis() - lastWatchdog > watchdogInterval) {
      lastWatchdog = millis();
      Serial.println("Watchdog triggered, Init Display new...");
      initDisplayText();
    }

wsclient.poll(); // Poll the WebSocket client for incoming messages
  

}