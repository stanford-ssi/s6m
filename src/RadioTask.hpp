#pragma once


#include <hal_rtos.h>
#include <string.h>
#include "ArduinoJson.h"
#include "StrBuffer.hpp"
#include "BinarySemaphore.hpp"

class RadioTask
{
private:

  static const size_t stackSize = 1000;

  static TaskHandle_t taskHandle;
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[stackSize];

  static BinarySemaphore sem;

  static void activity(void *p);

  static void setFlag();

public:
  RadioTask(uint8_t priority);
  TaskHandle_t getTaskHandle();
  
};