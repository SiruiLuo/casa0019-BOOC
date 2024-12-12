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

Servo myServo;  // 创建一个 ESP32Servo 对象来控制伺服电机

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// or select the display constructor line in one of the following files (old style):
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"
#include "images.h"

#define BUTTON1  25  // 下一页按钮引脚
#define BUTTON2  26  // 上一页按钮引脚
#define BUTTON3  27  // 返回主页按钮引脚

int currentPage = 0;      // 当前页面编号
const int totalPages = 5; // 页面总数

int bt1count = 0;
int bt2count = 0;
int bt3count = 0;
// alternately you can copy the constructor from GxEPD2_display_selection.h or GxEPD2_display_selection_added.h to here
// e.g. for Wemos D1 mini:
//GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2, /*BUSY=D2*/ 4)); // GDEH0154D67

// for handling alternative SPI pins (ESP32, RP2040) see example GxEPD2_Example.ino

// 定义用于存储 JSON 响应数据的结构体
struct Time {
  int tm;                 // 时间
  int sensors_absent;     // 未使用传感器数量
  int sensors_occupied;   // 已使用传感器数量
  int sensors_total;      // 总传感器数量
};

struct Averages {
  Time hours[24];         // 每小时的数据
};

struct Map {
  int id;                 // 地图 ID
  String name;            // 地图名称
  int sensors_absent;     // 未使用传感器数量
  int sensors_occupied;   // 已使用传感器数量
};

struct Survey {
  int id;                 // 调查 ID
  String name;            // 调查名称
  int sensors_occupied;   // 已使用传感器数量
  int sensors_absent;     // 未使用传感器数量
  int sensors_other;      // 其他传感器数量
  Map maps[10];           // 假设每个调查最多有 10 个地图
  Averages averages;     // 平均时间数量
};

struct Root {
  bool ok;                // 请求是否成功的标志
  Survey surveys[10];     // 假设最多有 10 个调查
};

Root myRoot;              // 定义全局变量存储解析结果

// MQTT 服务器配置
const char* mqttServer = "mqtt.cetools.org"; // MQTT 服务器地址
const int mqttPort = 1884;                   // MQTT 端口
const char* mqttUser = "student";          // MQTT 用户名
const char* mqttPassword = "ce2021-mqtt-forget-whale";        // MQTT 密码
const char* mqttTopic = "student/CASA0014/ucfnuoa/";        // MQTT 发布主题

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousMillis = 0; // 用于存储上一次请求的时间
const long interval = 60000;      // 请求间隔时间，设置为 15 秒

// Wi-Fi 配置信息
//const char* ssid = "CE-Hub-Staff";
//const char* password = "casa-ce-sputnik-pbs-camper";
int surveycount = 0;
// 更新时标志是否为首次加载
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

  myServo.attach(13);  // 将伺服连接到 ESP32 的 D13 引脚
  myServo.write(171);
  delay(1000);

  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
  
    //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  Serial.begin(115200);
  // 连接 Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqttServer, mqttPort);
  connectToMQTT();

  // 启动 HTTP 客户端
  HTTPClient http;
  http.begin(apiUrl1); // 使用第一个 API URL

  // 发送请求并接收响应
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 1); // 调用解析函数
  } else {
    Serial.println("Failed to connect to API 1");
  }
  
  http.end();  // 结束第一个请求

  // 发送第二个请求
  http.begin(apiUrl2); // 使用第二个 API URL

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 2); // 调用解析函数
  } else {
    Serial.println("Failed to connect to API 2");
  }

  http.end();  // 结束第二个请求

    // 发送第三个请求
  http.begin(apiUrl3); // 使用第三个 API URL

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 1); // 调用解析函数
  } else {
    Serial.println("Failed to connect to API 3");
  }

  http.end();  // 结束第二个请求

  http.begin(apiUrl4); // 使用第三个 API URL

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 2); // 调用解析函数
  } else {
    Serial.println("Failed to connect to API 4");
  }

  http.end();  // 结束第二个请求

  http.begin(apiUrl5); // 使用第三个 API URL

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 1); // 调用解析函数
  } else {
    Serial.println("Failed to connect to API 5");
  }

  http.end();  // 结束第二个请求

  http.begin(apiUrl6); // 使用第三个 API URL

  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    deserializeJsonData(payload, 2); // 调用解析函数
  } else {
    Serial.println("Failed to connect to API 6");
  }

  http.end();  // 结束第二个请求

  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  helloWorld();
  display.hibernate();
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("\u6b63\u5728\u8fde\u63a5\u5230 MQTT \u670d\u52a1\u5668...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) { // 提供用户名和密码
      Serial.println("\u5df2\u8fde\u63a5\u5230 MQTT \u670d\u52a1\u5668");
    } else {
      Serial.print("\u8fde\u63a5\u5931\u8d25\uff0c\u72b6\u6001\u7801: ");
      Serial.print(client.state());
      Serial.println(" 5\u79d2\u540e\u91cd\u8bd5...");
      delay(5000);
    }
  }
}


