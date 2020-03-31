#include <Arduino.h>

// include the library
#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   5
// DIO1 pin:  6
// NRST pin:  10
// BUSY pin:  9
SX1262 lora = new Module(5, 6, 10, 9);

void setup()
{

  delay(3000);

  Serial.begin(115200);
  delay(3000);

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            9
  // coding rate:                 7
  // sync word:                   0x12 (private network)
  // output power:                14 dBm
  // current limit:               60 mA
  // preamble length:             8 symbols
  // TCXO voltage:                1.6 V (set to 0 to not use TCXO)
  // CRC:                         enabled

  //NICE-RF 433
  int state = lora.begin(433.0F, 7.8, 5, 5, SX126X_SYNC_WORD_PRIVATE, 22, 150.0, 8, 1.8F);

  //Lambda62-9
  //int state = lora.begin(915.0F, 7.8, 5, 5, SX126X_SYNC_WORD_PRIVATE, 22, 150.0, 8, 0);

  //int state = lora.beginFSK(433.0F, 0.6, 0.0, 4.8, 22, 150.0, 16, 0, 1.8F);

  if (state == ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
}

void loop()
{
  Serial.print(F("[SX1262] Transmitting packet ... "));

  int state = 0;

  //lora.transmitDirect(0);
  // you can also transmit byte array up to 256 bytes long

  byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
  state = lora.transmit(byteArr, 8);

  if (state == ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(lora.getDataRate());
    Serial.println(F(" bps"));
  }
  else if (state == ERR_PACKET_TOO_LONG)
  {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));
  }
  else if (state == ERR_TX_TIMEOUT)
  {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait for a second before transmitting again
  delay(1000);
}