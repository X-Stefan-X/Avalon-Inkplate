/*
   MEMORY-OPTIMIZED Inkplate Display for Avalon
   - WebSocket SignalK with STATIC subscribe message (no JSON serialization)
   - Minimal RAM usage
   - FreeRTOS Multitasking
*/

#include "Inkplate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ESPmDNS.h>
#include "ArduinoJson.h"
#include "Arduino.h"
#include "Fonts/FreeMonoBold24pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "IMG_8906.h"

// Connection settings — werden ggf. durch mDNS-Discovery überschrieben
char signalkHost[64] = "openplotter-test.local";
int signalkPort = 3000;
// Update intervals
const unsigned long SLOW_UPDATE_INTERVAL = 10000;
const unsigned long FAST_UPDATE_INTERVAL = 200;
// Display
const unsigned long NAV_UPDATE_INTERVAL = 500;
const unsigned long ENV_UPDATE_INTERVAL = 3000;



QueueHandle_t wsMessageQueue;

Inkplate display(INKPLATE_1BIT);
WebSocketsClient webSocket;

// Mutex for display access
SemaphoreHandle_t displayMutex;

// Data structures
struct NavigationData {
  double depth;
  double cog;
  double stw;
  double sog;
  double twa;
  double awa;
  double tws;
  double aws;
  bool anyChanged;
};

struct EnvironmentData {
  double pressure;
  double latitude;
  double longitude;
  double pressureTrendSeverity;
  char pressureTrendTendency[20];
  char pressureTrendChangerate[20];
  bool anyChanged;
};

struct WSMessage {
  char* data;
  size_t length;
};

NavigationData navData = {0};
EnvironmentData envData = {0};

bool wsConnected = false;
bool displayErrorMode = false;
bool wifiEverConnected = false;
unsigned long lastWsHeartbeat = 0; 
const unsigned long WS_TIMEOUT = 30000;  // 30s Timeout

// Forward declarations
void webSocketTask(void *parameter);
void displayUpdateTask(void *parameter);
void slowDataTask(void *parameter);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void subscribeToSignalK();
bool fetchSingleValue(HTTPClient &http, const char* url);
void displayNavigationData();
void displayEnvironmentData();
double convertUnit(double value, const char* unit, const char* dataType);
void drawCenteredText(int x, int y, int w, int h, const char* text);
double safeParsePos(const char* ptr, const char* key);
void DisplayError();
void discoverSignalK();
void onWiFiEvent(WiFiEvent_t event);

// static buffer
char rxBuffer[4096];
volatile bool rxDataReady = false;
volatile size_t rxLen = 0;

void initDisplayText() {
  if (xSemaphoreTake(displayMutex, portMAX_DELAY) == pdTRUE) {
    display.clearDisplay();
    display.display();
    display.setFont();
    display.setTextColor(BLACK, WHITE);
    display.setTextSize(3);
    display.setCursor(3, 0);
    display.print("Tiefe in m");
    display.setCursor(303, 0);
    display.print("COG in Grad");

    display.drawThickLine(0, 157, 600, 157, 1, 1);
    display.setCursor(3, 160);
    display.print("STW in kn");
    display.setCursor(303, 160);
    display.print("SOG in kn");

    display.drawThickLine(0, 317, 600, 317, 1, 1);
    display.setCursor(5, 320);
    display.print("Luftdruck");
    display.setCursor(305, 320);
    display.print("Position Lat/Lon");

    display.drawThickLine(0, 477, 600, 477, 1, 1);
    display.setCursor(3, 480);
    display.print("TWA in Grad");
    display.setCursor(303, 480);
    display.print("AWA in Grad");

    display.drawThickLine(0, 637, 600, 637, 1, 1);
    display.setCursor(3, 640);
    display.print("TWS in kn");
    display.setCursor(303, 640);
    display.print("AWS in kn");

    display.drawThickLine(280, 0, 280, 800, 1, 20);

    display.setTextSize(3);
    display.setTextColor(WHITE, BLACK);
    display.cp437(true);
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
    display.print("6");

    display.display();

    xSemaphoreGive(displayMutex);
  }
  
  Serial.println("Display drawed for Data.");
}

void DisplayError() {
  display.clearDisplay();
  display.display();

  display.setTextColor(BLACK, WHITE);
  display.setTextSize(2);
  display.println("WS Disconneted!");
  display.println("try reconnect!");
  display.display();
  Serial.println("Displayed WS Connection Error!");
}

