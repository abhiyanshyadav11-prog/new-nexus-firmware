
#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

//#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

// GATT Service UUID
#define NEXUS_SERVICE_UUID   0xABCD

// Characteristic UUIDs
#define AUDIO_DATA_CHAR_UUID 0x1234
#define COMMAND_RX_CHAR_UUID 0x5678
#define COMMAND_TX_CHAR_UUID 0x9012
#define STATUS_CHAR_UUID     0x3456
#define BATTERY_CHAR_UUID    0x7890

// BLE Packet Format (simplified for header)
typedef struct {
    uint8_t  magic[2];
    uint8_t  version;
    uint8_t  type;
    uint16_t length;
    uint8_t  payload[250];
    uint16_t crc;
} nx_packet_t;

typedef struct {
    uint16_t seq;
    uint8_t  flags;
    uint8_t  payload[];
} nx_audio_packet_t;

void ble_manager_init(void);
void ble_task(void *pvParameters);
void ble_send_command_tx(nx_packet_t *packet);
void ble_send_status_update(uint8_t *status_data, uint16_t len);
void ble_send_battery_level(uint8_t battery_level);

#endif // BLE_MANAGER_H
