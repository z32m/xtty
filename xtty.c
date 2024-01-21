#include "xtty.h"
#include <xlog.h>

#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(xtty, LOG_LEVEL_DBG);

void xtty_irq(const struct device *dev, struct uart_event *evt, void *data)
{
    tty_spec_t *spec = data;
    if (dev != spec->uart)
        return;

    LOG_DBG("xtty_irq: evt: %u", evt->type);
    queue_msg_t *msg;

    switch (evt->type)
    {
    case UART_RX_RDY:
        struct uart_event_rx *rx = &evt->data.rx;
        if (rx->len)
        {
            msg = alloc_queue_msg(spec->recv);
            msg->len = rx->len;
            memcpy(msg->data, rx->buf + rx->offset, rx->len);

            k_queue_alloc_append(spec->recv->queue, msg);
        }
        break;

    case UART_RX_BUF_REQUEST:
        __ASSERT(spec->ovfw_assigned == 0, "UART_RX_BUF_REQUEST, ovfw_buf already assigned");
        LOG_DBG("UART_RX_BUF_REQUEST");

        struct uart_event_rx_buf *rx_buf = &evt->data.rx_buf;
        rx_buf->buf = spec->ovfw_buf;
        spec->ovfw_assigned = 1;
        break;

    case UART_RX_BUF_RELEASED:
        LOG_DBG("UART_RX_BUF_RELEASED");
        spec->ovfw_assigned = 0;
        break;

    case UART_RX_DISABLED:
        // todo: remove dup from xtty_init
        uart_rx_enable(dev, spec->recv_buf, spec->recv->msg_max_length, 10);
        break;

    case UART_TX_DONE:
    case UART_TX_ABORTED:
    case UART_RX_STOPPED:
        break;
    }
}

int xtty_init(tty_spec_t *xtty, const struct uart_config *spec)
{
    SURE(uart_configure, xtty->uart, spec);
    SURE(uart_callback_set, xtty->uart, xtty_irq, xtty);
    xtty->recv_buf = k_malloc(xtty->recv->msg_max_length);
    xtty->ovfw_buf = k_malloc(xtty->recv->msg_max_length);
    xtty->ovfw_assigned = 0;

    // todo: timeout to spec
    return uart_rx_enable(xtty->uart, xtty->recv_buf, xtty->recv->msg_max_length, 10);
}