/* uart_comm.c : handle the UART used for the communication with host ie AT commands
 */
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "app_uart.h"

#include "main.h"
#include "bsp_minew_nrf51.h"

#define COMM_UART_NB    (0)
#define COMM_UART_BAUDRATE  (115200)
#define MAX_RX_LINE (100)

static struct {
    int uartNb;
    uint8_t rx_buf[MAX_RX_LINE+2];      // wriggle space for the \0
    uint8_t rx_index;
    UART_TX_READY_FN_T tx_ready_fn;     // in case caller wants to be told
} _ctx;

// predecs
static void uart_event_handler(app_uart_evt_t * p_event);

/**@brief Function for initializing the UART.
 */
bool comm_uart_init(void) {
    _ctx.uartNb = COMM_UART_NB;
    return hal_bsp_uart_init(_ctx.uartNb, COMM_UART_BAUDRATE, uart_event_handler);
}

void comm_uart_deinit(void) {
    hal_bsp_uart_deinit(_ctx.uartNb);
}
int comm_uart_tx(uint8_t* data, int len, UART_TX_READY_FN_T tx_ready) {
    _ctx.tx_ready_fn = tx_ready;        // in case of..
    // check if disconnecting and ignore (as can't disconnect from uart...)
    if (data!=NULL)  {
        return (hal_bsp_uart_tx(_ctx.uartNb, data, len));
    }
}

/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' i.e '\r\n' (hex 0x0D) or if the string has reached a length of
 *          @ref NUS_MAX_DATA_LENGTH.
 */
static void uart_event_handler(app_uart_evt_t * p_event)
{
    uint32_t err_code;
    
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
        {
            // Read all the bytes we can 
            while(app_uart_get(&_ctx.rx_buf[_ctx.rx_index])==NRF_SUCCESS) 
            {
                // End of line? or buiffer full?
                if( (_ctx.rx_buf[_ctx.rx_index] == '\r') || (_ctx.rx_buf[_ctx.rx_index] == '\n') || (_ctx.rx_index >= MAX_RX_LINE) )
                {
                    _ctx.rx_buf[_ctx.rx_index+1] = 0; // null terminate the data in buffer (not overwriting the \r or \n though)
                    // And process
                    at_process_input(_ctx.rx_buf, &comm_uart_tx);
                    // reset our line buffer to start
                    _ctx.rx_index = 0;
                    // Continue for rest of data in input
                } else {
                    _ctx.rx_index++;
                }
            }
        }
        break;
 
        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
        break;
 
        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
        break;
        
        case APP_UART_TX_EMPTY:
            // if tx had been full, can tell the user they can restart...
            if (_ctx.tx_ready_fn!=NULL) {
                (*_ctx.tx_ready_fn)(&comm_uart_tx);
            }
            break;
 
        default:
        break;
    }
}

/*
                    err_code = ble_nus_string_send(&m_nus, data_array, index);
                    if (err_code != NRF_ERROR_INVALID_STATE) {
                        APP_ERROR_CHECK(err_code);
                    }
*/