#ifndef PEMSY_MIOTY_CREDENTIALS_H
#define PEMSY_MIOTY_CREDENTIALS_H

/* COPY to ignored config/mioty_credentials.h and replace with the credentials
 * registered at your lab base station. This zero key is a placeholder and
 * cannot communicate with the course base station. Do not commit real keys. */
#define PEMSY_MIOTY_NETWORK_KEY \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
/* Group 1 identity from script tables 11.2/11.3; coordinate with the lab. */
#define PEMSY_MIOTY_EUI64 0x70, 0xb3, 0xd5, 0x67, 0x70, 0xff, 0x01, 0xa1
#define PEMSY_MIOTY_SHORT_ADDRESS 0x01, 0xa1
#define PEMSY_MIOTY_TX_DBM 10
#endif
