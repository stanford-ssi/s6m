#include "LoggerTask.hpp"
#include "main.hpp"

TaskHandle_t LoggerTask::taskHandle = NULL;
StaticTask_t LoggerTask::xTaskBuffer;
StackType_t LoggerTask::xStack[stackSize];

StrBuffer<10000> LoggerTask::strBuffer;

char LoggerTask::lineBuffer[10000];
char LoggerTask::inputLineBuffer[1000];

bool LoggerTask::loggingEnabled = false;
bool LoggerTask::shitlEnabled = false;


LoggerTask::LoggerTask(uint8_t priority)
{
    LoggerTask::taskHandle = xTaskCreateStatic(activity,                  //static function to run
                                               "Logger",                  //task name
                                               stackSize,                 //stack depth (words!)
                                               NULL,                      //parameters
                                               priority,                  //priority
                                               LoggerTask::xStack,        //stack object
                                               &LoggerTask::xTaskBuffer); //TCB object
}

TaskHandle_t LoggerTask::getTaskHandle()
{
    return taskHandle;
}

void LoggerTask::log(const char *message)
{
    strBuffer.send(message, strlen(message) + 1);
}

void LoggerTask::logJSON(JsonDocument &jsonDoc, const char *id)
{
    jsonDoc["id"] = id;
    jsonDoc["stack"] = uxTaskGetStackHighWaterMark(NULL); //TODO: Check this for capacity... (dangerous!)

    if (jsonDoc.getMember("tick") == NULL)
    {
        jsonDoc["tick"] = xTaskGetTickCount();
    }

    //jsonDoc["la"] = xMessageBufferSpaceAvailable(bufferHandle);

    size_t len = measureJson(jsonDoc);
    char str[len + 5]; //plenty of room!
    serializeJson(jsonDoc, str, sizeof(str));
    log(str);
}

void LoggerTask::log(JsonDocument &jsonDoc)
{
    jsonDoc["stack"] = uxTaskGetStackHighWaterMark(NULL); //TODO: Check this for capacity... (dangerous!)

    if (jsonDoc.getMember("tick") == NULL)
    {
        jsonDoc["tick"] = xTaskGetTickCount();
    }

    size_t len = measureJson(jsonDoc);
    char str[len + 5]; //plenty of room!
    serializeJson(jsonDoc, str, sizeof(str));
    log(str);
}

void LoggerTask::activity(void *ptr)
{

    Serial.begin(9600);

    char *p = lineBuffer;
    TickType_t timeout = 0;
    while (true)
    {
        //Step 1: read in all the logs
        if (strBuffer.receive(p, 1000, true) > 0)
        {
            p = lineBuffer + strlen(lineBuffer);

            p[0] = '\n';
            p++;
            p[0] = '\0';

            if (p - lineBuffer > 8999 || xTaskGetTickCount() > timeout)
            { //we need to write!
                //Step 2: Write to USB
                writeUSB(lineBuffer);

                //reset buffer
                lineBuffer[0] = '\0';
                p = lineBuffer;
                timeout = xTaskGetTickCount() + 1000; //if there are no logs for a bit, we should still flush every once and a while
            }
        }
    }
}

void LoggerTask::writeUSB(char *buf)
{
    Serial.print(buf);
}