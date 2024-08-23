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

// Injected now as build_flags for all common examples
//#define ssid ""                     //Name of the WiFi network (SSID) that you want to connect Inkplate to
//#define pass ""                     //Password of that WiFi network

Inkplate display(INKPLATE_1BIT);    //Create an object on Inkplate library and also set library into 1 Bit mode (Monochrome)




void initDisplayText() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 0);
  display.print("Tiefe");
  display.setCursor(400, 0);
  display.print("TWA");
  display.setCursor(0, 300);
  display.print("SOG");
  display.setCursor(400, 300);
  display.print("TWS");
  display.display();
}

void setup() {
  display.begin();                  //Init Inkplate library (you should call this function ONLY ONCE)
  display.clearDisplay();           //Clear frame buffer of display
  display.display();                //Put clear image on display
  display.setTextSize(2);           //Set text scaling to two (text will be two times bigger)
  display.setCursor(0, 0);          //Set print position
  display.setTextColor(BLACK, WHITE);                 //Set text color to black and background color to white
  display.println("Scanning for WiFi networks...");    //Write text
  display.display();                                  //Send everything to display (refresh display)

  int n = WiFi.scanNetworks();                        //Start searching WiFi networks and put the nubmer of found WiFi networks in variable n
  if (n == 0) {                                       //If you did not find any network, show the message and stop the program.
    display.print("No WiFi networks found!");
    display.partialUpdate();
    while (true);
  } else {
    if (n > 10) n = 0;                                //If you did find, print name (SSID), encryption and signal strength of first 10 networks
    for (int i = 0; i < n; i++) {
      display.print(WiFi.SSID(i));
      display.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? 'O' : '*');
      display.print('\n');
      display.print(WiFi.RSSI(i), DEC);
    }
    display.partialUpdate();                          //(Partial) refresh thescreen
  }

  display.clearDisplay();                             //Clear everything in frame buffer
  display.setCursor(0,0);                             //Set print cursor to new position
  display.print("Connecting to ");                    //Print the name of WiFi network
  display.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);                             //Try to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);                                      //While it is connecting to network, display dot every second, just to know that Inkplate is alive.
    display.print('.');
    display.partialUpdate();
  }
  display.print("connected");                         //If it's connected, notify user
  display.partialUpdate();
  delay(1000);                                        //Wait for a second
  display.clearDisplay();                             //Clear everything in frame buffer
  display.setCursor(100,160);
  display.setTextSize(10);                             //Set print cursor to new position
  display.println("Willkommen");                       
  display.setCursor(80, 300);
  display.println("auf Avalon!");
  display.display();                                  //Refresh the screen
  delay(5000);                                        //Wait for 2 seconds
  display.clearDisplay();                             //Clear everything in frame buffer

  initDisplayText(); 
}


void ErrorHandlingValues(String ErrorText, int Postion_x, int Postion_y) {
  display.setCursor(Postion_x, Postion_y + 5);
  display.setTextSize(2);
  display.print("deserializeJson() failed: ");
  display.setCursor(Postion_x +10, Postion_y + 5);
  display.print(ErrorText);
  display.partialUpdate();
}

void ValueAndDisplayHandling(HTTPClient &http, int JsonLength, int Postion_x, int Postion_y) {
  { // Now try to connect to some web page (in this example www.example.com. And yes, this is a valid Web page :))
    if (http.GET() > 0) {
      // Stream& input -- richtige Länge eintragen.
      StaticJsonDocument<192> doc;
      DeserializationError error = deserializeJson(doc, http.getStream());
      if (error) {
        ErrorHandlingValues(error.c_str(), Postion_x, Postion_y);
      }
      else
      {
      const char *meta_description = doc["meta"]["description"]; // "Depth related data"
      double value = doc["value"];                               // 245.01020000000003
      const char *source = doc["$source"];                       // "simulator.0"
      const char *timestamp = doc["timestamp"];                  // "2024-04-26T18:24:41.384Z"

      // Print values
      display.setCursor(Postion_x, Postion_y);
      display.setTextSize(10);
      display.print(value);
      display.setCursor(Postion_x, Postion_y + 100);
      display.setTextSize(1);
      display.print(timestamp);
      display.partialUpdate();
      }
    }
  }
}

/**
 * The main loop function of the program.
 * This function is executed repeatedly in an infinite loop.
 * It connects to a web page using HTTP and retrieves data from it.
 * The retrieved data is then displayed on an Inkplate display.
 */
void loop() {

//Get Part
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(1000);
  http.setReuse(true);
// Error Strings
  if (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.print("WiFi not connected");
    display.partialUpdate();
    return;
  }
  
  //Depth
  if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/depth/")) {
    ValueAndDisplayHandling(http, 192, 0, 30); 
    }
  // TWA
  if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/twa/")) {
    ValueAndDisplayHandling(http, 192, 400, 30);
    }
//Speed 
if(http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/navigation/speedOverGround/")) {
  ValueAndDisplayHandling(http, 192, 0, 330);
  } 
  //tws
  if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/tws/")) {
    ValueAndDisplayHandling(http, 192, 400, 330);
  }

//http.end();
}