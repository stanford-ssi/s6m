#include "RadioTask.hpp"
#include "RadioUtils.hpp"
#include "main.hpp"

TaskHandle_t RadioTask::taskHandle = NULL;
StaticTask_t RadioTask::xTaskBuffer;
StackType_t RadioTask::xStack[stackSize];

MsgBuffer<packet_t, 1000> RadioTask::txbuf;
MsgBuffer<packet_t, 1000> RadioTask::rxbuf;

StaticEventGroup_t RadioTask::evbuf;
EventGroupHandle_t RadioTask::evgroup;

state_t RadioTask::state = Listening;

void RadioTask::setFlag(void)
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

void RadioTask::sendPacket(packet_t &packet)
{
    txbuf.send(packet);
    xEventGroupSetBits(evgroup, 0b10);
}

void RadioTask::waitForPacket(packet_t &packet)
{
    rxbuf.receive(packet, true);
}

void RadioTask::activity(void *ptr)
{
    SX1262 lora = new Module(5, 6, 10, 9);

    int state = lora.begin(433.0F, 7.8, 5, 5, SX126X_SYNC_WORD_PRIVATE, 22, 139.0, 8, 1.8F, false);

    if (state == ERR_NONE)
    {
        sys.tasks.logger.log("Radio Online!");
    }
    else
    {
        sys.tasks.logger.log("Radio Init Failed!");
    }

    lora.setDio1Action(setFlag);

    lora.setDioIrqParams(SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT,
                         SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT);

    lora.startReceive();

    state = Listening;

    uint32_t flags = 0;

    while (true)
    {
        flags = xEventGroupWaitBits(evgroup, 0b11, true, false, NEVER);
        //read all
        uint16_t irq = lora.getIrqStatus();
        //uint8_t status = lora.getStatus();
        //uint8_t chipmode = (status & 0b01110000) >> 4;
        //uint8_t cmdstatus = (status & 0b00001110) >> 1;
        logStatus(lora);
        lora.clearIrqStatus();

        if (flags & 0b01) //update from radio
        {
            if (state == TX)
            {
                if (irq & SX126X_IRQ_TX_DONE)
                {
                    sys.tasks.logger.log("Done TXing, start RXing for at least 50ms");
                    lora.startReceive();
                    state = Listening;
                    xEventGroupWaitBits(evgroup, 0b01, false, false, 50); //wait for other radio to get a chance to speak
                    continue;       //don't start TXing right away
                }
            }
            else if (state == Listening)
            {
                if (irq & SX126X_IRQ_PREAMBLE_DETECTED)
                {
                    sys.tasks.logger.log("Got Preamble");

                    xEventGroupWaitBits(evgroup, 0b01, true, false, 500); //wait for radio interupt for 500ms

                    irq = lora.getIrqStatus();
                    lora.clearIrqStatus();

                    if (irq & SX126X_IRQ_RX_DONE) //packet is ready, we can grab it
                    {
                        sys.tasks.logger.log("Got Packet");
                        processRX(lora);
                        state = Listening;
                    }
                    else if (irq & SX126X_IRQ_HEADER_VALID) //header is ready, but not packet. We are confident that an RxDone will come, (sucessfully or otherwise)
                    {
                        sys.tasks.logger.log("Got Header");
                        state = GotHeader;
                    }
                    else //we are giving up...
                    {
                        state = Listening;
                    }
                }
            }
            else if (state == GotHeader)
            {
                if (irq & SX126X_IRQ_RX_DONE)
                {
                    sys.tasks.logger.log("Got Packet 2");
                    processRX(lora);
                    if (txbuf.empty())
                    {
                        sys.tasks.logger.log("Done RXing, nothing to send, keep RXing");
                        state = Listening;
                        lora.startReceive();
                        continue;
                    }
                    else
                    {
                        sys.tasks.logger.log("Done RXing, time to TX");
                        packet_t packet;
                        if (txbuf.receive(packet, false))
                        {
                            lora.startTransmit(packet.data, packet.len);
                            state = TX;
                        }
                        else
                        {
                            //ERROR reading packet from txbuffer
                            sys.tasks.logger.log("Error Queueing Packet");
                        }
                    }
                }
            }
        }

        if (state == Listening && !txbuf.empty())
        { //Start TXing
            sys.tasks.logger.log("Stop Listening, start transmitting");
            packet_t packet;
            if (txbuf.receive(packet, false))
            {
                lora.startTransmit(packet.data, packet.len);
                state = TX;
            }
            else
            {
                //ERROR reading packet from txbuffer
                sys.tasks.logger.log("Error Queueing Packet");
            }
        }
    }
}

void RadioTask::processRX(SX1262 &lora)
{
    packet_t packet;
    lora.readData(packet.data, 255);
    packet.len = lora.getPacketLength();
    rxbuf.send(packet);
}

/*
TODO
- Multi-step packet listening
- need full state tracking
- Don't transmit over RX
- Channel Activity detection
*/

/*

activity:
    - read status
    - read irq




*/