#ifndef BSP_CAN_
#define BSP_CAN_

#include "error.h"
#include "main.h"

enum can_bitrate {
    CAN_BITRATE_10K   = 0,
    CAN_BITRATE_20K   = 1,
    CAN_BITRATE_50K   = 2,
    CAN_BITRATE_100K  = 3,
    CAN_BITRATE_125K  = 4,
    CAN_BITRATE_250K  = 5,
    CAN_BITRATE_500K  = 6,
    CAN_BITRATE_750K  = 7,
    CAN_BITRATE_1000K = 8,
    CAN_BITRATE_INVALID,
};

typedef enum can_bus_state {
    OFF_BUS = 0,
    ON_BUS  = 1,
} can_bus_state_t;

// CAN transmit buffering
#define TXQUEUE_LEN     28 // Number of buffers allocated
#define TXQUEUE_DATALEN 8  // CAN DLC length of data buffers

typedef struct cantxbuf_ {
    uint8_t             data[TXQUEUE_LEN][TXQUEUE_DATALEN]; // Data buffer
    CAN_TxHeaderTypeDef header[TXQUEUE_LEN];                // Header buffer
    uint8_t             head;                               // Head pointer
    uint8_t             tail;                               // Tail pointer
    uint8_t             full;                               // TODO: Set this when we are full, clear when the tail moves one.
} can_txbuf_t;

// Prototypes
void     can_init(void);
void     can_enable(void);
void     can_disable(void);
void     can_set_bitrate(enum can_bitrate bitrate);
void     can_set_silent(uint8_t silent);
void     can_set_autoretransmit(uint8_t autoretransmit);
uint32_t can_tx(CAN_TxHeaderTypeDef* tx_msg_header, uint8_t* tx_msg_data);
uint32_t can_rx(CAN_RxHeaderTypeDef* rx_msg_header, uint8_t* rx_msg_data);

void can_process(void);

uint8_t            is_can_msg_pending(uint8_t fifo);
CAN_HandleTypeDef* can_gethandle(void);

#endif
