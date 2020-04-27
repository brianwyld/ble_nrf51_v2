/* ble_comm.c : handle the BLE UART emulation used for the communication with remote BLE ie AT commands
 */
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

//#include "ble_nus_c.h"
#include "ble_nus.h"
#include "ble_hci.h"

#include "main.h"
#include "bsp_minew_nrf51.h"

#define COMM_UART_NB    (0)
#define COMM_UART_BAUDRATE  (115200)
#define MAX_RX_LINE (100)

static struct {
    bool connected;
    ble_nus_t               m_nus;                /**< Structure to identify the Nordic UART Service (as a server)*/
//    ble_nus_c_t             m_nus_c;               /**< Instance of NUS client service (as client). */
    uint8_t rx_buf[MAX_RX_LINE];
    uint8_t rx_index;
    UART_TX_READY_FN_T tx_ready_fn;     // in case caller wants to be told
} _ctx;

// predecs
static void comm_ble_nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)

/**@brief Function for initializing the BLE as a NUS slave (remote connects to me) and a NUS cient.
 * NUS client not yet supported
 */
bool comm_ble_init(void) {
    uint32_t       err_code;
    ble_nus_init_t nus_init;
 
    memset(&nus_init, 0, sizeof(nus_init));
 
    nus_init.data_handler = comm_ble_nus_data_handler;
 
    err_code = ble_nus_init(&_ctx.m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);

/* NUS client functionality not yet supported - might have to be another block?
    ble_nus_c_init_t nus_c_init;
    memset(&nus_c_init, 0, sizeof(nus_c_init));
    err_code = ble_nus_c_init(&_ctx.m_nus_c, &nus_c_init);
    APP_ERROR_CHECK(err_code);
    */
    return true;
}

void comm_ble_connect_client() {
    _ctx.connected = true;
    _ctx.rx_index = 0;      // just in case
}
void comm_ble_disconnect() {
/*
    if (p_evt!=NULL) {
        ble_nus_c_on_db_disc_evt(&_ctx.m_nus_c, p_evt);
    }
    */
    // Seems a little brutual to break its connection but there you go?
    if (_ctx.m_nus.conn_handle!=BLE_CONN_HANDLE_INVALID) {
        uint32_t err_code = sd_ble_gap_disconnect(_ctx.m_nus.conn_handle,
                                     BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
   }
    _ctx.m_nus.conn_handle=BLE_CONN_HANDLE_INVALID;

    // Tell at cmd processor by sending disc at command
    at_process_input("AT+DISC", &comm_ble_tx);
    _ctx.connected = false;
    _ctx.tx_ready_fn = NULL;
    _ctx.rx_index = 0;
}

int comm_ble_tx(uint8_t* data, int len, UART_TX_READY_FN_T tx_ready) {
    if (!_ctx.connected) {
        return -1;      // not connected, sorry
    }
    _ctx.tx_ready_fn = tx_ready;        // in case of..
    // check if disconnecting and ignore (as can't disconnect from uart...)
    if (data!=NULL)  {
        // Send to NUS : TODO do we need to cut it up into BLE_NUS_MAX_DATA_LEN sized blocks?
        uint32_t err_code = ble_nus_string_send(&_ctx.m_nus, data, len);
//        err_code = ble_nus_c_string_send(&_ctx.m_nus_c, data, len);
        if (err_code == NRF_ERROR_INVALID_STATE) {
            _ctx.connected = false;
        } else {
            APP_ERROR_CHECK(err_code);
        }
    } else {
        // disconnnect NUS
        comm_ble_disconnect(NULL);
    }
}

void comm_ble_dispatch(ble_evt_t * p_ble_evt) {
    ble_nus_on_ble_evt(&_ctx.m_nus, p_ble_evt);
//    ble_nus_c_on_ble_evt(&_ctx.m_nus_c,p_ble_evt);
}
/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the at processor.
 *
 * @param[in] p_nus    Nordic UART Service structure.
 * @param[in] p_data   Data to be send to UART module.
 * @param[in] length   Length of the data.
 
 */
/**@snippet [Handling the data received over BLE] */
static void comm_ble_nus_data_handler(ble_nus_t * p_nus, uint8_t* p_data, uint16_t length)
{
    for(int i=0;i<length;i++) {
        _ctx.rx_buf[_ctx.rx_index] = p_data[i];
        if( (_ctx.rx_buf[_ctx.rx_index] == '\r') || (_ctx.rx_buf[_ctx.rx_index] == '\n') || (_ctx.rx_index >= (MAX_RX_LINE-1)) )
        {
            _ctx.rx_buf[_ctx.rx_index] = 0; // null terminate the data in buffer
            // And process
            at_process_input(_ctx.rx_buf, &comm_ble_tx);
            // reset our line buffer
            _ctx.rx_index = 0;
            // Continue for rest of data in input
        } else {
            _ctx.rx_index++;
        }
    }
}

