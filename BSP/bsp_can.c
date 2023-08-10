#include "bsp_can.h"
#include "bsp_led.h"

extern CAN_HandleTypeDef hcan;
static CAN_FilterTypeDef filter;
static uint32_t          prescaler;
static can_bus_state_t   bus_state          = OFF_BUS;
static uint8_t           can_autoretransmit = ENABLE;
static can_txbuf_t       txqueue            = { 0 };

// Initialize CAN peripheral settings, but don't actually start the peripheral
void can_init(void)
{
    // Initialize default CAN filter configuration
    filter.FilterIdHigh         = 0;
    filter.FilterIdLow          = 0;
    filter.FilterMaskIdHigh     = 0;
    filter.FilterMaskIdLow      = 0;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterActivation     = ENABLE;

    // default to 125 kbit/s
    prescaler = 48;
    bus_state = OFF_BUS;

    HAL_NVIC_SetPriority(CEC_CAN_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CEC_CAN_IRQn);
}

// Start the CAN peripheral
void can_enable(void)
{
    if (bus_state == OFF_BUS) {
        hcan.Init.Prescaler = prescaler;
        hcan.Init.Mode      = CAN_MODE_NORMAL;

        hcan.Init.SyncJumpWidth        = CAN_SJW_1TQ;
        hcan.Init.TimeSeg1             = CAN_BS1_4TQ;
        hcan.Init.TimeSeg2             = CAN_BS2_3TQ;
        hcan.Init.TimeTriggeredMode    = DISABLE;
        hcan.Init.AutoBusOff           = ENABLE;
        hcan.Init.AutoWakeUp           = DISABLE;
        hcan.Init.AutoRetransmission   = can_autoretransmit;
        hcan.Init.ReceiveFifoLocked    = DISABLE;
        hcan.Init.TransmitFifoPriority = ENABLE;
        HAL_CAN_Init(&hcan);

        HAL_CAN_ConfigFilter(&hcan, &filter);

        HAL_CAN_Start(&hcan);
        bus_state = ON_BUS;

        led_blue_on();
    }
}

// Disable the CAN peripheral and go off-bus
void can_disable(void)
{
    if (bus_state == ON_BUS) {
        // Do a bxCAN reset (set RESET bit to 1)
        hcan.Instance->MCR |= CAN_MCR_RESET;
        bus_state = OFF_BUS;

        led_green_on();
    }
}

// Set the bitrate of the CAN peripheral
void can_set_bitrate(enum can_bitrate bitrate)
{
    if (bus_state == ON_BUS) {
        // cannot set bitrate while on bus
        return;
    }

    switch (bitrate) {
    case CAN_BITRATE_10K:
        prescaler = 600;
        break;
    case CAN_BITRATE_20K:
        prescaler = 300;
        break;
    case CAN_BITRATE_50K:
        prescaler = 120;
        break;
    case CAN_BITRATE_100K:
        prescaler = 60;
        break;
    case CAN_BITRATE_125K:
        prescaler = 48;
        break;
    case CAN_BITRATE_250K:
        prescaler = 24;
        break;
    case CAN_BITRATE_500K:
        prescaler = 12;
        break;
    case CAN_BITRATE_750K:
        prescaler = 8;
        break;
    case CAN_BITRATE_1000K:
        prescaler = 6;
        break;
    default:
        break;
    }

    led_green_on();
}

// Set CAN peripheral to silent mode
void can_set_silent(uint8_t silent)
{
    if (bus_state == ON_BUS) {
        // cannot set silent mode while on bus
        return;
    }
    if (silent) {
        hcan.Init.Mode = CAN_MODE_SILENT;
    } else {
        hcan.Init.Mode = CAN_MODE_NORMAL;
    }

    led_green_on();
}

// Enable/disable auto-retransmission
void can_set_autoretransmit(uint8_t autoretransmit)
{
    if (bus_state == ON_BUS) {
        // Cannot set autoretransmission while on bus
        return;
    }
    if (autoretransmit) {
        can_autoretransmit = ENABLE;
    } else {
        can_autoretransmit = DISABLE;
    }

    led_green_on();
}

// Send a message on the CAN bus
uint32_t can_tx(CAN_TxHeaderTypeDef* tx_msg_header, uint8_t* tx_msg_data)
{
    // Check if space available in the buffer (FIXME: wastes 1 item)
    if (((txqueue.head + 1) % TXQUEUE_LEN) == txqueue.tail) {
        error_assert(ERR_FULLBUF_CANTX);
        return HAL_ERROR;
    }

    // Copy header struct into array
    txqueue.header[txqueue.head] = *tx_msg_header;

    // Copy data into array
    for (uint8_t i = 0; i < tx_msg_header->DLC; i++) {
        txqueue.data[txqueue.head][i] = tx_msg_data[i];
    }

    // Increment the head pointer
    txqueue.head = (txqueue.head + 1) % TXQUEUE_LEN;

    return HAL_OK;
}

// Process messages in the TX output queue
void can_process(void)
{
    if ((txqueue.tail != txqueue.head)
        && (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) > 0)) {
        // Transmit can frame
        uint32_t mailbox_txed = 0;
        uint32_t status       = HAL_CAN_AddTxMessage(&hcan,
                                               &txqueue.header[txqueue.tail],
                                               txqueue.data[txqueue.tail],
                                               &mailbox_txed);

        txqueue.tail = (txqueue.tail + 1) % TXQUEUE_LEN;

        led_green_on();

        // This drops the packet if it fails (no retry). Failure is unlikely
        // since we check if there is a TX mailbox free.
        if (status != HAL_OK) {
            error_assert(ERR_CAN_TXFAIL);
        }
    }
}

// Receive message from the CAN bus RXFIFO
uint32_t can_rx(CAN_RxHeaderTypeDef* rx_msg_header, uint8_t* rx_msg_data)
{
    uint32_t status = HAL_CAN_GetRxMessage(
        &hcan, CAN_RX_FIFO0, rx_msg_header, rx_msg_data);

    led_blue_on();
    return status;
}

// Check if a CAN message has been received and is waiting in the FIFO
uint8_t is_can_msg_pending(uint8_t fifo)
{
    if (bus_state == OFF_BUS) {
        return 0;
    }
    return (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0);
}

// Return reference to CAN handle
CAN_HandleTypeDef* can_gethandle(void)
{
    return &hcan;
}

// Callback for FIFO0 full
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef* hcan)
{
    error_assert(ERR_CANRXFIFO_OVERFLOW);
}
