#include "RadioTask.hpp"
#include "RadioUtils.hpp"
#include "main.hpp"

TaskHandle_t RadioTask::taskHandle = NULL;
StaticTask_t RadioTask::xTaskBuffer;
StackType_t RadioTask::xStack[stackSize];

MsgBuffer<packet_t, 1050> RadioTask::txbuf;
MsgBuffer<packet_t, 1050> RadioTask::rxbuf;

StaticEventGroup_t RadioTask::evbuf;
EventGroupHandle_t RadioTask::evgroup;

// SX1262 has the following connections:
// NSS pin:   5
// DIO1 pin:  6
// NRST pin:  10
// BUSY pin:  9
Module RadioTask::mod(5, 6, 10, 9);
SX1262S RadioTask::lora(&mod);

MsgBuffer<radio_settings_t, 1000> RadioTask::settingsBuf;

void RadioTask::radioISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(evgroup, 0b01, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

RadioTask::RadioTask(uint8_t priority)
{
    RadioTask::taskHandle = xTaskCreateStatic(activity,                 //static function to run
                                              "Radio",                  //task name
                                              stackSize,                //stack depth (words!)
                                              NULL,                     //parameters
                                              priority,                 //priority
                                              RadioTask::xStack,        //stack object
                                              &RadioTask::xTaskBuffer); //TCB object
    evgroup = xEventGroupCreateStatic(&evbuf);
}

TaskHandle_t RadioTask::getTaskHandle()
{
    return taskHandle;
}

bool RadioTask::sendPacket(packet_t &packet)
{
    bool ret = txbuf.send(packet);     //this puts the packet in the buffer
    xEventGroupSetBits(evgroup, 0b10); //this notifies the task there might be new data to send
    return ret;
}

void RadioTask::waitForPacket(packet_t &packet)
{
    rxbuf.receive(packet, true);
}

void RadioTask::applySettings(radio_settings_t &settings)
{
    lora.setFrequency(settings.freq);
    lora.setBandwidth(settings.bw);
    lora.setSpreadingFactor(settings.sf);
    lora.setCodingRate(settings.cr);
    lora.setSyncWord(settings.syncword);
    lora.setOutputPower(settings.power);
    lora.setCurrentLimit(settings.currentLimit);
    lora.setPreambleLength(settings.preambleLength);
    //TODO: Add logging here
}

void RadioTask::setSettings(radio_settings_t &settings)
{
    settingsBuf.send(settings);
}

void RadioTask::activity(void *ptr)
{
    radio_settings_t settings; //default settings
    int state = lora.begin(settings.freq, settings.bw, settings.sf, settings.cr, settings.syncword, settings.power, settings.currentLimit, settings.preambleLength, 1.8F, false);

    if (state != ERR_NONE)
    {
        sys.tasks.logger.log("Radio Init Failed!");
    }

    lora.setDio1Action(radioISR);

    lora.setDioIrqParams(SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CAD_DETECTED | SX126X_IRQ_CAD_DONE,
                         SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CAD_DETECTED | SX126X_IRQ_CAD_DONE);

    lora.startReceive();

    while (true)
    {
        //if new settings are available, apply them
        if (!settingsBuf.empty())
        {
            radio_settings_t settings;
            settingsBuf.receive(settings, false);
            applySettings(settings);
        }

        uint32_t flags = xEventGroupWaitBits(evgroup, 0b11, true, false, NEVER);
        uint16_t irq = lora.getIrqStatus();
        //logStatus(lora);
        lora.clearIrqStatus();

        //RECEIVE BLOCK
        if (irq & SX126X_IRQ_PREAMBLE_DETECTED)
        {
            //sys.tasks.logger.log("Got Preamble");
            uint32_t time = lora.symbolToMs(32); //time of a packet header

            flags = xEventGroupWaitBits(evgroup, 0b01, true, false, time); //wait for another radio interupt for 500ms

            if (!(flags & 0b01))
            {
                sys.tasks.logger.log("No Header after waiting 32 symbols");
                continue; //it failed
            }

            irq = lora.getIrqStatus();
            lora.clearIrqStatus();

            if ((irq & SX126X_IRQ_HEADER_VALID) && !(irq & SX126X_IRQ_RX_DONE)) //header is ready, but not packet. We are confident that an RxDone will come, (sucessfully or otherwise)
            {
                //sys.tasks.logger.log("waiting for RXDone");
                uint32_t time = lora.getTimeOnAir(255) / 1000;                 //TODO: read this out of the header to make timeout tighter
                flags = xEventGroupWaitBits(evgroup, 0b01, true, false, time); //wait for radio interupt for 500ms

                if (!(flags & 0b01))
                {
                    sys.tasks.logger.log("waited for RxDone, it never came!");
                    continue; //it failed
                }

                irq = lora.getIrqStatus();
                lora.clearIrqStatus();
            }

            if (irq & SX126X_IRQ_RX_DONE) //packet is ready, we can grab it
            {
                //sys.tasks.logger.log("Got Packet");
                packet_t packet;
                lora.readData(packet.data, 255);
                packet.len = lora.getPacketLength();
                rxbuf.send(packet);
            }
            else
            {
                sys.tasks.logger.log("Expecting RxDone, but no beans.");
                continue;
            }
        }

        //TRANSMIT BLOCK
        if (txbuf.empty())
        {
            //sys.tasks.logger.log("Nothing to send, keep RXing");
            lora.startReceive();
            continue;
        }
        else
        {
            //sys.tasks.logger.log("Done RXing, time to TX");

            do
            {
                //sys.tasks.logger.log("Listening for quiet");
                lora.standby();
                lora.setCad();
                uint32_t time = lora.symbolToMs(12) + 50;
                xEventGroupWaitBits(evgroup, 0b01, true, false, time);
                irq = lora.getIrqStatus();
                lora.clearIrqStatus();
            } while (!(irq & SX126X_IRQ_CAD_DONE) || irq & SX126X_IRQ_CAD_DETECTED);

            //sys.tasks.logger.log("Heard quiet!");

            packet_t packet;
            if (txbuf.receive(packet, false))
            {

                uint32_t time = lora.getTimeOnAir(packet.len) / 1000; //get TOA in ms
                time = (time * 1.1) + 100;                            //add margin

                lora.startTransmit(packet.data, packet.len);
                flags = xEventGroupWaitBits(evgroup, 0b01, true, false, time);
                irq = lora.getIrqStatus();
                lora.clearIrqStatus();

                if (irq & SX126X_IRQ_TX_DONE)
                {
                    //sys.tasks.logger.log("Done TXing, start RXing for at least 16 symbols");
                    lora.startReceive();
                }
                else
                {
                    sys.tasks.logger.log("Expecting TxDone, but no beans.");
                }

                time = lora.symbolToMs(16);                           //this can be tuned
                xEventGroupWaitBits(evgroup, 0b01, false, false, time); //wait for other radio to get a chance to speak
            }
            else
            {
                //ERROR reading packet from txbuffer
                sys.tasks.logger.log("Error Queueing Packet");
            }
        }
    }
}