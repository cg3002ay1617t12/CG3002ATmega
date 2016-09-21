#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#define LONG_TIME 0xffff

SemaphoreHandle_t xSemaphore = NULL;

enum txrxMode {READY, PACKET_TYPE, PAYLOAD_LENGTH, PACKET_SEQ, COMPONENT_ID , PAYLOAD, CRC, TERMINATE, CORRUPT };
enum commsType {NEUTRAL, HELLO, ACK, NACK, PROBE, REQUEST, DATA};

char booted = '0';
char sendSequence = '0';

const int  dataSize = 8;
//const int crcSize = 1;

int numArray[8] = {0};
char crc = '0';
char packet_seq = '0';
int data = 0;

char lastPacketType = '0' ;
int lastComponentId = 0;
int lastPayload = 0;

// task declaration
void serialRead( void *pvParameters );
void sendSAMPLEData(void *pvParameters);

// function declaration
void convert8Num(int num);
char convert8CRC();
void sendPacket(char packetType, int componentId, int payload );
void handleData(int componentID, int data);

// the setup function runs once when you press reset or power the board
void setup() {

  xSemaphore = xSemaphoreCreateMutex();
  
  // setup serial ports
  Serial3.begin(115200);
  while (!Serial3); // to ensure that Serial 3 is setup
  Serial.begin(115200);
  while (!Serial);  // to ensure that Serial is setup
    
  Serial.println("hello world");
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

void loop()
{
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void serialRead(void *pvParameters)  // This is a task.
{ 
  int incomingByte = 0;
 
  int dataIndex = 0; 
  int componentID = 0;
  int crcIndex = 0;
  int crcData[4] = {0};
  
  txrxMode CurrMode = READY;
  commsType packetType = NEUTRAL;

  for (;;){

    if (Serial.available()>0){
      Serial.read();
      Serial.println("txing");
      sendPacket(',',31,2000);
    }
    
    if (Serial3.available()>0){
      
      incomingByte = Serial3.read();
               
      switch(CurrMode) {
         case READY :
            if (incomingByte == 60){
              CurrMode = PACKET_TYPE;
              packetType = NEUTRAL;
              Serial.println("Recieved Start");
            }
            break; /* optional */
         case PACKET_TYPE :
           switch(incomingByte){
              case 40 :
                Serial.println("Recieved HELLO");
                packetType = HELLO;
                CurrMode = TERMINATE;
                break;
              case 41 :
                Serial.println("Recieved ACK");
                CurrMode = PACKET_SEQ;
                packetType = ACK;
                break;
              case 42 :
                Serial.println("Recieved NACK");
                CurrMode = PACKET_SEQ;
                packetType = NACK;
                break;
              case 43 :
                Serial.println("Recieved PROBE");
                CurrMode = PACKET_SEQ;
                packetType = PROBE;
                break;
              case 44 :
                Serial.println("Recieved Request");
                CurrMode = PACKET_SEQ;
                packetType = REQUEST;
                break;
              case 45 :
                Serial.println("Recieved data");
                CurrMode = PACKET_SEQ;
                packetType = DATA;
                break;
              default :
                Serial.println(incomingByte);
                CurrMode = CORRUPT;
                Serial.println("CORRUPT");
           }
           break;
        case PACKET_SEQ :
         Serial.println("Payload_seq");
          if (packet_seq == incomingByte){
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          } else {
            CurrMode = COMPONENT_ID;
          }
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
          Serial.println(crc);
          if (crc == '1'){
            CurrMode = TERMINATE;
          } else {
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          }
          break;
        case TERMINATE:
        Serial.println("terminate");
          if (incomingByte != 62 ){
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          } else {
            CurrMode = READY;
            data = 0; // reset data
            if (packetType == DATA){
              sendACK();
              handleData(componentID,data);
            }
            if (packetType == HELLO){
              booted = '1';
            }
            if (packetType == ACK){
              xSemaphoreGive( xSemaphore );
            }
            if (packetType == NACK){
              sendSequence = ! sendSequence; // temporarily revert to last seq
              sendPacket(lastPacketType,lastComponentId,lastPayload);
              sendSequence = ! sendSequence;
            }
            if (packetType == PROBE){
              sendACK();
              packet_seq = !packet_seq;
            }
            vTaskDelay(10); 
          }
          break;
        case CORRUPT:
          Serial.println("handling corrupt packet");
          sendNACK();
          CurrMode = READY;
      }
    }
    vTaskDelay(10); 
  }
}

/*--------------------------------------------------*/
/*------------------- Functions --------------------*/
/*--------------------------------------------------*/

void convert8Num(int num){
  int count = 8;
  while (count > 0){
    count -= 1;
    if (num >= 255){
      num -= 255;
      numArray[count] = 255;
    } else {
      numArray[count] = num;
      num = 0;
    }
  }
}

void sendPacket(char packetType, int componentId, int payload) {
  Serial3.write('<');               // start <
  Serial3.write(packetType);        // data packet
  Serial3.write(sendSequence);      // seq id
  Serial3.write(char(componentId)); // component id
  convert8Num(payload);             // payload
  for (int i=0; i<8; i++){
      Serial3.write(numArray[i]);
    }
  Serial3.write(convert8CRC());     // crc(TBD)
  Serial3.write('>');               // end >
  Serial3.flush();
  sendSequence = ! sendSequence;    // update sendsequence id
  lastPacketType = packetType ;
  lastComponentId = componentId;
  lastPayload = payload;

}

void sendHello(){
  Serial3.write('<'); 
  Serial3.write('('); 
  Serial3.write('<'); 
  Serial3.flush();
}



void sendACK(){
  packet_seq = !packet_seq;
  sendPacket(')',1,666);
}

// assumes that hello will never corrupt
void sendNACK(){
  sendPacket('*',1,666);
}

char convert8CRC(){
  return (char)1;
}

void handleData(int componentID, int data){
  
}

// sample
void sendSAMPLEData(void *pvParameters) {
  while(1){
    if( xSemaphore != NULL ){
      if( xSemaphoreTake( xSemaphore, ( TickType_t )LONG_TIME ) == pdTRUE){
        sendPacket('-',1,666);
      }
    }
    vTaskDelay(10); 
  }
}
  

//int serialWrite(){
//  numBytesSent = Serial3.write(packet, packetSize);
//  Serial3.flush();
//  return numBytesSent;
//}

//void getTxBin() {
//  int remainder = 0;
//  for (int i=0; i < 12; i++){
//    int convertee = packet[i];
//    for (int n=7; n > -1; n--){
//      if (convertee % 2 == 1){
//        txbuf[i*8+n] = '1';
//      } else {
//        txbuf[i*8+n] = '0';
//      }
//      convertee/= 2;
//    }
//  }
//}
//
//typedef uint8_t crc;
//}

//if( xSemaphore != NULL ){
//  if( xSemaphoreTake( xSemaphore, ( TickType_t ) LONG_TIME ) == pdTRUE )
//}





