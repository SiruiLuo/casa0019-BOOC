//#include <GFX.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

Servo myServo;  

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// or select the display constructor line in one of the following files (old style):
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"
#include "images.h"

#define BUTTON1  25  
#define BUTTON2  26  
#define BUTTON3  27  

int currentPage = 0;     
const int totalPages = 5; 

int bt1count = 0;
int bt2count = 0;
int bt3count = 0;
// alternately you can copy the constructor from GxEPD2_display_selection.h or GxEPD2_display_selection_added.h to here
// e.g. for Wemos D1 mini:
//GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2, /*BUSY=D2*/ 4)); // GDEH0154D67

// for handling alternative SPI pins (ESP32, RP2040) see example GxEPD2_Example.ino


struct Time {
  int tm;                 
  int sensors_absent;     
  int sensors_occupied;   
  int sensors_total;      
};

struct Averages {
  Time hours[24];         
};

struct Map {
  int id;                 
  String name;            
  int sensors_absent;     
  int sensors_occupied; 
};

struct Survey {
  int id;                 
  String name;            
  int sensors_occupied;   
  int sensors_absent;     
  int sensors_other;      
  Map maps[10];           
  Averages averages;     
};

struct Root {
  bool ok;                
  Survey surveys[10];     
};

Root myRoot;              


const char* mqttServer = "mqtt.cetools.org"; 
const int mqttPort = 1884;                   
const char* mqttUser = "student";          
const char* mqttPassword = "ce2021-mqtt-forget-whale";        
const char* mqttTopic = "student/CASA0014/ucfnuoa/";        

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousMillis = 0; 
const long interval = 60000;      

int surveycount = 0;

bool isFirstLoad = true;
const char* ssid = "Pixel_3221";
const char* password = "20010906";
int currentangle = 171;

// API URL
String apiUrl1 = "https://uclapi.com/workspaces/sensors/summary?survey_ids=111&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";
String apiUrl2 = "https://uclapi.com/workspaces/sensors/averages/time?days=1&survey_ids=111&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";

String apiUrl3 = "https://uclapi.com/workspaces/sensors/summary?survey_ids=119&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";
String apiUrl4 = "https://uclapi.com/workspaces/sensors/averages/time?days=1&survey_ids=119&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";

String apiUrl5 = "https://uclapi.com/workspaces/sensors/summary?survey_ids=116&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";
String apiUrl6 = "https://uclapi.com/workspaces/sensors/averages/time?days=1&survey_ids=116&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";

void setup()
{

  myServo.attach(13);  
  myServo.write(171);
  delay(1000);

  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
  
    //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqttServer, mqttPort);
  connectToMQTT();


  HTTPClient http;
  http.begin(apiUrl1); 

  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 1);
  } else {
    Serial.println("Failed to connect to API 1");
  }
  
  http.end(); 


  http.begin(apiUrl2); 

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 2); 
  } else {
    Serial.println("Failed to connect to API 2");
  }

  http.end();  

   
  http.begin(apiUrl3); 

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 1); 
  } else {
    Serial.println("Failed to connect to API 3");
  }

  http.end();  

  http.begin(apiUrl4); 

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 2); 
  } else {
    Serial.println("Failed to connect to API 4");
  }

  http.end(); 

  http.begin(apiUrl5); 

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 1); 
  } else {
    Serial.println("Failed to connect to API 5");
  }

  http.end();  

  http.begin(apiUrl6); 

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 2); 
  } else {
    Serial.println("Failed to connect to API 6");
  }

  http.end(); 

  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  helloWorld();
  display.hibernate();
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("\u6b63\u5728\u8fde\u63a5\u5230 MQTT \u670d\u52a1\u5668...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("\u5df2\u8fde\u63a5\u5230 MQTT \u670d\u52a1\u5668");
    } else {
      Serial.print("\u8fde\u63a5\u5931\u8d25\uff0c\u72b6\u6001\u7801: ");
      Serial.print(client.state());
      Serial.println(" 5\u79d2\u540e\u91cd\u8bd5...");
      delay(5000);
    }
  }
}



