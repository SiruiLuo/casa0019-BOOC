#include <SoftwareSerial.h>
#include <Servo.h>

// 定义 UART 通信引脚
#define RX_PIN 10  // Arduino 接收数据的引脚
#define TX_PIN 11  // Arduino 发送数据的引脚

// 创建一个 Servo 对象
Servo myServo;

// 定义伺服电机的引脚
int servoPin = 9;

int currentAngle = 255;

unsigned long previousMillis = 0; // 用于存储上一次请求的时间
// 定义软件串口（用作 UART 通信）
SoftwareSerial mySerial(RX_PIN, TX_PIN);

// 定义结构体
struct TimeData {
  int hour;
  int sensors_absent;
  int sensors_occupied;
  int sensors_total;
};

struct MapData {
  int id;
  String name;
  int sensors_absent;
  int sensors_occupied;
};

struct SurveyData {
  int id;
  String name;
  int sensors_occupied;
  int sensors_absent;
  int sensors_other;
  MapData maps[5];      // 每个调查最多 5 张地图
  TimeData hours[24];   // 每小时的数据（24 小时）
};

SurveyData surveys[3]; // 最多 3 个调查
int surveyCount = 0;   // 当前存储的调查数量

void setup() {
  // 设置串口通信速率
  Serial.begin(115200);       // 主串口，用于调试
  mySerial.begin(9600);       // 软件串口，用于与外部设备通信
  myServo.attach(servoPin);

  // 初始化伺服电机到起始位置（例如 0 度）
  myServo.write(255);

  Serial.println("System Ready: Listening for data...");
}

void loop() {
  // 检查是否有新数据传入
  if (mySerial.available() > 0) {
    myServo.detach(); // 释放伺服控制
    digitalWrite(servoPin, LOW); // 将引脚设置为低电平
    String receivedData = mySerial.readStringUntil('\n'); // 读取一行数据
    processReceivedData(receivedData);
    // 假设传入的是已更新的数据
    previousMillis = millis();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 5000) {
    previousMillis = currentMillis;
    Serial.println("Triggered");
    //ServoRunning(1,111);
    ServoRunning(surveys[0].sensors_absent,surveys[0].sensors_occupied);
    //delay(1000);
  }
}

// 处理接收数据的函数
void processReceivedData(String data) {
  if (data.startsWith("Survey|")) {
    handleSurveyData(data);
  } else if (data.startsWith("Map|")) {
    handleMapData(data);
  } else if (data.startsWith("Hour|")) {
    handleHourData(data);
  } else {
    Serial.println("Unknown Data Format: " + data);
  }
}

// 查找 Survey ID 的函数
int findSurveyIndexById(int id) {
  for (int i = 0; i < surveyCount; i++) {
    if (surveys[i].id == id) {
      return i; // 找到对应的 Survey
    }
  }
  return -1; // 未找到
}

// 查找 Map ID 的函数
int findMapIndexById(SurveyData &survey, int mapId) {
  for (int i = 0; i < 5; i++) {
    if (survey.maps[i].id == mapId) {
      return i; // 找到对应的 Map
    }
  }
  return -1; // 未找到
}

// 处理 Survey 数据的函数
void handleSurveyData(String data) {
  int id = getField(data, 1).toInt();
  int surveyIndex = findSurveyIndexById(id);

  if (surveyIndex == -1) {
    if (surveyCount >= 3) {
      Serial.println("Survey storage is full!");
      return;
    }

    surveyIndex = surveyCount++;
    surveys[surveyIndex].id = id;
  }

  surveys[surveyIndex].name = getField(data, 2);
  surveys[surveyIndex].sensors_occupied = getField(data, 3).toInt();
  surveys[surveyIndex].sensors_absent = getField(data, 4).toInt();
  surveys[surveyIndex].sensors_other = getField(data, 5).toInt();

  Serial.println("Updated Survey ID: " + String(id));
}

// 处理 Map 数据的函数
void handleMapData(String data) {
  int surveyId = getField(data, 1).toInt();
  int mapId = getField(data, 2).toInt();
  int surveyIndex = findSurveyIndexById(surveyId);

  if (surveyIndex == -1) {
    Serial.println("Survey ID not found for Map: " + String(surveyId));
    return;
  }

  SurveyData &currentSurvey = surveys[surveyIndex];
  int mapIndex = findMapIndexById(currentSurvey, mapId);

  if (mapIndex == -1) {
    for (int i = 0; i < 5; i++) {
      if (currentSurvey.maps[i].id == 0) {
        mapIndex = i;
        currentSurvey.maps[mapIndex].id = mapId;
        break;
      }
    }
    if (mapIndex == -1) {
      Serial.println("Map storage is full for Survey ID: " + String(surveyId));
      return;
    }
  }

  currentSurvey.maps[mapIndex].name = getField(data, 3);
  currentSurvey.maps[mapIndex].sensors_absent = getField(data, 4).toInt();
  currentSurvey.maps[mapIndex].sensors_occupied = getField(data, 5).toInt();

  Serial.println("Updated Map ID: " + String(mapId) + " in Survey ID: " + String(surveyId));
}

// 处理 Hour 数据的函数
void handleHourData(String data) {
  int surveyId = getField(data, 1).toInt();
  int hour = getField(data, 2).toInt();
  int surveyIndex = findSurveyIndexById(surveyId);

  if (surveyIndex == -1) {
    Serial.println("Survey ID not found for Hour: " + String(surveyId));
    return;
  }

  if (hour < 0 || hour >= 24) {
    Serial.println("Invalid hour value: " + String(hour));
    return;
  }

  SurveyData &currentSurvey = surveys[surveyIndex];
  TimeData &time = currentSurvey.hours[hour];
  time.hour = hour;
  time.sensors_absent = getField(data, 3).toInt();
  time.sensors_occupied = getField(data, 4).toInt();
  time.sensors_total = getField(data, 5).toInt();

  Serial.println("Updated Hour Data: " + String(hour) + " for Survey ID: " + String(surveyId));
}

// 分割字符串的工具函数
String getField(String data, int index) {
  int start = 0;
  int end = data.indexOf('|');
  for (int i = 0; i < index; i++) {
    start = end + 1;
    end = data.indexOf('|', start);
    if (end == -1) break;
  }
  if (end == -1) return data.substring(start);
  return data.substring(start, end);
}

// 计算并旋转伺服电机的函数

// 伺服电机的控制函数
void ServoRunning(int absent, int occupied) {
  // 计算占用率百分比
  int percentage = (occupied * 100) / (absent + occupied); // 使用百分比计算
  // 将百分比转换为角度值
  int targetAngle = map(percentage, 0, 100, 255, 15);  // 从 255° 到 15° 映射

  // 确保角度从当前值逐步到目标值
  if (targetAngle != currentAngle) {
    // 计算旋转方向
    int angleStep = (targetAngle > currentAngle) ? 1 : -1;

    // 逐步旋转到目标角度
    for (int angle = currentAngle; angle != targetAngle; angle += angleStep) {
      myServo.write(angle);
      delay(15);  // 每次调整角度后等待 15 毫秒
    }

    // 更新当前角度为目标角度
    currentAngle = targetAngle;
    //Serial.println("Current Angle: " + String(currentAngle));
  }
  Serial.println("Current Angle: " + String(currentAngle));
  Serial.println("Current Ratio: " + String(percentage) + "%");
}