#include "RadioTask.hpp"
#include "main.hpp"
#include <RadioLib.h>

TaskHandle_t RadioTask::taskHandle = NULL;
StaticTask_t RadioTask::xTaskBuffer;
StackType_t RadioTask::xStack[stackSize];

BinarySemaphore RadioTask::sem;

volatile bool transmittedFlag = false;
volatile bool enableInterrupt = true;
int transmissionState = ERR_NONE;

void RadioTask::setFlag(void)
{
    sem.giveFromISR();
}

RadioTask::RadioTask(uint8_t priority)
{
    RadioTask::taskHandle = xTaskCreateStatic(activity,                 //static function to run
                                              "Radio",                  //task name
                                              stackSize,                //stack depth (words!)
                                              NULL,                     //parameters
                                              priority,                 //priority
                                              RadioTask::xStack,        //stack object
                                              &RadioTask::xTaskBuffer); //TCB object
}

TaskHandle_t RadioTask::getTaskHandle()
{
    return taskHandle;
}

void RadioTask::activity(void *ptr)
{
    pinMode(LED_BUILTIN, OUTPUT);

    // SX1262 has the following connections:
    // NSS pin:   5
    // DIO1 pin:  6
    // NRST pin:  10
    // BUSY pin:  9
    SX1262 lora = new Module(5, 6, 10, 9);

    int state = lora.begin(433.0F, 7.8, 5, 5, SX126X_SYNC_WORD_PRIVATE, 22, 139.0, 8, 1.8F, false);

    if (state == ERR_NONE)
    {
        sys.tasks.logger.log("Radio Online!");
    }
    else
    {
        sys.tasks.logger.log("Radio Init Failed!");
    }

    lora.setDio1Action(setFlag);

    TickType_t timer = 0;
    while (true)
    {
        vTaskDelayUntil(&timer, 1000);

        digitalWrite(LED_BUILTIN, false);
        vTaskDelayUntil(&timer, 1000);
        digitalWrite(LED_BUILTIN, true);
        sys.tasks.logger.log("Blink!");

        
        transmissionState = lora.startTransmit("Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!");

        if (transmissionState == ERR_NONE)
        {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
        }

        sem.take(NEVER);
        
    }
}
