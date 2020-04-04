#pragma once

#include <hal_rtos.h>
#include <string.h>
#include "ArduinoJson.h"
#include "StrBuffer.hpp"
#include "BinarySemaphore.hpp"
#include "MsgBuffer.hpp"

#include "SX1262S.hpp"
#include "event_groups.h"

struct
{
  uint8_t len;
  uint8_t data[255];
} typedef packet_t;

enum state_t
{
  TX,
  Listening,
  GotPreamble,
  GotHeader,
};

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

  // SX1262 has the following connections:
  // NSS pin:   5
  // DIO1 pin:  6
  // NRST pin:  10
  // BUSY pin:  9

  static state_t state;

public:
  RadioTask(uint8_t priority);
  TaskHandle_t getTaskHandle();
  void sendPacket(packet_t &packet);
  void waitForPacket(packet_t &packet);
};