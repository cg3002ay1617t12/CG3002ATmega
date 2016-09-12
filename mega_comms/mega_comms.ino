#include <Arduino_FreeRTOS.h>

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
  for (;;){
    bufferSize = Serial3.available();
    if (bufferSize> 1){
      if (booted == '1'){
        booted = '0';
        Serial.println("SYSTEM BOOT");
      } else {
        incomingByte = Serial.read();
        Serial.println(incomingByte, DEC);
        Serial3.read();
      }
    }
  
    vTaskDelay(10); 
  }
}

