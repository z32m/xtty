#include "xtty.h"
#include <zephyr/drivers/uart.h>

void xtty_irq(const struct device *dev, void *user_data)
{
    uart_irq_update(dev);

    tty_spec_t *spec = user_data;
    if (dev != spec->uart)
        return;

    while (uart_irq_rx_ready(dev))
    {
        queue_msg_t *msg = alloc_queue_msg(spec->recv);
        msg->len = uart_fifo_read(dev, msg->data, spec->recv->msg_max_length);
        if (!msg->len)
            free_queue_msg(spec->recv, msg);
        else
            k_queue_append(spec->recv->queue, msg);
    }
}