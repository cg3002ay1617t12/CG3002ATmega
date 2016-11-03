#include <Wire.h>
#include <LSM303.h>
#include <L3G.h>
#include <NewPing.h>

LSM303 imu1;
L3G gyro1;
int duration,distance;


#define ULTRA_THRESHOLD 50
#define SONAR_NUM 3
#define PING_INTERVAL 33 
#define MAX_DISTANCE 200

#define Ultra1Trig 12 // green
#define Ultra1Echo 11 // yellow
#define Ultra1Vib  10 // blue

#define Ultra2Trig 9 // green
#define Ultra2Echo 8 // yellow
#define Ultra2Vib  7 // blue

#define Ultra3Trig 6 // green
#define Ultra3Echo 5 // yellow
#define Ultra3Vib  4 // blue

NewPing sonar[SONAR_NUM] = {
  NewPing(Ultra1Trig, Ultra1Echo, MAX_DISTANCE),
  NewPing(Ultra2Trig, Ultra2Echo, MAX_DISTANCE),
  NewPing(Ultra3Trig, Ultra3Echo, MAX_DISTANCE)
};

int vibs[SONAR_NUM] = {10,7,4};

unsigned long pingTimer[SONAR_NUM]; // When each pings.
unsigned int cm[SONAR_NUM]; // Store ping distances.
uint8_t currentSensor = 0; // Which sensor is active.

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial3.begin(115200);
  Wire.begin();

  // ultrasound config
  pinMode(Ultra1Trig, OUTPUT);
  pinMode(Ultra1Echo, INPUT);
  pinMode(Ultra1Vib , OUTPUT);
  
  pinMode(Ultra2Trig, OUTPUT);
  pinMode(Ultra2Echo, INPUT);
  pinMode(Ultra2Vib , OUTPUT);
  
  pinMode(Ultra3Trig, OUTPUT);
  pinMode(Ultra3Echo, INPUT);
  pinMode(Ultra3Vib, OUTPUT);

  // ultrasound
  pingTimer[0] = millis() + 75; // First ping start in ms.
  for (uint8_t i = 1; i < SONAR_NUM; i++)
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;

  // imu config
  imu1.init();
  imu1.enableDefault();
  Serial.print("hello");
  imu1.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
  imu1.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};
  
  // gyro config
  if (!gyro1.init()){
    Serial.println("Failed to autodetect gyro type!");
    while (1);
  }
  gyro1.enableDefault();
}

void loop() {
  // put your main code here, to run repeatedly:
  imu1.read();
  gyro1.read();



  // acclerometer
  double a_x = (imu1.a.x / 1600.0);
  double a_y = (imu1.a.y / 1600.0);
  double a_z = (imu1.a.z / 1600.0);

//  Serial.print(a_x);
//  Serial.print(",");
//  Serial.print(a_y);
//  Serial.print(",");
//  Serial.print(a_z);
//  Serial.print(",");
  // pi
  Serial3.print(a_x);
  Serial3.print(",");
  Serial3.print(a_y);
  Serial3.print(",");
  Serial3.print(a_z);
  Serial3.print(",");

  // gyro
  float g_x = 0;
  float g_y = 0;
  float g_z = 0;
  float x_offset = 0.67;
  float y_offset = 2.04;
  float z_offset = -0.11;
  g_x = gyro1.g.x * 8.75/1000 + x_offset;
  g_y = gyro1.g.y * 8.75/1000 + y_offset;
  g_z = gyro1.g.z * 8.75/1000 + z_offset;

//  Serial.print(g_x);
//  Serial.print(",");
//  Serial.print(g_y);
//  Serial.print(",");
//  Serial.print(g_z);
//  Serial.print(",");
  // pi
  Serial3.print(g_x);
  Serial3.print(",");
  Serial3.print(g_y);
  Serial3.print(",");
  Serial3.print(g_z);
  Serial3.print(",");
  
  // heading
  float currheading = imu1.heading((LSM303::vector<int>){0, 0,  1});
//  Serial.println(currheading);
  Serial3.println(currheading);

//  // ultra 1
//  duration = sonar1.ping();
//  distance = duration / US_ROUNDTRIP_CM;
//
//  if (distance < ULTRA_THRESHOLD && distance != 0){
//      digitalWrite(Ultra1Vib,1);
//      Serial.print("DistanceLeft: ");
//      Serial.println(distance);
//  } else{
//     digitalWrite(Ultra1Vib,0);
//  }
//
//  // ultra 2 right
//  duration = sonar2.ping();
//  distance = duration / US_ROUNDTRIP_CM;
//
//  if (distance < ULTRA_THRESHOLD && distance != 0){
//      digitalWrite(Ultra2Vib,1);
//      Serial.print("DistanceRight: ");
//      Serial.println(distance);
//  } else{
//     digitalWrite(Ultra2Vib,0);
//  }
//
//  // ultra 3 centre
//  duration = sonar3.ping();
//  distance = duration / US_ROUNDTRIP_CM;
//
//  if (distance < ULTRA_THRESHOLD && distance != 0){
//      digitalWrite(Ultra3Vib,1);
//      Serial.print("DistanceCentre: ");
//      Serial.println(distance);
//  } else{
//     digitalWrite(Ultra3Vib,0);
//  }

  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    if (millis() >= pingTimer[i]) {
      pingTimer[i] += PING_INTERVAL * SONAR_NUM;
      if (i == 0 && currentSensor == SONAR_NUM - 1)
        oneSensorCycle(); // Do something with results.
      sonar[currentSensor].timer_stop();
      currentSensor = i;
      cm[currentSensor] = 0;
      sonar[currentSensor].ping_timer(echoCheck);
    }
  }
  delay(20);
}

void echoCheck() { // If ping echo, set distance to array.
  if (sonar[currentSensor].check_timer())
    cm[currentSensor] = sonar[currentSensor].ping_result / US_ROUNDTRIP_CM;
}

void oneSensorCycle() { // Do something with the results.
  for (uint8_t i = 0; i < SONAR_NUM; i++) {

    if (cm[i] < ULTRA_THRESHOLD && cm[i] != 0){
      digitalWrite(vibs[i],1);
      Serial.print(i);
      // 1 is centre motor centre ultra
      // 2 is right motor right ultra
      // 0 is left motor left ultrasound
    Serial.print("=");
    Serial.println(cm[i]);
    } else {
      digitalWrite(vibs[i],0);
    }
   
  }
}
