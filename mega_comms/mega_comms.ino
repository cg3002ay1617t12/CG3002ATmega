#include <Arduino_FreeRTOS.h>

int incomingByte = 0;

// define two tasks for Blink & AnalogRead
void serialRead( void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup() {
  // setup serial ports

  Serial.begin(115200);
  Serial3.begin(115200);

  // Now set up two tasks to run independently.
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
  (void) pvParameters;
  if (Serial3.available() > 0){
    incomingByte = Serial3.read();
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
  }
  
}

