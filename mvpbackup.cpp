#include <Arduino_FreeRTOS.h>

// Constants
#define dataSize 8
#define PACKET_SIZE 14

// Symbols used
#define STARTFRAME 60
#define ENDFRAME 62
#define HELLOSIGN 40
#define ACKSIGN 41
#define NACKSIGN 42

enum TXrxMode {READY, PACKET_TYPE, PAYLOAD_LENGTH, PACKET_SEQ, COMPONENT_ID , PAYLOAD, CRC, TERMINATE, CORRUPT};
enum PacketType {NEUTRAL, HELLO, ACK};

// variables for receiving data
TXrxMode CurrMode = READY;
PacketType packetType = NEUTRAL;
char recieveSequence = '0';
char expectedSequence = '0';
int incomingByte = 0;
int dataIndex = 0; 
int componentID = 0;
int data = 0;

// variables for sending data
char handshake = '0';   // start sending data only after handshake
char readyToSend = '0'; // to support stop and wait
char dataSendArray[PACKET_SIZE] = {0};
char sendSequence = '0';
char crc = '0';
int lastID = 0 ;
int lastData = 0;

// task declaration
void serialRead( void *pvParameters );
void sendSAMPLEData(void *pvParameters);

// function declaration
void convert8Data(int num);
char convert8CRC();
void prepDataPacket(int fromComponentID, int dataToSend);
void sendPacket(char packetType, int componentId, int payload );
void flipSendSequence();

void setup() {
  // setup serial ports
  Serial3.begin(115200);
  while (!Serial3); // to ensure that Serial 3 is setup
  Serial.begin(115200);
  while (!Serial);  // to ensure that Serial is setup

  Serial.println("Initializing Mega");
  
  sendHello();

  xTaskCreate(
    serialRead
    ,  (const portCHAR *)"serialRead"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  2  // priority
    ,  NULL );

  xTaskCreate(
    sendSAMPLEData
    ,  (const portCHAR *)"sendSAMPLEData"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  3  // priority
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
    if (Serial3.available() > 0){
      incomingByte = Serial3.read();
      switch(CurrMode) {            
         case READY :       // Syncing up to for packet opening
            if (incomingByte == STARTFRAME){
              CurrMode = PACKET_TYPE;
              packetType = NEUTRAL; 
              Serial.println("Recieved Start");
            }
            break;
         case PACKET_TYPE : // Determind what kind of packet is being recieved
           switch(incomingByte){
              case HELLOSIGN :
                Serial.println("Recieved HELLO");
                packetType = HELLO;
                CurrMode = TERMINATE;
                break;
              case ACKSIGN :
                Serial.println("Recieved ACK");
                packetType = ACK;
                CurrMode = PACKET_SEQ;
                break;
              default :
                CurrMode = CORRUPT;
                Serial.println(incomingByte);
                Serial.println("CORRUPT");
           }
          break;
        case PACKET_SEQ : // check to see what frame PI is expecting
          Serial.println("Payload_seq");
          expectedSequence = incomingByte;
          CurrMode = COMPONENT_ID;
          break;
        case COMPONENT_ID :
          Serial.println("component_id");
          componentID = incomingByte;
          CurrMode = PAYLOAD;
          break;
        case PAYLOAD :
          Serial.println("payload");
          if ( dataIndex < dataSize ){
            data = data + incomingByte;
            dataIndex = dataIndex + 1;
            if (dataIndex >= dataSize){
              dataIndex = 0;
              CurrMode = CRC;
            }
          } 
          break; 
        case CRC :
          Serial.println("crc");
          crc = incomingByte;
          if (crc == '1'){
            CurrMode = TERMINATE;
          } else {
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          }
          break;
        case TERMINATE:
          Serial.println("terminate");
          if (incomingByte == ENDFRAME){
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
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          }
          break;
        case CORRUPT:
          Serial.println("handling corrupt packet");
          CurrMode = READY;
      }
    }
    vTaskDelay(10); 
  }
}

// sample send data task
void sendSAMPLEData(void *pvParameters) {
  while(1){
    if (readyToSend == '1'){
      prepDataPacket(10, 2000);
      sendPacket();
    }
    vTaskDelay(10); 
  }
}

/*--------------------------------------------------*/
/*------------------- Functions --------------------*/
/*--------------------------------------------------*/

void sendHello(){
  Serial3.write("<(>");
  Serial3.flush();
}

void prepDataPacket(int fromComponentID, int dataToSend){
  dataSendArray[0]  = '<';
  dataSendArray[1]  = '-';
  dataSendArray[2]  = sendSequence;
  dataSendArray[3]  = char(fromComponentID);
  convert8Data(dataToSend); // index 4 to index 11 
  dataSendArray[12] = convert8CRC();
  dataSendArray[13] = '>';
  lastID = fromComponentID; // backup incase of NACK
  lastData = dataToSend;    // backup incase of NACK
}

void convert8Data(int num){
  int count = 8;
  while (count > 0){
    count -= 1;
    if (num >= 255){
      num -= 255;
      dataSendArray[count + 4] = 255;
    } else {
      dataSendArray[count + 4] = num;
      num = 0;
    }
  }
}

char convert8CRC(){
  return (char)1;
}

void flipSendSequence(){
  if (sendSequence == '1'){
    sendSequence = '0';
  } else {
    sendSequence = '1';
  }
}

void sendPacket(){
  Serial3.write(dataSendArray,PACKET_SIZE);
  Serial3.flush();
  flipSendSequence();
  readyToSend = '0';
}

