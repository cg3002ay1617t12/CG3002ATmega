#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <LSM303.h>
#include <L3G.h>

LSM303 imu1;
L3G gyro1;


#ifdef ARDUINO_AVR_MEGA2560
  // Serial 3: 15 (RX) 14 (TX);
  #define SERIAL Serial3
#elif ARDUINO_AVR_UNO
  #define SERIAL Serial3
#endif

// Constants
#define PACKET_SIZE 64
#define ULTRA_PING_DURATION 2
#define ULTRA_RATIO 58.2
#define ULTRA_THRESHOLD 60

// ASCII used for Recieving
#define ASCII_STARTFRAME 60
#define ASCII_ENDFRAME 62
#define ASCII_HELLO 49
#define ASCII_DATA 50
#define ASCII_ACK 51
#define DUMMY_CRC 49

#define IMU_RATIO 5

#define HANDSHAKE_SIZE 3
#define DATA_OFFSET 4
#define CRC_LENGTH 2

// Ultrasound 1 - MID
#define Ultra1Trig 10
#define Ultra1Echo 9
#define Ultra1Vib  8
// Ultrasound 2 - LEFT
#define Ultra2Trig 7
#define Ultra2Echo 6
#define Ultra2Vib  5
// ultrasound 3 - RIGHT
#define Ultra3Trig 13
#define Ultra3Echo 12
#define Ultra3Vib  11

#define VibrationMotor

// States used for Recieving
enum RxState {READY, PACKET_TYPE, TERMINATE};
enum PacketType {UNDETERMINED, HELLO, ACK};

// Variables for receiving data
RxState CurrMode = READY;
PacketType packetType = UNDETERMINED;
int incomingByte = 0;

// Variables for sending data
char handshake = '0';   // start sending data only after handshake
char readyToSend = '1'; // to support stop and wait
char packetSendArray[PACKET_SIZE] = {0};
char crc[2] = {00};

// task declaration
void serialRead(void *pvParameters);
void IMUONEread(void *pvParameters);
void sendIMUONEAccData(void *pvParameters);
void sendIMUONECompassData(void *pvParameters);
void sendIMUONEGyroData(void *pvParameters);
void sendUltra1soundDist(void *pvParameters);

// function declaration
void frameAndSendPacket(int fromComponentID, int dataToSend);
void sendHello();
void generateCRC(int frameSize);

// Synchronization primitives
SemaphoreHandle_t xSemaphore = NULL;

int IMUONEAccCount = 0;
int IMUONECompassCount = 0;
int IMUONEGyroCount = 0;

void setup() {
  xSemaphore = xSemaphoreCreateMutex();
  xSemaphoreGive(xSemaphore);
  // setup serial ports
  SERIAL.begin(9600);
  while (!SERIAL); // to ensure that Serial 3 is setup
  Serial.begin(9600);
  while (!Serial);  // to ensure that Serial is setup

  // init(s) for sensors
  // INIT GYRO1
  Wire.begin();
  if (!gyro1.init())
  {
    Serial.println("Failed to autodetect gyro type!");
    while (1);
  }
  gyro1.enableDefault();
  
  // INIT ULTRA1
  pinMode(Ultra1Trig, OUTPUT);
  pinMode(Ultra1Echo, INPUT);
  pinMode(Ultra1Vib, OUTPUT);

  // INIT ULTRA2
  pinMode(Ultra2Trig, OUTPUT);
  pinMode(Ultra2Echo, INPUT);
  pinMode(Ultra2Vib, OUTPUT);

  // INIT ULTRA3
  pinMode(Ultra3Trig, OUTPUT);
  pinMode(Ultra3Echo, INPUT);
  pinMode(Ultra3Vib, OUTPUT);

  // INIT IMU1
  imu1.init();
  imu1.enableDefault();
  imu1.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
  imu1.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};

  sendHello();

  Serial.println("Initialized Mega");

  xTaskCreate(
    serialRead
    ,  (const portCHAR *)"serialRead"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  10  // priority
    ,  NULL );

   xTaskCreate(
    sendIMUONEAccData
    ,  (const portCHAR *)"sendIMUONEAccData"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  4  // priority
    ,  NULL );

   xTaskCreate(
    sendIMUONECompassData
    ,  (const portCHAR *)"sendIMUONECompassData"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  12  // priority
    ,  NULL );

  xTaskCreate(
   sendIMUONEGyroData
   ,  (const portCHAR *)"sendIMUONEGyroData"   // A name just for humans
   ,  128  // Stack size
   ,  NULL
   ,  4  // priority
   ,  NULL );

   xTaskCreate(
    sendUltra1soundDist
    ,  (const portCHAR *)"sendUltra1soundDist"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  4  // priority
    ,  NULL );

    xTaskCreate(
    sendUltra2soundDist
    ,  (const portCHAR *)"sendUltra1soundDist"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  5  // priority
    ,  NULL );

    xTaskCreate(
    sendUltra3soundDist
    ,  (const portCHAR *)"sendUltra1soundDist"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  6  // priority
    ,  NULL );


