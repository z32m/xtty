#ifndef __xtty_h__
#define __xtty_h__

#include <xqueue.h>

typedef struct
{
    const struct device *uart;
    queue_data_spec_t *recv;
    queue_data_spec_t *send;
} tty_spec_t;

void xtty_irq(const struct device *dev, void *user_data);

#define DEFINE_XTTY_DATA(_name, _msg_size, _msgs_limit)        \
    DEFINE_QUEUE_DATA(_name##_recv_q, _msg_size, _msgs_limit); \
    DEFINE_QUEUE_DATA(_name##_send_q, _msg_size, _msgs_limit)

#define DEFINE_XTTY(_queue, _tty_dev)                                              \
    const queue_data_spec_t _queue##_recv_spec = INIT_QUEUE_SPEC(_queue##_recv_q); \
    const queue_data_spec_t _queue##_send_spec = INIT_QUEUE_SPEC(_queue##_send_q); \
    tty_spec_t _queue = {                                                    \
        .uart = _tty_dev,                                                          \
        .recv = &_queue##_recv_spec,                                               \
        .send = &_queue##_send_spec};                                              \
    uart_irq_callback_user_data_set(_tty_dev, xtty_irq, &_queue);                  \
    uart_irq_rx_enable(_tty_dev);                                                  \
    uart_irq_tx_enable(_tty_dev)

#endif