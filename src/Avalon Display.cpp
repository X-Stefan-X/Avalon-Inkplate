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
  Serial.println("initDisplayText");
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

  Serial.println("End initDisplayText");
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
  display.print('\n');
  display.print("Connecting to ");                    //Print the name of WiFi network
  display.print(WIFI_SSID);
  Serial.println("Connecting to ");                    //Print the name of WiFi network
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);                             //Try to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);                                      //While it is connecting to network, display dot every second, just to know that Inkplate is alive.
    display.print('.');
    display.partialUpdate();
  }
  display.print("connected");                         //If it's connected, notify user
  Serial.println("connected");                         //If it's connected, notify user
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

  Serial.println("End Wifi Connection");
  initDisplayText(); 
}


void ErrorHandlingValues(String ErrorText, int Postion_x, int Postion_y) {
  Serial.println("deserializeJson failed");
  Serial.println(ErrorText);
  display.setCursor(Postion_x, Postion_y);
  display.setTextSize(2);
  display.print("deserializeJson failed");
  display.setCursor(Postion_x, Postion_y + 20);
  display.print(ErrorText);
  display.partialUpdate();
}

void ValueAndDisplayHandling(HTTPClient &http, int JsonLength, int Postion_x, int Postion_y, char* Called) {
  { // Now try to connect to some web page (in this example www.example.com. And yes, this is a valid Web page :))
    if (http.GET() > 0) {
      Serial.printf("GET request successful for %s ", Called);
      // Stream& input -- richtige Länge eintragen.
      StaticJsonDocument<192> doc;
      DeserializationError error = deserializeJson(doc, http.getStream());
      if (error) {
        ErrorHandlingValues(error.c_str(), Postion_x, Postion_y);
      }
      else
      {
        const char *timestamp = doc["timestamp"];                  // "2024-04-26T18:24:41.384Z"
        Serial.printf("Timestamp: %s", timestamp);

        // Print values
        display.setCursor(Postion_x, Postion_y);
        if (strcmp(Called, "position") == 0) {
          // Latitude and Longitude are ptinted in two lines
          const char *meta_description = doc["meta"]["description"]; // "Depth related data"
          //Serial.printf("Meta_description: %s", meta_description);   
          double latitude = doc["value"]["latitude"];                               // 245.01020000000003
          //Serial.printf("latitude: %f", latitude);  
          double longitude = doc["value"]["longitude"];                               // 245.01020000000003
          //Serial.printf("longitude: %f", longitude);
          const char *source = doc["$source"];                       // "simulator.0"
          //Serial.printf("Source: %s", source);
          const char *unit = doc["meta"]["units"];                            // "m"
          //Serial.printf("Unit: %s", unit);
          Serial.println("Printing Latitude and Longitude");
          display.setTextSize(3);
          display.printf("Lat: %f",latitude);
          display.setCursor(Postion_x, Postion_y + 20);
          display.printf("Lon: %f", longitude);
        } else if (strcmp(Called, "pressuretrend") == 0) {
            const char *meta_description = doc["meta"]["description"]; // "Depth related data"
            //Serial.printf("Meta_description: %s", meta_description);     
            double severity = doc["value"]["severity"];                               // 245.01020000000003
            //Serial.printf("severity: %f", severity);
            const char *tendency = doc["value"]["tendency"];                               // 245.01020000000003
            //Serial.printf("tendency: %s", tendency);  
            const char *changerate = doc["value"]["changerate"];                               // 245.01020000000003
            //Serial.printf("changerate: %s", changerate);
            const char *source = doc["$source"];                       // "simulator.0"
            //Serial.printf("Source: %s", source);
            const char *unit = doc["meta"]["units"];                            // "m"
            //Serial.printf("Unit: %s", unit);
            Serial.println("Printing Barometer Trend");
            display.setTextSize(3);
            display.printf("T: %1.0f - %s", severity, tendency);
            display.setCursor(Postion_x, Postion_y + 20);
            display.printf("R: %s", changerate);
        } 
        else {
          // All other values are printed in one line
          // Try to convert the value to a number
          const char *meta_description = doc["meta"]["description"]; // "Depth related data"
          //Serial.printf("Meta_description: %s", meta_description);     
          double value = doc["value"];                               // 245.01020000000003
          //Serial.printf("Value: %f ", value);
          const char *source = doc["$source"];                       // "simulator.0"
          //Serial.printf("Source: %s", source);
          const char *timestamp = doc["timestamp"];                  // "2024-04-26T18:24:41.384Z"
          //Serial.printf("Timestamp: %s", timestamp);
          const char *unit = doc["meta"]["units"];                            // "m"
          //Serial.printf("Unit: %s", unit);
          
          if (strcmp(unit, "rad") == 0) {
            value = value * RAD_TO_DEG;
            unit = "°";
          } else if (strcmp(unit, "m/s") == 0) {
            value = value * 1.94384;
            unit = "kn";
          } else if (strcmp(unit, "Pa") == 0) {
            value = value / 100;
            unit = "hPa";
          } else if (strcmp(unit, "m") == 0)
          { if (strcmp(Called, "depth") == 0) {}
          else {
            value = value * 0.00053996;
            unit = "Nm";
          }
          } else if (strcmp(unit, "K") == 0) { 
            value = value - 273.15;
            unit = "°C";
          } else if (strcmp(unit, "ratio") == 0) {
            value = value * 100;
            unit = "%";
          }  
          Serial.print("New unit: ");
          Serial.println(unit);
          display.setTextSize(12);
          
          if (strcmp(Called, "depth") == 0 || strcmp(Called, "watertemp") == 0 || strcmp(Called, "windspeed") == 0) {
            display.printf("%2.1f", value);
            Serial.printf("Ausgabe: %2.1f \n", value);
          } else if (strcmp(Called, "stw") == 0 || strcmp(Called, "sog") == 0) {
            display.printf("%1.1f", value);
            Serial.printf("Ausgabe: %1.1f \n", value);
          } else if (strcmp(Called, "windangle") == 0 || strcmp(Called, "cog") == 0 ) {
            display.printf("%3.0f", value);
            Serial.printf("Ausgabe: %3.0f \n", value);
          } else {
            if (strcmp(Called, "pressure") == 0) {display.setTextSize(8);}
            display.printf("%4.0f", value);
            Serial.printf("Ausgabe: %4.0f \n", value);
          }
        }
        // Put the timestamp on the next line
        if (strcmp(Called, "pressuretrend") != 0) {
          if (strcmp(Called, "pressure") == 0) {display.setCursor(Postion_x, Postion_y + 70);}
          else {display.setCursor(Postion_x, Postion_y + 100);}
          
          display.setTextSize(1);
          display.print(timestamp);
        }
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
    Serial.println("WiFi not connected");
    display.clearDisplay();
    display.setTextSize(3);
    display.print("WiFi not connected");
    display.partialUpdate();
    delay(1000);
    return;
  }

  for (int i = 0; i < 20000; i++)
  {   
    //Depth
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/depth/belowSurface/")) {
      ValueAndDisplayHandling(http, 192, 3, 25, "depth"); 
      }
    // COG
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/navigation/courseOverGroundTrue/")) {
      ValueAndDisplayHandling(http, 192, 303, 25, "cog");
      }
  //STW 
  if(http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/navigation/speedThroughWater/")) {
    ValueAndDisplayHandling(http, 192, 3, 185, "stw");
    } 
    //SOG
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/navigation/speedOverGround/")) {
      ValueAndDisplayHandling(http, 192, 303, 185, "sog");
    }
  //environment.outside.pressure
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/outside/pressure/")) {
      ValueAndDisplayHandling(http, 192, 3, 345, "pressure");
    }
    
    //environment.barometer.trend
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/barometer/trend/")) {
      ValueAndDisplayHandling(http, 192, 3, 425, "pressuretrend");
    }
 //navigation.position
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/navigation/position/")) {
      ValueAndDisplayHandling(http, 192, 303, 345, "position");
    }
    //environment.wind.angleTrueGround 
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/wind/angleTrueGround/")) {
      ValueAndDisplayHandling(http, 192, 3, 505, "windangle");
    }
  //environment.wind.angleApparent
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/wind/angleApparent/")) {
      ValueAndDisplayHandling(http, 192, 303, 505, "windangle");
    }
  //environment.wind.speedOverGround
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/wind/speedOverGround/")) {
      ValueAndDisplayHandling(http, 192, 3, 665, "windspeed");
    }
  //environment.wind.speedApparent
    if (http.begin("http://openplotter:3000/signalk/v1/api/vessels/self/environment/wind/speedApparent/")) {
      ValueAndDisplayHandling(http, 192, 303, 665, "windspeed");
    }
 
  }

// Display neu aufbauen
initDisplayText();

//http.end();
}