void updateSurveyData(Survey &currentSurvey, JsonObject &survey) {
  currentSurvey.sensors_occupied = survey["sensors_occupied"];
  currentSurvey.sensors_absent = survey["sensors_absent"];
  currentSurvey.sensors_other = survey["sensors_other"];
  
  
  JsonArray maps = survey["maps"];
  int mapIndex = 0;
  for (JsonObject map : maps) {
    Map &currentMap = currentSurvey.maps[mapIndex++];
    currentMap.id = map["id"];
    currentMap.name = String(map["name"].as<const char*>());
    currentMap.sensors_absent = map["sensors_absent"];
    currentMap.sensors_occupied = map["sensors_occupied"];
  }
}

void fetchingData(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

   
    Serial.println("Fetching data again...");
    
   
    HTTPClient http;
    http.begin(apiUrl1); 

    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 1); 
    } else {
      Serial.println("Failed to connect to API 1");
    }
    
    http.end();  

    
    http.begin(apiUrl2); 

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 2); 
    } else {
      Serial.println("Failed to connect to API 2");
    }

    http.end(); 

      
    http.begin(apiUrl3); 

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 1); 
    } else {
      Serial.println("Failed to connect to API 3");
    }

    http.end(); 

    http.begin(apiUrl4); 

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 2); 
    } else {
      Serial.println("Failed to connect to API 4");
    }

    http.end();  

    http.begin(apiUrl5); 

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 1); 
    } else {
      Serial.println("Failed to connect to API 5");
    }

    http.end();  

    http.begin(apiUrl6); 

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 2); 
    } else {
      Serial.println("Failed to connect to API 6");
    }

    http.end(); 
  }
}


void deserializeJsonData(String payload, int apiType) {
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("JSON Parsing Error: ");
    Serial.println(error.c_str());
    return;
  }

  if (apiType == 1) {
    JsonArray surveys = doc["surveys"];
    int surveyIndex = 0;
    for (JsonObject survey : surveys) {
      int surveyID = survey["id"];
      
     
      Survey *currentSurvey = nullptr;
      for (int i = 0; i < surveycount; i++) {
        if (myRoot.surveys[i].id == surveyID) {
          currentSurvey = &myRoot.surveys[i];
          break;
        }
      }
      
      if (currentSurvey == nullptr) {
        currentSurvey = &myRoot.surveys[surveycount++];
        currentSurvey->id = surveyID;
        currentSurvey->name = String(survey["name"].as<const char*>());
      }
      
      updateSurveyData(*currentSurvey, survey);

      
      String surveyData = "Survey|" +
                          String(currentSurvey->id) + "|" +
                          currentSurvey->name + "|" +
                          String(currentSurvey->sensors_occupied) + "|" +
                          String(currentSurvey->sensors_absent) + "|" +
                          String(currentSurvey->sensors_other);
      Serial.println(surveyData);
      delay(100);

      
      JsonArray maps = survey["maps"];
      for (JsonObject map : maps) {
        int mapID = map["id"];
        String mapData = "Map|" +
                         String(currentSurvey->id) + "|" +
                         String(mapID) + "|" +
                         String(map["name"].as<const char*>()) + "|" +
                         String(map["sensors_absent"]) + "|" +
                         String(map["sensors_occupied"]);
        Serial.println(mapData);
        delay(100);
      }
    }
  } else if (apiType == 2) {
    
    JsonArray surveys = doc["surveys"];
    for (JsonObject survey : surveys) {
      int surveyID = survey["survey_id"];
      Survey *currentSurvey = nullptr;
      for (int i = 0; i < surveycount; i++) {
        if (myRoot.surveys[i].id == surveyID) {
          currentSurvey = &myRoot.surveys[i];
          break;
        }
      }
      if (currentSurvey == nullptr) {
        Serial.println("Survey ID not found in averages");
        continue;
      }

      JsonObject averages = survey["averages"];
      for (JsonPair kv : averages) {
        String time = kv.key().c_str();
        if (time.endsWith(":00:00")) { 
          int hour = time.substring(0, 2).toInt();
          Time &currentTime = currentSurvey->averages.hours[hour];
          JsonObject data = kv.value();
          currentTime.tm = hour;
          currentTime.sensors_absent = data["sensors_absent"];
          currentTime.sensors_occupied = data["sensors_occupied"];
          currentTime.sensors_total = data["sensors_total"];

          
          String timeData = "Hour|" +
                            String(surveyID) + "|" +
                            String(currentTime.tm) + "|" +
                            String(currentTime.sensors_absent) + "|" +
                            String(currentTime.sensors_occupied) + "|" +
                            String(currentTime.sensors_total);
          Serial.println(timeData);
          delay(100);
        }
      }
    }
  }
}
// E-ink Screen Code

