#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

unsigned long previousMillis = 0; // 用于存储上一次请求的时间
const long interval = 60000;      // 请求间隔时间，设置为 15 秒

// Wi-Fi 配置信息
const char* ssid = "CE-Hub-Staff";
const char* password = "casa-ce-sputnik-pbs-camper";
//const char* ssid = "Pixel_3221";
//const char* password = "20010906";

// API URL
String apiUrl1 = "https://uclapi.com/workspaces/sensors/summary?survey_ids=111&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";
String apiUrl2 = "https://uclapi.com/workspaces/sensors/averages/time?days=1&survey_ids=111&token=uclapi-04e8bbac6dd8d8d-d96285390ba9438-4abdabeffc6fc12-d2bc2643f1c8dff";

void setup() {
  // 启动串口监视器
  Serial.begin(115200);
  
  // 初始化 Serial2 (RXp2 和 TXp2)
  Serial2.begin(9600, SERIAL_8N1, 16, 17);  // 使用 GPIO 16 和 GPIO 17 来初始化 Serial2

  // 连接 Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

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
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // 在这里可以添加需要定时执行的代码，比如再次请求 API 或更新数据显示
    Serial.println("Fetching data again...");
    
    // 调用相应的 API 进行数据更新
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
    // 解析第一个 API 数据
    JsonArray surveys = doc["surveys"];
    int surveyIndex = 0;
    for (JsonObject survey : surveys) {
      Survey &currentSurvey = myRoot.surveys[surveyIndex++];
      currentSurvey.id = survey["id"];
      currentSurvey.name = String(survey["name"].as<const char*>());
      currentSurvey.sensors_occupied = survey["sensors_occupied"];
      currentSurvey.sensors_absent = survey["sensors_absent"];
      currentSurvey.sensors_other = survey["sensors_other"];
      
      // 构造调查数据字符串并通过 Serial2 发送
      String surveyData = "Survey|"+
                          String(currentSurvey.id) + "|" +
                          currentSurvey.name + "|" +
                          String(currentSurvey.sensors_occupied) + "|" +
                          String(currentSurvey.sensors_absent) + "|" +
                          String(currentSurvey.sensors_other);
      Serial2.println(surveyData);
      Serial.println(surveyData);
      delay(100);

      // 获取每个 map 数据
      JsonArray maps = survey["maps"];
      int mapIndex = 0;
      for (JsonObject map : maps) {
        Map &currentMap = currentSurvey.maps[mapIndex++];
        currentMap.id = map["id"];
        currentMap.name = String(map["name"].as<const char*>());
        currentMap.sensors_absent = map["sensors_absent"];
        currentMap.sensors_occupied = map["sensors_occupied"];
        
        // 构造地图数据字符串并通过 Serial2 发送
        String mapData = "Map|"+
                         String(currentSurvey.id) + "|" +
                         String(currentMap.id) + "|" +
                         currentMap.name + "|" +
                         String(currentMap.sensors_absent) + "|" +
                         String(currentMap.sensors_occupied);
        Serial2.println(mapData);
        Serial.println(mapData);
        delay(100);
      }
    }
  } else if (apiType == 2) {
    // 解析第二个 API 数据
    JsonArray surveys = doc["surveys"];
    for (JsonObject survey : surveys) {
      int survey_id = survey["survey_id"];
      if (survey_id == 111) {  // 找到对应的调查 ID
        JsonObject averages = survey["averages"];
        for (JsonPair kv : averages) {
          String time = kv.key().c_str();
          if (time.endsWith(":00:00")) { // 只取整点数据
            int hour = time.substring(0, 2).toInt();
            Time &currentTime = myRoot.surveys[0].averages.hours[hour]; // 假设 surveyIndex = 0
            JsonObject data = kv.value();
            currentTime.tm = hour;
            currentTime.sensors_absent = data["sensors_absent"];
            currentTime.sensors_occupied = data["sensors_occupied"];
            currentTime.sensors_total = data["sensors_total"];

            // 构造时间数据字符串并通过 Serial2 发送
            String timeData = "Hour|"+
                              String(survey_id) + "|" +
                              String(currentTime.tm) + "|" +
                              String(currentTime.sensors_absent) + "|" +
                              String(currentTime.sensors_occupied) + "|" +
                              String(currentTime.sensors_total);
            Serial2.println(timeData);
            Serial.println(timeData);
            delay(100);
          }
        }
      }
    }
  }
}
