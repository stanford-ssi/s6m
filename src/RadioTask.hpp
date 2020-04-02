#pragma once


#include <hal_rtos.h>
#include <string.h>
#include "ArduinoJson.h"
#include "StrBuffer.hpp"
#include "BinarySemaphore.hpp"
#include "MsgBuffer.hpp"

struct {
 uint8_t len;
 uint8_t data[255];
} typedef packet_t;


class RadioTask
{
private:

  static const size_t stackSize = 1000;

  static TaskHandle_t taskHandle;
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[stackSize];

  static void activity(void *p);

  static void setFlag();

  static MsgBuffer<packet_t,1000> txbuf;
  static MsgBuffer<packet_t,1000> rxbuf;

public:
  RadioTask(uint8_t priority);
  TaskHandle_t getTaskHandle();
  void sendPacket(packet_t &packet);
  void waitForPacket(packet_t &packet);
  
};