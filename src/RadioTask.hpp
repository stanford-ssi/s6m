#pragma once

#include <hal_rtos.h>
#include <string.h>
#include "ArduinoJson.h"
#include "MsgBuffer.hpp"
#include "event_groups.h"

#define RADIOLIB_STATIC_ONLY
#include "SX1262S.hpp"

struct
{
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

  static void radioISR();

  static MsgBuffer<packet_t, 1000> txbuf;
  static MsgBuffer<packet_t, 1000> rxbuf;

  static StaticEventGroup_t evbuf;
  static EventGroupHandle_t evgroup;

  static Module mod;
  static SX1262S lora;

public:
  RadioTask(uint8_t priority);
  TaskHandle_t getTaskHandle();
  void sendPacket(packet_t &packet);
  void waitForPacket(packet_t &packet);
};