#include <Wire.h>
#include <LSM303.h>
#include <L3G.h>

LSM303 imu1;
L3G gyro1;
int duration,distance;

#define ULTRA_RATIO 58.2
#define ULTRA_THRESHOLD 50
#define ULTRA_MIN 0
#define ULTRA_MAX 1000

#define Ultra1Trig 13 // green
#define Ultra1Echo 12 // yellow
#define Ultra1Vib  11 // blue

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial3.begin(115200);
  Wire.begin();

  // ultrasound config
  pinMode(Ultra1Trig, OUTPUT);
  pinMode(Ultra1Echo, OUTPUT);
  pinMode(Ultra1Echo, INPUT);

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

  Serial.print(a_x);
  Serial.print(",");
  Serial.print(a_y);
  Serial.print(",");
  Serial.print(a_z);
  Serial.print(",");
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

  Serial.print(g_x);
  Serial.print(",");
  Serial.print(g_y);
  Serial.print(",");
  Serial.print(g_z);
  Serial.print(",");
  // pi
  Serial3.print(g_x);
  Serial3.print(",");
  Serial3.print(g_y);
  Serial3.print(",");
  Serial3.print(g_z);
  Serial3.print(",");
  
  // heading
  float currheading = imu1.heading((LSM303::vector<int>){0, 0, 1});
  Serial.println(currheading);
  Serial3.println(currheading);

  // ultra
  digitalWrite(Ultra1Trig, LOW); 
  delay(2);  
  digitalWrite(Ultra1Trig, HIGH);   
  delay(2);         
  digitalWrite(Ultra1Trig, LOW);
  duration = pulseIn(Ultra1Echo, HIGH);
  distance = duration / ULTRA_RATIO;

  if (distance>ULTRA_MIN && ULTRA_MAX>distance){
    if (distance <= ULTRA_THRESHOLD && distance > 10){
      digitalWrite(Ultra1Vib,1);
      Serial.print("Distance: ");
      Serial.println(distance);
//      Serial3.print("Distance: ");
//      Serial3.println(distance);
    } else {
      digitalWrite(Ultra1Vib,0);
    }
  }
  
  delay(1);
  
  
}