const char HelloWorld[] = "Hello World!";

void helloWorld()
{
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    //delay(5000);

    //display.fillScreen(GxEPD_WHITE);
    //display.drawXBitmap(0, 0, page111, 200, 200, GxEPD_BLACK);
    display.drawXBitmap(0, 0, monalisaData, 400, 300, GxEPD_BLACK);

    //display.drawPixel(1, 1, GxEPD_BLACK);
    //display.fillRect(10, 30, 16, 16, GxEPD_BLACK);
    //display.drawRect(10, 10, 32, 16, GxEPD_BLACK);
    //progress_bar();


    //display.
    //display.setCursor(100, 170);
    //display.print(HelloWorld);
  }
  while (display.nextPage());
}

void Occupancy111(){
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    //delay(5000);

    //display.fillScreen(GxEPD_WHITE);
    display.drawXBitmap(100, 0, page111, 200, 200, GxEPD_BLACK);

    display.setCursor(40, 230);
    display.print("Available:");
    display.setCursor(155, 230);
    display.print(String(myRoot.surveys[0].sensors_absent));
    display.setCursor(240, 230);
    display.print("Occupied:");
    display.setCursor(340, 230);
    display.print(String(myRoot.surveys[0].sensors_occupied));

    progress_bar(200, 100, 250, myRoot.surveys[0].sensors_occupied, myRoot.surveys[0].sensors_absent);

    int angle = map(myRoot.surveys[0].sensors_occupied, 0, (myRoot.surveys[0].sensors_occupied + myRoot.surveys[0].sensors_absent), 171, 9);
    if (currentangle <= angle) {
      for (int tempangle = currentangle; tempangle <= angle; tempangle++) {
      myServo.write(tempangle);  
      delay(15);  
      }
    }else{
      for (int tempangle = currentangle; tempangle >= angle; tempangle--) {
      myServo.write(tempangle);  
      delay(15);  
      }
    }
    currentangle = angle;

    //MQTT Message Sending
    if (!client.connected()) {
    connectToMQTT();
    }
    client.loop();

    String jsonMessage = "{\"button\": \"0\"}"; 
    client.publish(mqttTopic, jsonMessage.c_str());
    Serial.println("\u5df2\u53d1\u5e03JSON\u6d88\u606f: " + jsonMessage);

  }
  while (display.nextPage());
}

