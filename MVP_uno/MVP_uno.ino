#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include <LSM303.h>
#include <L3G.h>

LSM303 imu1;
L3G gyro1;

// Constants
#define PACKET_SIZE 64

// ASCII used for Recieving
#define ASCII_STARTFRAME 60
#define ASCII_ENDFRAME 62
#define ASCII_HELLO 49
#define ASCII_DATA 50
#define ASCII_ACK 51
#define DUMMY_CRC 49

#define HANDSHAKE_SIZE 3
#define DATA_OFFSET 4
#define CRC_LENGTH 2

// States used for Recieving
enum RxState {READY, PACKET_TYPE, TERMINATE};
enum PacketType {UNDETERMINED, HELLO, ACK};

// Variables for receiving data
RxState CurrMode = READY;
PacketType packetType = UNDETERMINED;
int incomingByte = 0;

// Variables for sending data
char handshake = '0';   // start sending data only after handshake
char readyToSend = '0'; // to support stop and wait
char packetSendArray[PACKET_SIZE] = {0};
char crc[2] = {00};

// task declaration
void serialRead(void *pvParameters);
void IMUONEread(void *pvParameters);
void sendIMUONEAccData(void *pvParameters);
void sendIMUONECompassData(void *pvParameters);
void sendIMUONEGyroData(void *pvParameters);

// function declaration
int processFloat(double val, char *result, int datalength, bool addComma);
void frameAndSendPacket(int fromComponentID, int dataToSend);
void sendHello();
void generateCRC(int frameSize);

void setup() {
  // setup serial ports
//  Serial3.begin(115200);
//  while (!Serial3); // to ensure that Serial 3 is setup
  Serial.begin(115200);
  while (!Serial);  // to ensure that Serial is setup

  // init(s) for sensors
  Wire.begin();
  if (!gyro1.init())
  {
//    Serial.println("Failed to autodetect gyro type!");
    while (1);
  }
  gyro1.enableDefault();
  // INIT IMU1
  imu1.init();
  imu1.enableDefault();
  /*
  Calibration values; the default values of +/-32767 for each axis
  lead to an assumed magnetometer bias of 0. Use the Calibrate example
  program to determine appropriate values for your particular unit.
  */
  imu1.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
  imu1.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};

  sendHello();

//  Serial.println("Initialized Mega");

  xTaskCreate(
    serialRead
    ,  (const portCHAR *)"serialRead"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  10  // priority
    ,  NULL );

   xTaskCreate(
    IMUONEread
    ,  (const portCHAR *)"IMUONEread"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  5  // priority
    ,  NULL );

   xTaskCreate(
    sendIMUONEAccData
    ,  (const portCHAR *)"sendIMUONEAccData"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  3  // priority
    ,  NULL );
    

   xTaskCreate(
    sendIMUONECompassData
    ,  (const portCHAR *)"sendIMUONECompassData"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  4  // priority
    ,  NULL );

    xTaskCreate(
    sendIMUONEGyroData
    ,  (const portCHAR *)"sendIMUONEGyroData"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  5  // priority
    ,  NULL );
    
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
//      Serial.print(incomingByte);
      switch(CurrMode) {            
         case READY :       // Syncing up to for packet opening
            if (incomingByte == ASCII_STARTFRAME){
              CurrMode = PACKET_TYPE;
              packetType = UNDETERMINED; 
//              Serial.println("Recieved Start");
            }
            break;
         case PACKET_TYPE : // Determind what kind of packet is being recieved
           switch(incomingByte){
              case ASCII_HELLO :
//                Serial.println("Recieved HELLO");
                packetType = HELLO;
                CurrMode = TERMINATE;
                break;
              case ASCII_ACK :
//                Serial.println("Recieved ACK");
                packetType = ACK;
                CurrMode = TERMINATE;
                break;
              default :
                CurrMode = READY;
//                Serial.println("CORRUPT, resetting to READY");
           }
          break;
        case TERMINATE:
//          Serial.println("terminate");
          if (incomingByte == ASCII_ENDFRAME){
            CurrMode = READY;
            if (packetType == HELLO){
              handshake = '1';
              readyToSend = '1';
//              Serial.println("Handshake completed");
            }
            if (packetType == ACK){
              readyToSend = '1';
            }
          } else {
            CurrMode = READY;
//            Serial.println("CORRUPT, resetting to READY");
          }       
      }
    }
    vTaskDelay(10); 
  }
}

