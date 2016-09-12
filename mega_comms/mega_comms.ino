#include <Arduino_FreeRTOS.h>

void serialRead( void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup() {
  // setup serial ports
  Serial.begin(115200);
  Serial3.begin(115200);

  Serial.println("hihi");
  
  xTaskCreate(
    serialRead
    ,  (const portCHAR *)"serialRead"   // A name just for humans
    ,  128  // Stack size
    ,  NULL
    ,  2  // priority
    ,  NULL );
 
    
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.  
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void serialRead(void *pvParameters)  // This is a task.
{
  int incomingByte = 0;
  
  if (Serial3.available() > 0){
    incomingByte = Serial3.read();
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
  }
  
}

