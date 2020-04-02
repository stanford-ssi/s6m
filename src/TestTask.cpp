#include "TestTask.hpp"
#include "main.hpp"
#include <RadioLib.h>

TaskHandle_t TestTask::taskHandle = NULL;
StaticTask_t TestTask::xTaskBuffer;
StackType_t TestTask::xStack[stackSize];

TestTask::TestTask(uint8_t priority)
{
    TestTask::taskHandle = xTaskCreateStatic(activity,                 //static function to run
                                              "Test",                  //task name
                                              stackSize,                //stack depth (words!)
                                              NULL,                     //parameters
                                              priority,                 //priority
                                              TestTask::xStack,        //stack object
                                              &TestTask::xTaskBuffer); //TCB object
}

TaskHandle_t TestTask::getTaskHandle()
{
    return taskHandle;
}


void TestTask::activity(void *ptr)
{
    pinMode(LED_BUILTIN, OUTPUT);

    while(true){
        digitalWrite(LED_BUILTIN, true);
        packet_t packet = {30,"Hi, This is a test!"};
        sys.tasks.radio.sendPacket(packet);
        sys.tasks.logger.log("Queued message for transmission!");
        digitalWrite(LED_BUILTIN, false);
        vTaskDelay(5000);
    }
}