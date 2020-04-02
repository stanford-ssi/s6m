#pragma once

#include <hal_rtos.h>
#include <string.h>
#include "ArduinoJson.h"
#include "StrBuffer.hpp"
#include "BinarySemaphore.hpp"
#include "MsgBuffer.hpp"

class TestTask
{
private:

  static const size_t stackSize = 1000;

  static TaskHandle_t taskHandle;
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[stackSize];

  static void activity(void *p);


public:
  TestTask(uint8_t priority);
  TaskHandle_t getTaskHandle();
  
};