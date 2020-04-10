#include "TXTask.hpp"
#include "main.hpp"
#include <RadioLib.h>

TaskHandle_t TXTask::taskHandle = NULL;
StaticTask_t TXTask::xTaskBuffer;
StackType_t TXTask::xStack[stackSize];

TXTask::TXTask(uint8_t priority)
{
    TXTask::taskHandle = xTaskCreateStatic(activity_wrapper,        //static function to run
                                             "tx",                  //task name
                                             stackSize,               //stack depth (words!)
                                             NULL,                    //parameters
                                             priority,                //priority
                                             TXTask::xStack,        //stack object
                                             &TXTask::xTaskBuffer); //TCB object
}

TaskHandle_t TXTask::getTaskHandle()
{
    return taskHandle;
}

void TXTask::activity_wrapper(void *p)
{
    sys.tasks.test.activity();
}

void TXTask::activity()
{
    pinMode(LED_BUILTIN, OUTPUT);

    uint32_t i = 0;
    packet_t packet;
    bool led = false;
    packet.len = snprintf((char *)packet.data, 255, "B:%lu", i) + 1;
    while (true)
    {
        if (sys.tasks.radio.sendPacket(packet))
        {
            log(info, "queued");
            i++;
            packet.len = snprintf((char *)packet.data, 255, "B:%lu", i) + 1;
            led = !led;
            digitalWrite(LED_BUILTIN, led);
        }
        else
        {
            vTaskDelay(100);
        }
    }
}

void TXTask::log(log_type t, const char *msg)
{
    if (t & log_mask)
    {
        StaticJsonDocument<1000> doc;
        doc["id"] = pcTaskGetName(taskHandle);
        doc["msg"] = msg;
        doc["level"] = (uint8_t)t;
        doc["tick"] = xTaskGetTickCount();
        sys.tasks.logger.log(doc);
    }
}