void discoverSignalK() {
  Serial.println("mDNS: suche SignalK-Server...");
  int n = MDNS.queryService("signalk-ws", "tcp");
  if (n > 0) {
    strncpy(signalkHost, MDNS.hostname(0).c_str(), sizeof(signalkHost) - 1);
    signalkPort = MDNS.port(0);
    Serial.printf("mDNS: gefunden: %s:%d\n", signalkHost, signalkPort);
  } else {
    Serial.printf("mDNS: nichts gefunden, Fallback: %s:%d\n", signalkHost, signalkPort);
  }
}

void setup() {
  Serial.begin(115200);
  display.begin();
  display.clearDisplay();
  display.setRotation(1);
  display.display();
  display.setTextSize(3);
  display.setCursor(0, 0);
  display.setTextColor(BLACK, WHITE);

  Serial.println("Starting Avalon Display...");
  display.println("(c) Stefan Krax 2025, 2026");
  display.println("Starting Avalon Display...");

  display.println("Scanning WiFi...");
  Serial.println("Scanning WiFi...");
  display.display();

  int n = WiFi.scanNetworks();
  if (n == 0) {
    display.print("No WiFi!");
    display.partialUpdate();
    while (true);
  } else {
    if (n > 10) n = 10;
    for (int i = 0; i < n; i++) {
      display.print(WiFi.SSID(i));
      display.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? 'O' : '*');
      display.print('\n');
      Serial.println(WiFi.SSID(i));
    }
    display.partialUpdate();
    delay(2000);
  }

  WiFi.onEvent(onWiFiEvent);

  display.print('\n');
  display.print("Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    display.print('.');
    display.partialUpdate();
  }
  
  Serial.println("WiFi connected.");
  display.print("OK");
  display.partialUpdate();
  delay(100);
  discoverSignalK();
  display.clearDisplay();
  display.drawImage(IMG_8906, 0, 0, IMG_8906_w, IMG_8906_h);
  display.display();
  display.setCursor(50, 300);
  display.setFont(&FreeMonoBold24pt7b);
  display.setTextSize(1);
  display.println("Willkommen");
  display.setCursor(30, 400);
  display.println("auf Avalon!");
  display.display();
  delay(3000);
  display.setFont();

  Serial.printf("Free heap before tasks: %d bytes\n", ESP.getFreeHeap());

  displayMutex = xSemaphoreCreateMutex();
  if (displayMutex == NULL) {
    Serial.println("*** DISPLAY MUTEX FAILED TO CREATE ***");
  }
  

  webSocket.begin(signalkHost, signalkPort, "/signalk/v1/stream?subscribe=none");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);

  // Stack-Größen massiv erhöht für Stabilität
  xTaskCreatePinnedToCore(webSocketTask, "WebSocketTask", 10000, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(displayUpdateTask, "DisplayTask", 10000, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(slowDataTask, "SlowDataTask", 6000, NULL, 1, NULL, 1);

  Serial.println("Tasks created");
  Serial.printf("Free heap after tasks: %d bytes\n", ESP.getFreeHeap());
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      wsConnected = false;
      displayErrorMode = true;
      lastWsHeartbeat = millis();
      break;
      
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected");
      wsConnected = true;
      displayErrorMode = false;
      lastWsHeartbeat = millis();
      initDisplayText();
      break;
      
    case WStype_TEXT:
      // Check payload pointer and length
      if (payload != NULL && length > 0 && length < 4095 && !rxDataReady) {
        memcpy(rxBuffer, payload, length);
        rxBuffer[length] = '\0'; // Null-Terminierung
        rxLen = length;
        rxDataReady = true;
      }
      break;

    default:
      break;
  }
}

void subscribePath(const char* path, int period) {
  String msg = "{\"context\":\"vessels.self\",\"subscribe\":[{\"path\":\"";
  msg += path;
  msg += "\",\"period\":";
  msg += String(period);
  msg += ",\"format\":\"delta\",\"policy\":\"fixed\"}]}";  // CHANGED: instant -> fixed
  webSocket.sendTXT(msg);
  delay(100);
}

