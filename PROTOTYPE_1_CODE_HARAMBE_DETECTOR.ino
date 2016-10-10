/*
 * HARAMBE DETECTOR
 * CODE FOR PROTOTYPE 1
 * Author: Harambelover92
 * SETTINGS FOR COMPASS
 * Default: {-32767, -32767, -32767}
 * Office: min: { -1509,  +1410,  -1183}    max: { +2069,  +2638,   +385}
 * Lab: min: { -3492,  -2296,  -5359}    max: {   -82,  +1143,   +842}
 */

#define echoPinC 7 // Echo Pin for Center detection
#define trigPinC 8 // Trigger Pin for Center detection
#define motorPinC 3 // Motor Pin for Center detection

#define echoPinR 4 // Echo Pin for Right detection
#define trigPinR 2 // Trigger Pin for Right detection
#define motorPinR 5 // Motor Pin for Right detection

#define echoPinL 9 // Echo Pin for Left detection
#define trigPinL 10 // Trigger Pin for Left detection
#define motorPinL 6 // Motor Pin for Left detection

#define button 11 // Reset button to reset the distance

#define LEDPin 13 // Onboard LED

#include <Wire.h>
#include <LSM303.h>

LSM303 compass;


int maximumRange = 200; // Maximum range needed
int minimumRange = 0; // Minimum range needed
int count = 0;
int countflag = 0;
long durationC, distanceC, durationR, distanceR, durationL, distanceL; // Duration used to calculate distance
int stride_length = 20;
int delayC = 0;

void setup() {
 Serial.begin (9600);
 
 pinMode(trigPinC, OUTPUT);
 pinMode(echoPinC, INPUT);
 pinMode(motorPinC,OUTPUT);
 
 pinMode(trigPinR, OUTPUT);
 pinMode(echoPinR, INPUT);
 pinMode(motorPinR,OUTPUT);
 
 pinMode(trigPinL, OUTPUT);
 pinMode(echoPinL, INPUT);
 pinMode(motorPinL,OUTPUT);

 pinMode(button,INPUT);
 
 pinMode(LEDPin, OUTPUT); // Use LED indicator (if required)

 Serial.begin(9600);
  Wire.begin();
  compass.init();
  compass.enableDefault();
  
  compass.m_min = (LSM303::vector<int16_t>){-3492,  -2296,  -5359};
  compass.m_max = (LSM303::vector<int16_t>){-82,  +1143,   +842};
}

void loop() {
  
 digitalWrite(trigPinC, LOW); 
 delayMicroseconds(2); 

 digitalWrite(trigPinC, HIGH);
 delayMicroseconds(10); 
 
 digitalWrite(trigPinC, LOW);
 durationC = pulseIn(echoPinC, HIGH);
 
 //Calculate the distance (in cm) based on the speed of sound.
 distanceC = durationC/58.2;
 
 if (distanceC >= maximumRange || distanceC <= minimumRange){
 //Serial.println("-1");
 digitalWrite(LEDPin, HIGH); 
 }
 else {
 digitalWrite(LEDPin, LOW);
 
  if(distanceC < 50) {
     Serial.print("Distance from Center:");
     Serial.println(distanceC);
     analogWrite(motorPinC,255-(distanceC*255/50));
   }
  else {
     analogWrite(motorPinC,0);
  }
 }

 digitalWrite(trigPinR, LOW); 
 delayMicroseconds(2); 

 digitalWrite(trigPinR, HIGH);
 delayMicroseconds(10); 
 
 digitalWrite(trigPinR, LOW);
 durationR = pulseIn(echoPinR, HIGH);
 
 distanceR = durationR/58.2;
 
 if (distanceR >= maximumRange || distanceR <= minimumRange){
 //Serial.println("-1");
 digitalWrite(LEDPin, HIGH); 
 }
 else {
 digitalWrite(LEDPin, LOW);
 
  if(distanceR < 50) {
     Serial.print("Distance from Right:");
     Serial.println(distanceR);
     analogWrite(motorPinR,255-(distanceR*255/50));
   }
  else {
     analogWrite(motorPinR,0);
  }
 }

 digitalWrite(trigPinL, LOW); 
 delayMicroseconds(2); 

 digitalWrite(trigPinL, HIGH);
 delayMicroseconds(10); 
 
 digitalWrite(trigPinL, LOW);
 durationL = pulseIn(echoPinL, HIGH);
 
 distanceL = durationL/58.2;
 
 if (distanceL >= maximumRange || distanceL <= minimumRange){
 //Serial.println("-1");
 digitalWrite(LEDPin, HIGH); 
 }
 else {
 digitalWrite(LEDPin, LOW);
 
  if(distanceL < 50) {
     Serial.print("Distance from Left:");
     Serial.println(distanceL);
     analogWrite(motorPinL,255-(distanceL*255/50));
   }
  else {
     analogWrite(motorPinL,0);
  }
 }

  compass.read();

  float heading = compass.heading((LSM303::vector<int>){0, -1, 0});

  if(delayC == 10){
    Serial.print("Direction from North:");
    Serial.println(heading);
    delayC = 0;
  } else {
    delayC++;
  }
 delay(500);
}
