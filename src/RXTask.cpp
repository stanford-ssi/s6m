#include "RXTask.hpp"
#include "main.hpp"
#include <RadioLib.h>

TaskHandle_t RXTask::taskHandle = NULL;
StaticTask_t RXTask::xTaskBuffer;
StackType_t RXTask::xStack[stackSize];

RXTask::RXTask(uint8_t priority)
{
    RXTask::taskHandle = xTaskCreateStatic(activity,              //static function to run
                                           "Test",                //task name
                                           stackSize,             //stack depth (words!)
                                           NULL,                  //parameters
                                           priority,              //priority
                                           RXTask::xStack,        //stack object
                                           &RXTask::xTaskBuffer); //TCB object
}

TaskHandle_t RXTask::getTaskHandle()
{
    return taskHandle;
}

void RXTask::activity(void *ptr)
{
    uint32_t i = 0;
    while (true)
    {
        packet_t packet;
        sys.tasks.radio.waitForPacket(packet);
        i++;
        char str[50];
        snprintf(str, 50, "Got %lu of %s", i, (char *)packet.data);
        sys.tasks.logger.log(str);
    }
}