double safeParsePos(const char* ptr, const char* key) {
  if (ptr == NULL) return -9999.0;
  ptr += strlen(key);
  while (*ptr == ' ' || *ptr == '\t') ptr++;
  if (!*ptr || (!isdigit((unsigned char)*ptr) && *ptr != '-' && *ptr != '.')) return -9999.0;
  char* endPtr;
  double val = strtod(ptr, &endPtr);
  return (endPtr > ptr) ? val : -9999.0;
}

void subscribeToSignalK() {
  Serial.println("Subscribing to SignalK");
  
  subscribePath("environment.depth.belowSurface", FAST_UPDATE_INTERVAL);
  subscribePath("navigation.courseOverGroundTrue", FAST_UPDATE_INTERVAL);
  subscribePath("navigation.speedThroughWater", FAST_UPDATE_INTERVAL);
  subscribePath("navigation.speedOverGround", FAST_UPDATE_INTERVAL);
  subscribePath("environment.wind.angleTrueWater", FAST_UPDATE_INTERVAL);
  subscribePath("environment.wind.angleApparent", FAST_UPDATE_INTERVAL);
  subscribePath("environment.wind.speedTrue", FAST_UPDATE_INTERVAL);
  subscribePath("environment.wind.speedApparent", FAST_UPDATE_INTERVAL);
  subscribePath("environment.outside.pressure", SLOW_UPDATE_INTERVAL);
  subscribePath("navigation.position", SLOW_UPDATE_INTERVAL);
  
  Serial.println("All subscriptions sent");
}

