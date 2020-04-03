#include "RadioTask.hpp"
#include "RadioUtils.hpp"
#include "main.hpp"

TaskHandle_t RadioTask::taskHandle = NULL;
StaticTask_t RadioTask::xTaskBuffer;
StackType_t RadioTask::xStack[stackSize];

MsgBuffer<packet_t, 1000> RadioTask::txbuf;
MsgBuffer<packet_t, 1000> RadioTask::rxbuf;

state_t RadioTask::state = Listening;

void RadioTask::setFlag(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(taskHandle, 0b01, eSetBits, &xHigherPriorityTaskWoken);
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
}

TaskHandle_t RadioTask::getTaskHandle()
{
    return taskHandle;
}

void RadioTask::sendPacket(packet_t &packet)
{
    txbuf.send(packet);
    xTaskNotify(taskHandle, 0b10, eSetBits);
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

    while (true)
    {
        uint32_t flags;
        xTaskNotifyWait(0b11, 0, &flags, NEVER);
        //read all
        uint16_t irq = lora.getIrqStatus();
        uint8_t status = lora.getStatus();
        //uint8_t chipmode = (status & 0b01110000) >> 4;
        //uint8_t cmdstatus = (status & 0b00001110) >> 1;

        logStatus(lora);

        if (flags & 0b01) //update from radio
        {
            if (state == TX)
            {
                if (irq & SX126X_IRQ_TX_DONE)
                {
                    if (txbuf.empty())
                    {
                        lora.startReceive();
                        state = Listening;
                    }
                    else
                    {
                        //Keep TXing
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
            else if (state == Listening)
            {
                if (irq & SX126X_IRQ_PREAMBLE_DETECTED)
                {
                    state = GotPreamble;

                    TickType_t xTicksToWait = 500;
                    TimeOut_t xTimeOut;

                    vTaskSetTimeOutState(&xTimeOut);
                    do
                    {
                        xTaskNotifyWait(0b11, 0, &flags, 500);
                    } while (!(flags & 0b01) && !xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait)); //wait repeatedly (up to 500ms from initial start) for an interupt from the radio

                    irq = lora.getIrqStatus();
                    lora.clearIrqStatus();
                    status = lora.getStatus();

                    if (irq & SX126X_IRQ_RX_DONE) //packet is ready, we can grab it
                    {
                        processRX(lora);
                        state = Listening;
                    }
                    else if (irq & SX126X_IRQ_HEADER_VALID) //header is ready, but not packet. We are confident that an RxDone will come, (sucessfully or otherwise)
                    {
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
                    processRX(lora);
                    state = Listening;
                }
            }
        }

        if (state == Listening && !txbuf.empty())
        { //Start TXing
            sys.tasks.logger.log("Txing!");
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