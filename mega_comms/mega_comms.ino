#include <Arduino_FreeRTOS.h>

char booted = '0';

void serialRead( void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup() {
  // setup serial ports
  Serial3.begin(115200);
  Serial.begin(115200);

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
    if (Serial3.available() > 0){
      incomingByte = Serial3.read();
      if (booted == '1'){
        booted = '0';
        Serial.println("init");
      } else {
        Serial.println("running");
      }
  //    Serial.print("I received: ");
  //    Serial.println(incomingByte, DEC);
    }
  
    vTaskDelay(10); 
  }
}

