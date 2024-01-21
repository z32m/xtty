#ifndef __xtty_h__
#define __xtty_h__

#include <xqueue.h>
#include <zephyr/drivers/uart.h>

typedef struct
{
    const struct device *uart;
    queue_data_spec_t *recv;
    queue_data_spec_t *send;
    uint8_t *recv_buf;
    uint8_t *ovfw_buf;
    uint8_t ovfw_assigned;
} tty_spec_t;

int xtty_init(tty_spec_t *xtty, const struct uart_config *spec);

#define DEFINE_XTTY_DATA(_name, _msg_size, _msgs_limit)        \
    DEFINE_QUEUE_DATA(_name##_recv_q, _msg_size, _msgs_limit); \
    DEFINE_QUEUE_DATA(_name##_send_q, _msg_size, _msgs_limit)

#define DEFINE_XTTY(_queue) tty_spec_t _queue;

#define PREPARE_XTTY(_queue, _tty_dev)                                       \
    queue_data_spec_t _queue##_recv_spec = INIT_QUEUE_SPEC(_queue##_recv_q); \
    queue_data_spec_t _queue##_send_spec = INIT_QUEUE_SPEC(_queue##_send_q); \
    _queue.uart = _tty_dev;                                                  \
    _queue.recv = &_queue##_recv_spec;                                       \
    _queue.send = &_queue##_send_spec;

#endif