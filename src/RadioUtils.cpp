#include "RadioUtils.hpp"

#define SHORT_TO_BINARY(byte)        \
    (byte & 0x8000 ? '1' : '0'),     \
        (byte & 0x4000 ? '1' : '0'), \
        (byte & 0x2000 ? '1' : '0'), \
        (byte & 0x1000 ? '1' : '0'), \
        (byte & 0x0800 ? '1' : '0'), \
        (byte & 0x0400 ? '1' : '0'), \
        (byte & 0x0200 ? '1' : '0'), \
        (byte & 0x0100 ? '1' : '0'), \
        (byte & 0x80 ? '1' : '0'),   \
        (byte & 0x40 ? '1' : '0'),   \
        (byte & 0x20 ? '1' : '0'),   \
        (byte & 0x10 ? '1' : '0'),   \
        (byte & 0x08 ? '1' : '0'),   \
        (byte & 0x04 ? '1' : '0'),   \
        (byte & 0x02 ? '1' : '0'),   \
        (byte & 0x01 ? '1' : '0')

void logStatus(SX1262 &lora)
{
    uint16_t irq = lora.getIrqStatus();
    uint8_t status = lora.getStatus();

    uint8_t chipmode = (status & 0b01110000) >> 4;
    uint8_t cmdstatus = (status & 0b00001110) >> 1;

    char irqstr[100];
    char *p = irqstr;
    p += sprintf(p, "IRQ: ");

    if (irq & SX126X_IRQ_TX_DONE)
    {
        p += sprintf(p, "TxDone ");
    }

    if (irq & SX126X_IRQ_RX_DONE)
    {
        p += sprintf(p, "RxDone ");
    }

    if (irq & SX126X_IRQ_PREAMBLE_DETECTED)
    {
        p += sprintf(p, "Preamble ");
    }

    if (irq & SX126X_IRQ_SYNC_WORD_VALID)
    {
        p += sprintf(p, "SyncValid ");
    }

    if (irq & SX126X_IRQ_HEADER_VALID)
    {
        p += sprintf(p, "HeaderValid ");
    }

    if (irq & SX126X_IRQ_HEADER_ERR)
    {
        p += sprintf(p, "HeaderErr ");
    }

    if (irq & SX126X_IRQ_CRC_ERR)
    {
        p += sprintf(p, "CrcErr ");
    }

    if (irq & SX126X_IRQ_CAD_DONE)
    {
        p += sprintf(p, "CadDone ");
    }

    if (irq & SX126X_IRQ_CAD_DETECTED)
    {
        p += sprintf(p, "CadDetected ");
    }

    if (irq & SX126X_IRQ_TIMEOUT)
    {
        p += sprintf(p, "Timeout ");
    }

    char str[200];
    sprintf(str, "chipmode: 0x%hX  cmdstatus: 0x%hX  %s", chipmode, cmdstatus, irqstr);
    sys.tasks.logger.log(str);
}