//send IMU task

void IMUONEread(void *pvParameters) {
  while (1){
    imu1.read();
    vTaskDelay(1);
  }
}


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
      datalength = DATA_OFFSET;
      x = (imu1.a.x / 1600.0);
      y = (imu1.a.y / 1600.0);
      z = (imu1.a.z / 1600.0);
  
      // setting up x
      dtostrf(x, 4, 2, tempX);
      sprintf(packetSendArray+datalength,"%s", tempX);
      datalength += 6;
      packetSendArray[++datalength] = ',';
      ++datalength;
  
      // setting up y
      dtostrf(y, 4, 2, tempY);
      sprintf(packetSendArray+datalength,"%s", tempY);
      datalength += 6;
      packetSendArray[++datalength] = ',';
      ++datalength;
  
      // setting up z
      dtostrf(z, 4, 2, tempZ);
      sprintf(packetSendArray+datalength,"%s", tempZ);
      datalength += 6;
       
      frameAndSendPacket(1, datalength);
    }
    vTaskDelay(2);
  }
}

void sendIMUONECompassData(void *pvParameters) {
  int datalength = 0;
  float currheading = 0;
  char tempCompass[6] = {0};
  while(1){
    if (readyToSend == '1'){
      datalength = DATA_OFFSET;
      currheading = imu1.heading();
      // setting up compass
      dtostrf(currheading, 4, 2, tempCompass);
      sprintf(packetSendArray+datalength,"%s", tempCompass);
      datalength += 6;
      frameAndSendPacket(2, datalength);
    }
    vTaskDelay(10);
  }
}

void sendIMUONEGyroData(void *pvParameters) {
  int datalength = 0;
  char tempX[6] = {0};
  char tempY[6] = {0};
  char tempZ[6] = {0};
  double x = 0;
  double y = 0;
  double z = 0;
  // Offsets to correct gyro zero bias
  float x_offset = 0.67;
  float y_offset = 2.04;
  float z_offset = -0.11;
  while(1){
    if (readyToSend == '1'){
      datalength = DATA_OFFSET;
      x = (gyro1.g.x * 8.75/1000 + x_offset);
      y = (gyro1.g.y * 8.75/1000 + y_offset);
      z = (gyro1.g.z * 8.75/1000 + z_offset);
  
      // setting up x
      datalength = processFloat(x, tempX, datalength, true);
  
      // setting up y
      datalength = processFloat(y, tempY, datalength, true);
  
      // setting up z
      datalength = processFloat(z, tempZ, datalength, false);
       
      frameAndSendPacket(3, datalength);
    }
    vTaskDelay(2);
  }
}

int processFloat(double val, char *result, int datalength, bool addComma) {
  // setting up z
  dtostrf(val, 4, 2, result);
  sprintf(packetSendArray+datalength,"%s", result);
  if (addComma) {
    packetSendArray[++datalength] = ',';
  }
  datalength += 6;
  return datalength;
}

/*--------------------------------------------------*/
/*------------------- Functions --------------------*/
/*--------------------------------------------------*/

void sendHello(){
  packetSendArray[0] = ASCII_STARTFRAME;
  packetSendArray[1] = ASCII_HELLO;
  packetSendArray[2] = ASCII_ENDFRAME;
  Serial.write(packetSendArray,HANDSHAKE_SIZE);
  Serial.flush();
}

void frameAndSendPacket(int fromComponentID, int dataLength){
  packetSendArray[0] = ASCII_STARTFRAME;
  packetSendArray[1] = ASCII_DATA;
  packetSendArray[2] = fromComponentID;
  packetSendArray[3] = dataLength;
  generateCRC(DATA_OFFSET+dataLength);
  packetSendArray[DATA_OFFSET+dataLength+CRC_LENGTH] = ASCII_ENDFRAME;
  Serial.write(packetSendArray,DATA_OFFSET+dataLength+CRC_LENGTH+1);
  Serial.flush();
}

void generateCRC(int CRCStart){
  packetSendArray[CRCStart] = DUMMY_CRC;
  packetSendArray[CRCStart + 1] = DUMMY_CRC;
}


