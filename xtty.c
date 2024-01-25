#include "xtty.h"
#include <xlog.h>

#include <zephyr/logging/log.h>
#include <string.h>

#ifdef XTTY_DEBUG
#define XTTY_LOG_LEVEL LOG_LEVEL_DBG
#else
#define XTTY_LOG_LEVEL LOG_LEVEL_INF
#endif

LOG_MODULE_REGISTER(xtty, XTTY_LOG_LEVEL);

void xtty_irq(const struct device *dev, struct uart_event *evt, void *data)
{
    tty_spec_t *spec = data;

    if (dev != spec->uart)
        return;

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

            xqueue_send_msg(spec->recv, msg);
        }
        break;

    case UART_RX_BUF_REQUEST:
        __ASSERT(spec->ovfw_assigned == 0, "UART_RX_BUF_REQUEST, ovfw_buf already assigned");
        //LOG_DBG("UART_RX_BUF_REQUEST");

        struct uart_event_rx_buf *rx_buf = &evt->data.rx_buf;
        rx_buf->buf = spec->ovfw_buf;
        spec->ovfw_assigned = 1;
        break;

    case UART_RX_BUF_RELEASED:
        //LOG_DBG("UART_RX_BUF_RELEASED");
        spec->ovfw_assigned = 0;
        break;

    case UART_RX_DISABLED:
        // todo: remove dup from xtty_init
        uart_rx_enable(dev, spec->recv_buf, spec->recv->msg_max_length, 10);
        break;

    case UART_TX_DONE:
        spec->state |= XTTY_TX_DONE;
        // LOG_DBG("UART_TX_DONE");
        spec->sent_count--;

        break;

    case UART_TX_ABORTED:
        spec->state |= XTTY_TX_ABORTED;
        spec->sent_count--;

        //LOG_DBG("UART_TX_ABORTED");
        break;

    case UART_RX_STOPPED:
        spec->state |= XTTY_TX_STPOPPED;
        spec->sent_count--;

        //LOG_DBG("UART_RX_STOPPED");
        break;

    default:
        //LOG_DBG("evt: %u", evt->type);
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
    xtty->state = 0;
    xtty->sent_count = 0;

    // todo: timeout to spec
    return uart_rx_enable(xtty->uart, xtty->recv_buf, xtty->recv->msg_max_length, 10);
}

void xtty_sender_thread(tty_spec_t *xtty, int32_t sleep_time_usec)
{
    LOG_DBG("uart sender started: uart: %p, sleep: %uus",
            xtty->uart,
            sleep_time_usec);

    queue_data_spec_t *queue = xtty->send;

    while (1)
    {
        while (!queue->len)
            k_sleep(K_USEC(sleep_time_usec));

        while (queue->len)
        {
            queue_msg_t *msg = xqueue_recv_msg(queue, K_FOREVER);

            if (msg == NULL)
            {
                LOG_ERR("add lock: %d", queue->len);
                break;
            }

            int err = uart_tx(xtty->uart, msg->data, msg->len, SYS_FOREVER_US);
            if (!err)
            {
                xtty->sent_count++;
                while (xtty->sent_count > 0)
                    k_sleep(K_USEC(10));
            }
            else
            {
                LOG_ERR("unable to send due err: %d, uart: %p", err, xtty->uart);
                k_msleep(100);
            }

            free_queue_msg(xtty->send, msg);
        }
    }
}
