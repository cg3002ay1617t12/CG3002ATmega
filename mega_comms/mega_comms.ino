#include <Arduino_FreeRTOS.h>
#include <math.h>

enum rxMode {READY, PACKET_TYPE, PAYLOAD_LENGTH, PACKET_SEQ, COMPONENT_ID , PAYLOAD, CRC, TERMINATE, CORRUPT };

char booted = '0';
int bufferSize = 0;  


void serialRead( void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup() {
  
  // setup serial ports
  Serial3.begin(115200);
  while (!Serial3);
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("hi");

  xTaskCreate(
    serialRead
    ,  (const portCHAR *)"serialRead"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  2  // priority
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
  
  int packet_size = 0;
  int packet_size_count = 0;
  
  char packet_seq = '0';

  int numComponent = 0; // MAX 10
  int componentID[10] = {0};
  char componentFlag = '0';
  int componentTemp = 0;

  int dataIndex = 0; 
  int payloadData = 1; // need to findout why is it it bugging out

  int crcCount = 4;
  int crcData = 1; // need to findout why is it it bugging out
  
  rxMode CurrMode = READY;
  
  for (;;){
    if (Serial3.available()>0){
      
      incomingByte = Serial3.read();
         
      switch(CurrMode) {
         case READY :
            if (incomingByte == 60){
              CurrMode = PACKET_TYPE;
              Serial.println("Recieved Start");
            }
            break; /* optional */
         case PACKET_TYPE :
           switch(incomingByte){
              case 40 :
                Serial.println("Recieved ACK");
                CurrMode = PAYLOAD_LENGTH;
                break;
              case 41 :
                Serial.println("Recieved NACK");
                CurrMode = PAYLOAD_LENGTH;
                break;
              case 42 :
                Serial.println("Recieved Prob");
                CurrMode = PAYLOAD_LENGTH;
                break;
              case 43 :
                Serial.println("Recieved Read");
                CurrMode = PAYLOAD_LENGTH;
                break;
              case 44 :
                Serial.println("Recieved Data");
                CurrMode = PAYLOAD_LENGTH;
                break;
              case 45 :
                Serial.println("Recieved Request");
                CurrMode = PAYLOAD_LENGTH;
                break;
              default :
                CurrMode = CORRUPT;
                Serial.println("CORRUPT");
           }
           break;
        case PAYLOAD_LENGTH :
          Serial.println("Payload_length");
          if (packet_size_count == 0){
            packet_size += (incomingByte-48) * 100;
            packet_size_count += 1;
          } else if (packet_size_count == 1){
            packet_size += (incomingByte-48) * 10;
            packet_size_count += 1;
          } else {
            packet_size += (incomingByte-48);
            packet_size_count = 0;
            CurrMode = PACKET_SEQ;
            Serial.println("DEBUG!!!");
            Serial.println(packet_size);
            Serial.println("DEBUG!!!");
            dataIndex = packet_size;
          }
          break;
        case PACKET_SEQ :
          Serial.println("Payload_seq");
          if (packet_seq == (incomingByte-48)){
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          } else {
            CurrMode = COMPONENT_ID;
            packet_seq = !packet_seq;
          }
          break;
        case COMPONENT_ID :
        Serial.println("component_id");
          if (componentFlag == '0'){
            componentTemp += (incomingByte-48)*10;
            componentFlag = !componentFlag;
          } else {
            componentTemp += (incomingByte-48);
            componentID[numComponent] = componentTemp;
            numComponent++;
            componentTemp = 0;
            componentFlag = !componentFlag;
          }
          if (numComponent == 11){
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          }
          else if(incomingByte == 64){
            if (componentFlag == '1'){
              CurrMode = CORRUPT;
              Serial.println("CORRUPT");
            }
            CurrMode = PAYLOAD;
          } 
  
          break;
        case PAYLOAD :
        Serial.println("payload");
          if (dataIndex > -1){
            payloadData = payloadData + pow(10.0,dataIndex-1) * (incomingByte-48);
            dataIndex = dataIndex - 1; 
            if (dataIndex == 0){
              CurrMode = CRC;
            }
          }
          break; 
        // need to kiv the different sensors and the number of bytes of data sent respectively
        case CRC :
          Serial.println("crc");
           if (crcCount > -1){
            Serial.println("~");
            Serial.println(incomingByte);
            crcData = crcData + pow(10.0,crcCount-1) * (incomingByte-48);
            Serial.println(crcData);
            crcCount = crcCount - 1; 
            if (crcCount == 0){
              crcCount = 4;
              CurrMode = TERMINATE;
            }
          }
          break;
        case TERMINATE:
        Serial.println("terminate");
          if (incomingByte != 62 ){
            CurrMode = CORRUPT;
            Serial.println("CORRUPT");
          } else {
            CurrMode = READY;
            Serial.println(componentID[0]);
            Serial.println(payloadData);
            Serial.println(crcData);
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