void Other111(){
  String AVLB0, AVLB1, AVLB2, AVLB3;

  AVLB0 = myRoot.surveys[0].maps[0].name + " " + String(myRoot.surveys[0].maps[0].sensors_absent);
  AVLB1 = myRoot.surveys[0].maps[1].name + " " + String(myRoot.surveys[0].maps[1].sensors_absent);
  AVLB2 = myRoot.surveys[0].maps[2].name + " " + String(myRoot.surveys[0].maps[2].sensors_absent);
  AVLB3 = myRoot.surveys[0].maps[3].name + " " + String(myRoot.surveys[0].maps[3].sensors_absent);

  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    //delay(5000);

    //display.fillScreen(GxEPD_WHITE);
    //display.drawXBitmap(100, 0, page111, 200, 200, GxEPD_BLACK);

    display.setCursor(40, 40);
    display.print(AVLB0);
    progress_bar(280 ,40, 45, myRoot.surveys[0].maps[0].sensors_occupied, myRoot.surveys[0].maps[0].sensors_absent);

    display.setCursor(40, 110);
    display.print(AVLB1);
    progress_bar(280, 40, 115, myRoot.surveys[0].maps[1].sensors_occupied, myRoot.surveys[0].maps[1].sensors_absent);

    display.setCursor(40, 180);
    display.print(AVLB2);
    progress_bar(280, 40, 185, myRoot.surveys[0].maps[2].sensors_occupied, myRoot.surveys[0].maps[2].sensors_absent);

    display.setCursor(40, 250);
    display.print(AVLB3);
    progress_bar(280, 40, 255, myRoot.surveys[0].maps[3].sensors_occupied, myRoot.surveys[0].maps[3].sensors_absent);

    /*display.setCursor(40, 40);
    display.print(AVLB0);
    progress_bar(280 ,40, 45, 25, 100);

    display.setCursor(40, 110);
    display.print(AVLB1);
    progress_bar(280, 40, 115, 90, 80);

    display.setCursor(40, 180);
    display.print(AVLB0);
    progress_bar(280, 40, 185, 30, 50);

    display.setCursor(40, 250);
    display.print(AVLB1);
    progress_bar(280, 40, 255, 20, 10);*/

  }
  while (display.nextPage());
}

void Occupancy116(){
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    //delay(5000);

    //display.fillScreen(GxEPD_WHITE);
    display.drawXBitmap(0, 0, page116, 400, 300, GxEPD_BLACK);

    display.setCursor(40, 230);
    display.print("Available:");
    display.setCursor(155, 230);
    display.print(String(myRoot.surveys[2].sensors_absent));
    display.setCursor(240, 230);
    display.print("Occupied:");
    display.setCursor(340, 230);
    display.print(String(myRoot.surveys[2].sensors_occupied));

    progress_bar(200, 100, 250, myRoot.surveys[2].sensors_occupied, myRoot.surveys[2].sensors_absent);

    int angle = map(myRoot.surveys[2].sensors_occupied, 0, (myRoot.surveys[2].sensors_occupied + myRoot.surveys[2].sensors_absent), 171, 9);
    if (currentangle <= angle) {
      for (int tempangle = currentangle; tempangle <= angle; tempangle++) {
      myServo.write(tempangle);  
      delay(15);  
      }
    }else{
      for (int tempangle = currentangle; tempangle >= angle; tempangle--) {
      myServo.write(tempangle);  
      delay(15);  
      }
    }
    currentangle = angle;

    //MQTT Message Sending
    if (!client.connected()) {
    connectToMQTT();
    }
    client.loop();

    String jsonMessage = "{\"button\": \"0\"}"; 
    client.publish(mqttTopic, jsonMessage.c_str());
    Serial.println("\u5df2\u53d1\u5e03JSON\u6d88\u606f: " + jsonMessage);

  }
  while (display.nextPage());
}

