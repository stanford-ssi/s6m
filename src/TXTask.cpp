#include "TXTask.hpp"
#include "main.hpp"
#include <RadioLib.h>
#include <rBase64.h>

TaskHandle_t TXTask::taskHandle = NULL;
StaticTask_t TXTask::xTaskBuffer;
StackType_t TXTask::xStack[stackSize];

TXTask::TXTask(uint8_t priority)
{
    TXTask::taskHandle = xTaskCreateStatic(activity_wrapper,      //static function to run
                                           "tx",                  //task name
                                           stackSize,             //stack depth (words!)
                                           NULL,                  //parameters
                                           priority,              //priority
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

    // uint32_t i = 0;
    // packet_t packet;
    // bool led = false;
    // packet.len = snprintf((char *)packet.data, 255, "B:%lu", i) + 1;
    // while (true)
    // {
    //     if (sys.tasks.radio.sendPacket(packet))
    //     {
    //         log(info, "queued");
    //         i++;
    //         packet.len = snprintf((char *)packet.data, 255, "B:%lu", i) + 1;
    //         led = !led;
    //         digitalWrite(LED_BUILTIN, led);
    //     }
    //     else
    //     {
    //         vTaskDelay(100);
    //     }
    // }

    char str[200];

    while (true)
    {
        uint32_t len = sys.tasks.logger.inputBuffer.receive(str, 200, true);
        str[len] = 0;
        StaticJsonDocument<200> doc;
        if (deserializeJson(doc, str) == DeserializationError::Ok)
        {
            JsonVariant id = doc["id"];
            if (!id.isNull())
            {
                if (strcmp(id, "tx") == 0)
                {
                    JsonVariant data = doc["data"];
                    if (!data.isNull())
                    {
                        packet_t packet;
                        char temp[255];
                        strcpy(temp,data.as<char *>());
                        packet.len = rbase64_decode((char *)packet.data, temp, strlen(temp));
                        sys.tasks.radio.sendPacket(packet);
                    }
                }
            }
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