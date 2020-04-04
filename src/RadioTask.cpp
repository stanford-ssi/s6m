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

void RadioTask::sendPacket(packet_t &packet)
{
    txbuf.send(packet); //this puts the packet in the buffer
    xEventGroupSetBits(evgroup, 0b10); //this notifies the task there might be new data to send
}

void RadioTask::waitForPacket(packet_t &packet)
{
    rxbuf.receive(packet, true);
}

void RadioTask::activity(void *ptr)
{
    SX1262S lora = new Module(5, 6, 10, 9);

    int state = lora.begin(433.0F, 7.8, 5, 5, SX126X_SYNC_WORD_PRIVATE, 22, 139.0, 8, 1.8F, false);

    if (state == ERR_NONE)
    {
        sys.tasks.logger.log("Radio Online!");
    }
    else
    {
        sys.tasks.logger.log("Radio Init Failed!");
    }

    lora.setDio1Action(radioISR);

    lora.setDioIrqParams(SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT,
                         SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT);

    lora.startReceive();

    while (true)
    {
        uint32_t flags = xEventGroupWaitBits(evgroup, 0b11, true, false, NEVER);
        uint16_t irq = lora.getIrqStatus();
        logStatus(lora);
        lora.clearIrqStatus();

        //RECEIVE BLOCK
        if (irq & SX126X_IRQ_PREAMBLE_DETECTED)
        {
            sys.tasks.logger.log("Got Preamble");

            flags = xEventGroupWaitBits(evgroup, 0b01, true, false, 500); //wait for another radio interupt for 500ms

            if (!(flags & 0b01))
            {
                continue; //it failed
            }

            irq = lora.getIrqStatus();
            lora.clearIrqStatus();

            if ((irq & SX126X_IRQ_HEADER_VALID) && !(irq & SX126X_IRQ_RX_DONE)) //header is ready, but not packet. We are confident that an RxDone will come, (sucessfully or otherwise)
            {
                sys.tasks.logger.log("waiting for RXDone");
                flags = xEventGroupWaitBits(evgroup, 0b01, true, false, 5000); //wait for radio interupt for 500ms

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
                sys.tasks.logger.log("Got Packet");
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
            sys.tasks.logger.log("Nothing to send, keep RXing");
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
                flags = xEventGroupWaitBits(evgroup, 0b01, true, false, 500);
                irq = lora.getIrqStatus();
                lora.clearIrqStatus();

                if (irq & SX126X_IRQ_TX_DONE)
                {
                    sys.tasks.logger.log("Done TXing, start RXing for at least 50ms");
                    lora.startReceive();
                }
                else
                {
                    sys.tasks.logger.log("Expecting TxDone, but no beans.");
                }
                xEventGroupWaitBits(evgroup, 0b01, false, false, 50); //wait for other radio to get a chance to speak
            }
            else
            {
                //ERROR reading packet from txbuffer
                sys.tasks.logger.log("Error Queueing Packet");
            }
        }
    }
}