//   xTaskCreate(
//    IMUONEread
//    ,  (const portCHAR *)"IMUONEread"   // A name just for humans
//    ,  128  // Stack size
//    ,  NULL
//    ,  5  // priority
//    ,  NULL );
    
}

void loop() {
  // use task!
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void serialRead(void *pvParameters)  // This is a task.
{ 
  for (;;){
    if (Serial.available() > 0){
      incomingByte = Serial.read();
      Serial.print(incomingByte);
      switch(CurrMode) {            
         case READY :       // Syncing up to for packet opening
            if (incomingByte == ASCII_STARTFRAME){
              CurrMode = PACKET_TYPE;
              packetType = UNDETERMINED; 
              Serial.println("Recieved Start");
            }
            break;
         case PACKET_TYPE : // Determind what kind of packet is being recieved
           switch(incomingByte){
              case ASCII_HELLO :
               Serial.println ("Recieved HELLO");
                packetType = HELLO;
                CurrMode = TERMINATE;
                break;
              case ASCII_ACK :
               Serial.println("Recieved ACK");
                packetType = ACK;
                CurrMode = TERMINATE;
                break;
              default :
                CurrMode = READY;
               Serial.println("CORRUPT, resetting to READY");
           }
          break;
        case TERMINATE:
//          Serial.println("terminate");
          if (incomingByte == ASCII_ENDFRAME){
            CurrMode = READY;
            if (packetType == HELLO){
              handshake = '1';
              readyToSend = '1';
             Serial.println("Handshake completed");
            }
            if (packetType == ACK){
              readyToSend = '1';
            }
          } else {
            CurrMode = READY;
           Serial.println("CORRUPT, resetting to READY");
          }       
      }
    }
    vTaskDelay(10); 
  }
}

//send IMU task

//void IMUONEread(void *pvParameters) {
//  while (1){
////    imu1.read();
//    vTaskDelay(1);
//  }
//}

void sendIMUONEAccData(void *pvParameters) {
  int datalength = 0;
  char tempX[6] = {0};
  char tempY[6] = {0};
  char tempZ[6] = {0};
  double x = 0;
  double y = 0;
  double z = 0;
  while(1){
    if (readyToSend == '1'){
      if( xSemaphore != NULL ) {
        if(xSemaphoreTake(xSemaphore, (TickType_t) 10) == pdTRUE) {
          imu1.read();
          datalength = DATA_OFFSET;
          x = (imu1.a.x / 1600.0);
          y = (imu1.a.y / 1600.0);
          z = (imu1.a.z / 1600.0);
      
          // setting up x
          dtostrf(x, 4, 2, tempX);
          sprintf(packetSendArray+datalength,"%s", tempX);
          datalength += strlen(tempX);
          packetSendArray[datalength++] = ',';
      
          // setting up y
          dtostrf(y, 4, 2, tempY);
          sprintf(packetSendArray+datalength,"%s", tempY);
          datalength += strlen(tempY);
          packetSendArray[datalength++] = ',';
      
          // setting up z
          dtostrf(z, 4, 2, tempZ);
          sprintf(packetSendArray+datalength,"%s", tempZ);
          datalength += strlen(tempZ);
           
          frameAndSendPacket(1, datalength);
          // readyToSend = 0;

          xSemaphoreGive(xSemaphore);
        } else {
            // Serial.println("Waiting...");
        }
      }
    }
    vTaskDelay(1);
  }
}

void sendIMUONECompassData(void *pvParameters) {
  int datalength = 0;
  float currheading = 0;
  char tempCompass[7] = {0};
  while(1){
    if (readyToSend == '1') {
//      if(xSemaphore != NULL) {
//        if( xSemaphoreTake(xSemaphore, (TickType_t) 10) == pdTRUE ) {
            datalength = DATA_OFFSET;
            imu1.read();  // acc will be reading at higher much higher frequency no need to re-read here
            currheading = imu1.heading((LSM303::vector<int>){0, 0, 1});
            dtostrf(currheading, 6, 2, tempCompass);
            sprintf(packetSendArray+datalength, "%s", tempCompass);
            datalength += strlen(tempCompass);
            frameAndSendPacket(2, datalength);
            
//            xSemaphoreGive(xSemaphore);
        } else {
            // Serial.println("Waiting...");
        }
      }
    // readyToSend = 0;
    vTaskDelay(5);
}

void sendIMUONEGyroData(void *pvParameters) {
  int datalength = 0;
  char tempX[6] = {0};
  char tempY[6] = {0};
  char tempZ[6] = {0};
  float g_x = 0;
  float g_y = 0;
  float g_z = 0;
  float x_offset = 0.67;
  float y_offset = 2.04;
  float z_offset = -0.11;
  while(1){
    if (readyToSend == '1'){
      if(xSemaphore != NULL) {
        if( xSemaphoreTake(xSemaphore, (TickType_t) 10) == pdTRUE ) {
          gyro1.read();
          datalength = DATA_OFFSET;
          g_x = gyro1.g.x * 8.75/1000 + x_offset;
          g_y = gyro1.g.y * 8.75/1000 + y_offset;
          g_z = gyro1.g.z * 8.75/1000 + z_offset;
      
          // setting up x
          dtostrf(g_x, 4, 2, tempX);
          sprintf(packetSendArray+datalength,"%s", tempX);
          datalength += strlen(tempX);
          packetSendArray[datalength++] = ',';
      
          // setting up y
          dtostrf(g_y, 4, 2, tempY);
          sprintf(packetSendArray+datalength,"%s", tempY);
          datalength += strlen(tempY);
          packetSendArray[datalength++] = ',';
      
          // setting up z
          dtostrf(g_z, 4, 2, tempZ);
          sprintf(packetSendArray+datalength,"%s", tempZ);
          datalength += strlen(tempZ);
           
          frameAndSendPacket(3, datalength);
          // readyToSend = 0;
           
          xSemaphoreGive(xSemaphore);
        } else {
          // Serial.println("Waiting...");
        }
      }
    }
    vTaskDelay(1);
  }
}

void sendUltra1soundDist(void *pvParameters) {
  int datalength = 0;
  int duration,distance;
  float currheading = 0;
  char tempCompass[6] = {0};
  while(1){
       if (readyToSend == '1'){
          datalength = DATA_OFFSET;
          digitalWrite(Ultra1Trig, LOW); 
          vTaskDelay(ULTRA_PING_DURATION);
          digitalWrite(Ultra1Trig, HIGH);
          vTaskDelay(ULTRA_PING_DURATION);
          digitalWrite(Ultra1Trig, LOW);
          duration = pulseIn(Ultra1Echo, HIGH);
          distance = duration / ULTRA_RATIO;
          // Obstacle detected
          if (distance <= ULTRA_THRESHOLD and distance > 7){
            Serial.print("Distance Front: ");
            Serial.println(distance);
            digitalWrite(Ultra1Vib, 1);
          } else {
            digitalWrite(Ultra1Vib,0);
          }
    }
    vTaskDelay(5);
  }
}

void sendUltra2soundDist(void *pvParameters) {
  int datalength = 0;
  int duration,distance;
  float currheading = 0;
  char tempCompass[6] = {0};
  while(1){
       if (readyToSend == '1'){
          datalength = DATA_OFFSET;
          digitalWrite(Ultra2Trig, LOW); 
          vTaskDelay(ULTRA_PING_DURATION);
          digitalWrite(Ultra2Trig, HIGH);
          vTaskDelay(ULTRA_PING_DURATION);
          digitalWrite(Ultra2Trig, LOW);
          duration = pulseIn(Ultra2Echo, HIGH);
          distance = duration / ULTRA_RATIO;
          // Obstacle detected
          if (distance <= ULTRA_THRESHOLD and distance > 7){
            Serial.print("Distance Left: ");
            Serial.println(distance);
            digitalWrite(Ultra2Vib, 1);
          } else {
            digitalWrite(Ultra2Vib,0);
          }
    }
    vTaskDelay(5);
  }
}

void sendUltra3soundDist(void *pvParameters) {
  int datalength = 0;
  int duration,distance;
  float currheading = 0;
  char tempCompass[6] = {0};
  while(1){
       if (readyToSend == '1'){
          datalength = DATA_OFFSET;
          digitalWrite(Ultra3Trig, LOW); 
          vTaskDelay(ULTRA_PING_DURATION);
          digitalWrite(Ultra3Trig, HIGH);
          vTaskDelay(ULTRA_PING_DURATION);
          digitalWrite(Ultra3Trig, LOW);
          duration = pulseIn(Ultra3Echo, HIGH);
          distance = duration / ULTRA_RATIO;
          // Obstacle detected
          if (distance <= ULTRA_THRESHOLD and distance > 7){
            Serial.print("Distance Right: ");
            Serial.println(distance);
            digitalWrite(Ultra3Vib, 1);
          } else {
            digitalWrite(Ultra3Vib,0);
          }
    }
    vTaskDelay(5);
  }
}

/*--------------------------------------------------*/
/*------------------- Functions --------------------*/
/*--------------------------------------------------*/

void sendHello(){
  packetSendArray[0] = ASCII_STARTFRAME;
  packetSendArray[1] = ASCII_HELLO;
  packetSendArray[2] = ASCII_ENDFRAME;
  SERIAL.write(packetSendArray,HANDSHAKE_SIZE);
  SERIAL.flush();
}

void frameAndSendPacket(int fromComponentID, int dataLength){
  packetSendArray[0] = ASCII_STARTFRAME;
  packetSendArray[1] = ASCII_DATA;
  packetSendArray[2] = fromComponentID;
  packetSendArray[3] = dataLength - DATA_OFFSET;
  generateCRC(dataLength);
  packetSendArray[dataLength+CRC_LENGTH] = ASCII_ENDFRAME;
  SERIAL.write(packetSendArray,dataLength+CRC_LENGTH+1); // DATALENGTH has already taken DATA_OFFSET into account, why do you need to + DATA_OFFSET here?
  SERIAL.flush();
//  Serial.write(packetSendArray,dataLength+CRC_LENGTH+1); 
//  Serial.println();
  

  if (fromComponentID == 2){
     Serial.write(packetSendArray, dataLength+CRC_LENGTH+1);
     Serial.println();
  }
 
}

void generateCRC(int CRCStart){
  packetSendArray[CRCStart] = DUMMY_CRC;
  packetSendArray[CRCStart + 1] = DUMMY_CRC;
}