void Other116(){
  String AVLB0, AVLB1, AVLB2, AVLB3;

  AVLB0 = myRoot.surveys[2].maps[0].name + " " + String(myRoot.surveys[2].maps[0].sensors_absent);
  //AVLB1 = myRoot.surveys[0].maps[1].name + " Available: " + String(myRoot.surveys[0].maps[1].sensors_absent);
  //AVLB2 = myRoot.surveys[0].maps[2].name + " Available: " + String(myRoot.surveys[0].maps[2].sensors_absent);
  //AVLB3 = myRoot.surveys[0].maps[3].name + " Available: " + String(myRoot.surveys[0].maps[3].sensors_absent);

  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    //delay(5000);

    //display.fillScreen(GxEPD_WHITE);
    //display.drawXBitmap(100, 0, page111, 200, 200, GxEPD_BLACK);

    display.setCursor(40, 40);
    display.print(AVLB0);
    progress_bar(280 ,40, 45, myRoot.surveys[2].maps[0].sensors_occupied, myRoot.surveys[2].maps[0].sensors_absent);

    /*display.setCursor(40, 90);
    display.print(AVLB1);
    progress_bar(40, 115, myRoot.surveys[0].maps[1].sensors_occupied, myRoot.surveys[0].maps[1].sensors_absent);

    display.setCursor(40, 140);
    display.print(AVLB0);
    progress_bar(40, 165, myRoot.surveys[0].maps[2].sensors_occupied, myRoot.surveys[0].maps[2].sensors_absent);

    display.setCursor(40, 190);
    display.print(AVLB1);
    progress_bar(40, 215, myRoot.surveys[0].maps[3].sensors_occupied, myRoot.surveys[0].maps[3].sensors_absent);*/

    //display.setCursor(40, 40);
    //display.print(AVLB0);
    //progress_bar(280 ,40, 45, 25, 100);

    /*display.setCursor(40, 110);
    display.print(AVLB1);
    progress_bar(280, 40, 115, 90, 80);

    display.setCursor(40, 180);
    display.print(AVLB0);
    progress_bar(280, 40, 185, 30, 50);

    display.setCursor(40, 250);
    display.print(AVLB1);
    progress_bar(280, 40, 255, 20, 10);*/

  }
  while (display.nextPage());
}

void Occupancy119(){
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    //delay(5000);

    //display.fillScreen(GxEPD_WHITE);
    display.drawXBitmap(0, 0, page119, 400, 300, GxEPD_BLACK);

    display.setCursor(40, 230);
    display.print("Available:");
    display.setCursor(155, 230);
    display.print(String(myRoot.surveys[1].sensors_absent));
    display.setCursor(240, 230);
    display.print("Occupied:");
    display.setCursor(340, 230);
    display.print(String(myRoot.surveys[1].sensors_occupied));

    progress_bar(200, 100, 250, myRoot.surveys[1].sensors_occupied, myRoot.surveys[1].sensors_absent);

    int angle = map(myRoot.surveys[1].sensors_occupied, 0, (myRoot.surveys[1].sensors_occupied + myRoot.surveys[1].sensors_absent), 171, 9);
    if (currentangle <= angle) {
      for (int tempangle = currentangle; tempangle <= angle; tempangle++) {
      myServo.write(tempangle);  
      delay(15);  
      }
    }else{
      for (int tempangle = currentangle; tempangle >= angle; tempangle--) {
      myServo.write(tempangle);  
      delay(15);  
      }
    }
    currentangle = angle;

    //MQTT Message Sending
    if (!client.connected()) {
    connectToMQTT();
    }
    client.loop();

    String jsonMessage = "{\"button\": \"0\"}"; 
    client.publish(mqttTopic, jsonMessage.c_str());
    Serial.println("\u5df2\u53d1\u5e03JSON\u6d88\u606f: " + jsonMessage);

  }
  while (display.nextPage());
}

