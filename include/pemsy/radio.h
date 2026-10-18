#ifndef PEMSY_RADIO_H
#define PEMSY_RADIO_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/types.h"

#ifndef PEMSY_NODE_ADDRESS
#define PEMSY_NODE_ADDRESS 1
#endif
#ifndef PEMSY_RADIO_FREQUENCY_HZ
#define PEMSY_RADIO_FREQUENCY_HZ 869800000u
#endif
#ifndef PEMSY_RFM69_HIGH_POWER
#define PEMSY_RFM69_HIGH_POWER 0
#endif

#define RADIO_MAX_TEXT 16
#define RADIO_BROADCAST 0xff
#define RADIO_IRQ_PIN 20

typedef struct {
    uint8_t sender;
    bool is_broadcast;
    char text[RADIO_MAX_TEXT + 1];
    int16_t rssi_dbm;
} radio_packet_t;

#ifdef __cplusplus
extern "C" {
#endif
bool radio_init(void);
/* Starts TX; call poll until take_tx_result reports completion. */
bool radio_send(uint8_t recipient, const char *message);
void radio_poll(void);
bool radio_receive(radio_packet_t *packet);
bool radio_busy(void);
bool radio_is_online(void);
bool radio_take_tx_result(bool *success);
/* Called by the application's shared GPIO callback. No SPI in this handler. */
void radio_handle_irq(uint gpio, uint32_t events);
/* Exclusive ownership handoff to mioty; call radio_init after mioty completes. */
bool radio_suspend(void);
bool radio_test_spi(void);
#ifdef __cplusplus
}
#endif
#endif