// 用于更新某个 Survey 数据
void updateSurveyData(Survey &currentSurvey, JsonObject &survey) {
  currentSurvey.sensors_occupied = survey["sensors_occupied"];
  currentSurvey.sensors_absent = survey["sensors_absent"];
  currentSurvey.sensors_other = survey["sensors_other"];
  
  // 更新 Maps 数据
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

    // 在这里可以添加需要定时执行的代码，比如再次请求 API 或更新数据显示
    Serial.println("Fetching data again...");
    
    // 启动 HTTP 客户端
    HTTPClient http;
    http.begin(apiUrl1); // 使用第一个 API URL

    // 发送请求并接收响应
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 1); // 调用解析函数
    } else {
      Serial.println("Failed to connect to API 1");
    }
    
    http.end();  // 结束第一个请求

    // 发送第二个请求
    http.begin(apiUrl2); // 使用第二个 API URL

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 2); // 调用解析函数
    } else {
      Serial.println("Failed to connect to API 2");
    }

    http.end();  // 结束第二个请求

      // 发送第三个请求
    http.begin(apiUrl3); // 使用第三个 API URL

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 1); // 调用解析函数
    } else {
      Serial.println("Failed to connect to API 3");
    }

    http.end();  // 结束第二个请求

    http.begin(apiUrl4); // 使用第三个 API URL

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 2); // 调用解析函数
    } else {
      Serial.println("Failed to connect to API 4");
    }

    http.end();  // 结束第二个请求

    http.begin(apiUrl5); // 使用第三个 API URL

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 1); // 调用解析函数
    } else {
      Serial.println("Failed to connect to API 5");
    }

    http.end();  // 结束第二个请求

    http.begin(apiUrl6); // 使用第三个 API URL

    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      deserializeJsonData(payload, 2); // 调用解析函数
    } else {
      Serial.println("Failed to connect to API 6");
    }

    http.end();  // 结束第二个请求
  }
}

// 解析 JSON 数据的函数
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
      
      // 查找或创建对应的 Survey 数据
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

      // 构造 Survey 数据并发送
      String surveyData = "Survey|" +
                          String(currentSurvey->id) + "|" +
                          currentSurvey->name + "|" +
                          String(currentSurvey->sensors_occupied) + "|" +
                          String(currentSurvey->sensors_absent) + "|" +
                          String(currentSurvey->sensors_other);
      Serial.println(surveyData);
      delay(100);

      // 更新每个 Map 数据
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
    // 针对平均值数据处理
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
        if (time.endsWith(":00:00")) { // 只取整点数据
          int hour = time.substring(0, 2).toInt();
          Time &currentTime = currentSurvey->averages.hours[hour];
          JsonObject data = kv.value();
          currentTime.tm = hour;
          currentTime.sensors_absent = data["sensors_absent"];
          currentTime.sensors_occupied = data["sensors_occupied"];
          currentTime.sensors_total = data["sensors_total"];

          // 构造时间数据字符串并通过 Serial2 发送
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
      myServo.write(tempangle);  // 设置伺服电机角度
      delay(15);  // 每次变化之间的延迟，使旋转缓慢
      }
    }else{
      for (int tempangle = currentangle; tempangle >= angle; tempangle--) {
      myServo.write(tempangle);  // 设置伺服电机角度
      delay(15);  // 每次变化之间的延迟，使旋转缓慢
      }
    }
    currentangle = angle;

    //MQTT Message Sending
    if (!client.connected()) {
    connectToMQTT();
    }
    client.loop();

    String jsonMessage = "{\"button\": \"0\"}"; // JSON 格式消息
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
      myServo.write(tempangle);  // 设置伺服电机角度
      delay(15);  // 每次变化之间的延迟，使旋转缓慢
      }
    }else{
      for (int tempangle = currentangle; tempangle >= angle; tempangle--) {
      myServo.write(tempangle);  // 设置伺服电机角度
      delay(15);  // 每次变化之间的延迟，使旋转缓慢
      }
    }
    currentangle = angle;

    //MQTT Message Sending
    if (!client.connected()) {
    connectToMQTT();
    }
    client.loop();

    String jsonMessage = "{\"button\": \"0\"}"; // JSON 格式消息
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
      myServo.write(tempangle);  // 设置伺服电机角度
      delay(15);  // 每次变化之间的延迟，使旋转缓慢
      }
    }else{
      for (int tempangle = currentangle; tempangle >= angle; tempangle--) {
      myServo.write(tempangle);  // 设置伺服电机角度
      delay(15);  // 每次变化之间的延迟，使旋转缓慢
      }
    }
    currentangle = angle;

    //MQTT Message Sending
    if (!client.connected()) {
    connectToMQTT();
    }
    client.loop();

    String jsonMessage = "{\"button\": \"0\"}"; // JSON 格式消息
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
    // 总宽度
    int totalWidth = barlength;

    // 将 occupied 映射到 0 到 totalWidth 的范围
    int filledWidth = map(occupied, 0, occupied + absent, 0, totalWidth);

    // 计算百分比，使用浮点数计算并转化为整数百分比
    int percentage = (occupied * 100) / (occupied + absent);

    // 转换为字符串并添加百分号
    String strValue = String(percentage) + "%";

    // 画外框
    display.drawRoundRect(x, y, totalWidth + 10, 30, 15, GxEPD_BLACK);

    // 画填充的部分
    display.fillRoundRect(x + 5, y + 5, filledWidth, 20, 10, GxEPD_BLACK);

    // 显示百分比
    display.setCursor(x + totalWidth + 15, y + 20);
    display.print(strValue);
}

void buttonlisten(){
  // 检测按钮1 (下一页)
  if (digitalRead(BUTTON1) == LOW) {
    delay(200); // 消抖
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
  
  // 检测按钮2 (上一页)
  if (digitalRead(BUTTON2) == LOW) {
    delay(200); // 消抖
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

  // 检测按钮3 (返回主页)
  if (digitalRead(BUTTON3) == LOW) {
    delay(200); // 消抖
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