void Other119(){
  String AVLB0, AVLB1, AVLB2, AVLB3;

  AVLB0 = myRoot.surveys[1].maps[0].name + " " + String(myRoot.surveys[1].maps[0].sensors_absent);
  //AVLB1 = myRoot.surveys[0].maps[1].name + " Available: " + String(myRoot.surveys[0].maps[1].sensors_absent);
  //AVLB2 = myRoot.surveys[0].maps[2].name + " Available: " + String(myRoot.surveys[0].maps[2].sensors_absent);
  //AVLB3 = myRoot.surveys[0].maps[3].name + " Available: " + String(myRoot.surveys[0].maps[3].sensors_absent);

  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //int16_t tbx, tby; uint16_t tbw, tbh;
  //display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    //delay(5000);

    //display.fillScreen(GxEPD_WHITE);
    //display.drawXBitmap(100, 0, page111, 200, 200, GxEPD_BLACK);

    display.setCursor(40, 40);
    display.print(AVLB0);
    progress_bar(280 ,40, 45, myRoot.surveys[1].maps[0].sensors_occupied, myRoot.surveys[1].maps[0].sensors_absent);

    /*display.setCursor(40, 90);
    display.print(AVLB1);
    progress_bar(40, 115, myRoot.surveys[0].maps[1].sensors_occupied, myRoot.surveys[0].maps[1].sensors_absent);

    display.setCursor(40, 140);
    display.print(AVLB0);
    progress_bar(40, 165, myRoot.surveys[0].maps[2].sensors_occupied, myRoot.surveys[0].maps[2].sensors_absent);

    display.setCursor(40, 190);
    display.print(AVLB1);
    progress_bar(40, 215, myRoot.surveys[0].maps[3].sensors_occupied, myRoot.surveys[0].maps[3].sensors_absent);*/

    //display.setCursor(40, 40);
    //display.print(AVLB0);
    //progress_bar(280 ,40, 45, 25, 100);

    /*display.setCursor(40, 110);
    display.print(AVLB1);
    progress_bar(280, 40, 115, 90, 80);

    display.setCursor(40, 180);
    display.print(AVLB0);
    progress_bar(280, 40, 185, 30, 50);

    display.setCursor(40, 250);
    display.print(AVLB1);
    progress_bar(280, 40, 255, 20, 10);*/

  }
  while (display.nextPage());
}

void progress_bar(int barlength, int x, int y, int occupied, int absent) {
    
    int totalWidth = barlength;

    
    int filledWidth = map(occupied, 0, occupied + absent, 0, totalWidth);

    
    int percentage = (occupied * 100) / (occupied + absent);

    
    String strValue = String(percentage) + "%";

    
    display.drawRoundRect(x, y, totalWidth + 10, 30, 15, GxEPD_BLACK);

    
    display.fillRoundRect(x + 5, y + 5, filledWidth, 20, 10, GxEPD_BLACK);

    
    display.setCursor(x + totalWidth + 15, y + 20);
    display.print(strValue);
}

void buttonlisten(){

  if (digitalRead(BUTTON1) == LOW) {
    delay(200); 
    if (bt1count == 0){
      Occupancy111();
      bt1count++;
      bt2count = 0;
      bt3count = 0;
    }else if (bt1count == 1){
      Other111();
      bt1count++;
      bt2count = 0;
      bt3count = 0;
    }else if (bt1count == 2){
      helloWorld();
      bt1count = 0;
      bt2count = 0;
      bt3count = 0;
    }
  }
  

  if (digitalRead(BUTTON2) == LOW) {
    delay(200); 
    if (bt2count == 0){
      Occupancy116();
      bt2count++;
      bt1count = 0;
      bt3count = 0;
    }else if (bt2count == 1){
      Other116();
      bt2count++;
      bt1count = 0;
      bt3count = 0;
    }else if (bt2count == 2){
      helloWorld();
      bt1count = 0;
      bt2count = 0;
      bt3count = 0;
    }
  }

 
  if (digitalRead(BUTTON3) == LOW) {
    delay(200); 
    if (bt3count == 0){
      Occupancy119();
      bt3count++;
      bt1count = 0;
      bt2count = 0;
    }else if (bt3count == 1){
      Other119();
      bt3count++;
      bt1count = 0;
      bt2count = 0;
    }else if (bt3count == 2){
      helloWorld();
      bt3count = 0;
      bt2count = 0;
      bt1count = 0;
    }
  }
}



void loop() {
  //fetchingData();
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  buttonlisten();
  display.hibernate();
};