void webSocketTask(void *parameter) {
  bool subscribed = false;
  
  while (true) {
    webSocket.loop();
    
    if (wsConnected && !subscribed) {
      Serial.println("Calling subscribeToSignalK from task");
      subscribeToSignalK();
      subscribed = true;
    }
    
    if (!wsConnected) subscribed = false;
    
    if (rxDataReady) {
      // Basic validation
      if (strncmp(rxBuffer, "{", 1) == 0 && strstr(rxBuffer, "updates") != NULL) {
        
        auto findValPtr = [](const char* buf, const char* path) -> const char* {
          const char* pathPtr = strstr(buf, path);
          if (pathPtr == NULL) return NULL;
          const char* valPtr = strstr(pathPtr, "\"value\":");
          return (valPtr && valPtr > pathPtr) ? valPtr : NULL;
        };

        auto safeParse = [](const char* ptr) -> double {
          if (ptr == NULL) return -9999.0;
          ptr += 8; // Skip "value":
          // Skip whitespace, quotes, colons
          while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == ':' || *ptr == '"')) ptr++;
          if (!*ptr) return -9999.0;
          
          char* endPtr;
          double val = strtod(ptr, &endPtr);
          if (ptr == endPtr || endPtr == ptr) return -9999.0;
          return val;
        };

        // ATOMARE Updates ohne Mutex (thread-safe für doubles/bools)
        double val;
        
        val = safeParse(findValPtr(rxBuffer, "environment.depth.belowSurface"));
        if (val != -9999.0) { navData.depth = convertUnit(val, "m", "depth"); navData.anyChanged = true; }

        val = safeParse(findValPtr(rxBuffer, "navigation.courseOverGroundTrue"));
        if (val != -9999.0) { navData.cog = convertUnit(val, "rad", "cog"); navData.anyChanged = true; }
        
        val = safeParse(findValPtr(rxBuffer, "navigation.speedThroughWater"));
        if (val != -9999.0) { navData.stw = convertUnit(val, "m/s", "stw"); navData.anyChanged = true; }
        
        val = safeParse(findValPtr(rxBuffer, "navigation.speedOverGround"));
        if (val != -9999.0) { navData.sog = convertUnit(val, "m/s", "sog"); navData.anyChanged = true; }
        
        val = safeParse(findValPtr(rxBuffer, "environment.wind.angleTrueWater"));
        if (val != -9999.0) { navData.twa = convertUnit(val, "rad", "windangle"); navData.anyChanged = true; }
        
        val = safeParse(findValPtr(rxBuffer, "environment.wind.angleApparent"));
        if (val != -9999.0) { navData.awa = convertUnit(val, "rad", "windangle"); navData.anyChanged = true; }
        
        val = safeParse(findValPtr(rxBuffer, "environment.wind.speedTrue"));
        if (val != -9999.0) { navData.tws = convertUnit(val, "m/s", "windspeed"); navData.anyChanged = true; }
        
        val = safeParse(findValPtr(rxBuffer, "environment.wind.speedApparent"));
        if (val != -9999.0) { navData.aws = convertUnit(val, "m/s", "windspeed"); navData.anyChanged = true; }
        
        val = safeParse(findValPtr(rxBuffer, "environment.outside.pressure"));
        if (val != -9999.0) { envData.pressure = convertUnit(val, "Pa", "pressure"); envData.anyChanged = true; }

        // SignalK DELTA Position (genau für dein Format!)
        const char* pathPtr = strstr(rxBuffer, "\"path\":\"navigation.position\"");
        if (pathPtr) {
          
          // "value":{ nach path finden
          const char* valuePtr = strstr(pathPtr + 28, "\"value\":{");  // +28 skippt "path":"navigation.position"
          if (valuePtr) {
            
            // Innerhalb value:{} nach lat/lon
            const char* latPtr = strstr(valuePtr + 8, "\"latitude\":");  // +8 skippt "value":{
            const char* lonPtr = strstr(valuePtr, "\"longitude\":");
            
            if (latPtr && lonPtr) {
              double lat = safeParsePos(latPtr, "\"latitude\":");
              double lon = safeParsePos(lonPtr, "\"longitude\":");
              
              if (lat > -90 && lat < 90 && lon > -180 && lon < 180) {
                envData.latitude = lat;
                envData.longitude = lon;
                envData.anyChanged = true;
                Serial.printf("GPS PARSED: %.6f°N %.6f°E\n", lat, lon);
              }
            }
          }
        } else {
          Serial.printf("Parsed: D=%.1f COG=%.0f STW=%.1f SOG=%.1f TWA=%.0f AWA=%.0f TWS=%.1f AWS=%.1f Pressure=%.0f\n", navData.depth, navData.cog, navData.stw, navData.sog, navData.twa, navData.awa, navData.tws, navData.aws, envData.pressure);
        }   
      }
      rxDataReady = false;
    }
    
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void displayUpdateTask(void *parameter) {
  unsigned long lastNavUpdate = 0;
  unsigned long lastEnvUpdate = 0;
  bool errorShown = false;

  while (true) {
    unsigned long now = millis();

    if (displayErrorMode && !errorShown) {
      if (xSemaphoreTake(displayMutex, 500 / portTICK_PERIOD_MS) == pdTRUE) {
        DisplayError();
        xSemaphoreGive(displayMutex);
        errorShown = true;
      }
    }
    if (!displayErrorMode) errorShown = false;

    if (navData.anyChanged && (now - lastNavUpdate >= NAV_UPDATE_INTERVAL)) {
      
      // 500ms Timeout!
      if (xSemaphoreTake(displayMutex, 500 / portTICK_PERIOD_MS) == pdTRUE) {
        displayNavigationData();
        xSemaphoreGive(displayMutex);
        navData.anyChanged = false;
        lastNavUpdate = now;
        Serial.println("Display updated OK");
      } else {
        Serial.println("*** DISPLAY MUTEX TIMEOUT ***");
      }
    }
    if (envData.anyChanged && (now - lastEnvUpdate >= ENV_UPDATE_INTERVAL)) {
      if (xSemaphoreTake(displayMutex, 500 / portTICK_PERIOD_MS) == pdTRUE) {
        displayEnvironmentData();
        xSemaphoreGive(displayMutex);
        envData.anyChanged = false;
        lastEnvUpdate = now;
        Serial.println("ENV display OK");
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


void slowDataTask(void *parameter) {
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(2000);
  http.setReuse(true);

  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    }

    char barometerUrl[128];
    snprintf(barometerUrl, sizeof(barometerUrl),
             "http://%s:%d/signalk/v1/api/vessels/self/environment/barometer/trend",
             signalkHost, signalkPort);
    if (fetchSingleValue(http, barometerUrl)) {
      envData.anyChanged = true;
    }

    vTaskDelay(SLOW_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

bool fetchSingleValue(HTTPClient &http, const char* url) {
  if (!http.begin(url)) return false;

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    // Use SMALL JsonDocument
    JsonDocument doc;
    
    DeserializationError error = deserializeJson(doc, http.getStream());

    if (error) {
      Serial.printf("JSON error: %s\n", error.c_str());
      return false;
    }

    if (doc["value"].is<JsonObject>()) {
      JsonObject trendValue = doc["value"];

      if (trendValue["severity"].is<double>()) {
        envData.pressureTrendSeverity = trendValue["severity"];
      }

      if (trendValue["tendency"].is<const char*>()) {
        strncpy(envData.pressureTrendTendency, trendValue["tendency"], 19);
      }

      if (trendValue["changerate"].is<const char*>()) {
        strncpy(envData.pressureTrendChangerate, trendValue["changerate"], 19);
      }

      return true;
    }
  }

  return false;
}

double convertUnit(double value, const char* unit, const char* dataType) {
  if (strcmp(unit, "rad") == 0) {
    value = value * RAD_TO_DEG;
  } else if (strcmp(unit, "m/s") == 0) {
    value = value * 1.94384;
  } else if (strcmp(unit, "Pa") == 0) {
    value = value / 100;
  } else if (strcmp(unit, "m") == 0 && strcmp(dataType, "depth") != 0) {
    value = value * 0.00053996;
  } else if (strcmp(unit, "K") == 0) {
    value = value - 273.15;
  } else if (strcmp(unit, "ratio") == 0) {
    value = value * 100;
  } 
  return value;
}


void displayNavigationData() {
  
  display.setTextColor(BLACK, WHITE);
  display.setFont(&FreeSans24pt7b);
  display.setTextSize(3);
  char buffer[20];
 
  display.fillRect(0, 25, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%.1f", navData.depth);
  display.setCursor(0, 125);
  display.print(buffer);

  display.fillRect(300, 25, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%3.0f", navData.cog);
  display.setCursor(300, 125);
  display.print(buffer);

  display.fillRect(0, 200, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%.1f", navData.stw);
  display.setCursor(0, 300);
  display.print(buffer);

  display.fillRect(300, 200, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%.1f", navData.sog);
  display.setCursor(300, 300);
  display.print(buffer);

  //Pressure + Position

  display.fillRect(0, 500, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%3.0f", navData.twa);
  display.setCursor(0, 600);
  display.print(buffer);

  display.fillRect(300, 500, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%3.0f", navData.awa);
  display.setCursor(300, 600);
  display.print(buffer);

  display.fillRect(0, 695, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%.1f", navData.tws);
  display.setCursor(0, 795);
  display.print(buffer);

  display.fillRect(300, 695, 270, 110, WHITE);
  snprintf(buffer, sizeof(buffer), "%.1f", navData.aws);
  display.setCursor(300, 795);
  display.print(buffer);

  display.partialUpdate();
}


void displayEnvironmentData() {
  display.setTextColor(BLACK, WHITE);
  
  char buffer[20];

  display.setTextSize(2);
  // Luftdruck + Trend 
  display.fillRect(0, 345, 270, 130, WHITE);
  snprintf(buffer, sizeof(buffer), "%4.0f", envData.pressure);
  display.setCursor(30, 410);
  display.print(buffer);
  
  display.setTextSize(1);
  display.setFont(&FreeSans12pt7b);
  snprintf(buffer, sizeof(buffer), "T:%.0f %s", envData.pressureTrendSeverity, envData.pressureTrendTendency);
  display.setCursor(1, 440);
  display.print(buffer);
  
  snprintf(buffer, sizeof(buffer), "R:%s", envData.pressureTrendChangerate);
  display.setCursor(1, 465);
  display.print(buffer);

  // Position 
  display.setFont(&FreeSans24pt7b);
  display.fillRect(300, 345, 300, 130, WHITE);
  snprintf(buffer, sizeof(buffer), "%f", envData.latitude);
  display.setCursor(305, 390);
  display.print(buffer);
  snprintf(buffer, sizeof(buffer), "%.5f", envData.longitude);
  display.setCursor(305, 430);
  display.print(buffer);

  display.partialUpdate();
}

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (wifiEverConnected) {
        Serial.println("WiFi lost, reconnecting...");
        WiFi.reconnect();
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      if (!wifiEverConnected) {
        wifiEverConnected = true;
      } else {
        Serial.println("WiFi reconnected.");
        discoverSignalK();
      }
      break;
    default:
      break;
  }
}

void loop() {
  static unsigned long lastStatusCheck = 0;
  if (millis() - lastStatusCheck > 30000) {
    Serial.printf("Free heap: %d | WS: %s\n", ESP.getFreeHeap(), wsConnected ? "OK" : "NO");
    lastStatusCheck = millis();
  }
  delay(1000);
}