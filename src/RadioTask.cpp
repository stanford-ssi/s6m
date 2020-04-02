#include "RadioTask.hpp"
#include "main.hpp"
#include <RadioLib.h>

TaskHandle_t RadioTask::taskHandle = NULL;
StaticTask_t RadioTask::xTaskBuffer;
StackType_t RadioTask::xStack[stackSize];

MsgBuffer<packet_t, 1000> RadioTask::txbuf;
MsgBuffer<packet_t, 1000> RadioTask::rxbuf;

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

    // SX1262 has the following connections:
    // NSS pin:   5
    // DIO1 pin:  6
    // NRST pin:  10
    // BUSY pin:  9
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

    bool txing = 0; //0=rx 1=tx
    lora.startReceive();

    packet_t packet;

    while (true)
    {
        //wait for a notification
        uint32_t flags;
        xTaskNotifyWait(0b11, 0, &flags, NEVER);

        if ((flags & 0b01) && !txing) //if were Rxing, and we got an rx_done signal
        {
            lora.readData(packet.data, 255);
            packet.len = lora.getPacketLength();
            rxbuf.send(packet);
            //finished receiveing
        }
        else if ((flags & 0b10) && txing) //if we were Txing, and we got an queue interupt
        {
            continue; //don't interup the TX process, keep waiting.
        }

        //time to start doing the next thing
        if (txbuf.empty())
        {
            lora.startReceive();
            txing = false;
        }
        else
        {
            if (txbuf.receive(packet, false))
            {
                //should add CAD here
                lora.startTransmit(packet.data, packet.len);
                txing = true;
            }
            else
            {
                //ERROR reading packet from txbuffer
            }
        }
    }
}