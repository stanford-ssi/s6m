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

struct
{
  float freq = 433.551F;
  float bw = 10.4F;
  uint8_t sf = 12;
  uint8_t cr = 8;
  uint8_t syncword = SX126X_SYNC_WORD_PRIVATE;
  int8_t power = 22;
  float currentLimit = 139.0F;
  uint8_t preambleLength = 8;
} typedef radio_settings_t;

class RadioTask
{
private:
  static const size_t stackSize = 1000;

  static TaskHandle_t taskHandle;
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[stackSize];

  static void activity(void *p);

  static void radioISR();

  static void applySettings(radio_settings_t &settings);

  static MsgBuffer<packet_t, 1050> txbuf;
  static MsgBuffer<packet_t, 1050> rxbuf;

  static StaticEventGroup_t evbuf;
  static EventGroupHandle_t evgroup;

  static Module mod;
  static SX1262S lora;

  static MsgBuffer<radio_settings_t, 1000> settingsBuf;

public:
  RadioTask(uint8_t priority);
  TaskHandle_t getTaskHandle();
  bool sendPacket(packet_t &packet);
  void waitForPacket(packet_t &packet);
  void setSettings(radio_settings_t &settings);
};