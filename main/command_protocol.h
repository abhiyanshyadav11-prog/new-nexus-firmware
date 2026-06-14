
#ifndef COMMAND_PROTOCOL_H
#define COMMAND_PROTOCOL_H

#include <stdint.h>

// Magic bytes for Nexus packets
#define NEXUS_MAGIC_BYTE_1 0x4E // 'N'
#define NEXUS_MAGIC_BYTE_2 0x58 // 'X'

// Protocol Version
#define NEXUS_PROTOCOL_VERSION 0x01

// Command Types (phone -> pendant)
typedef enum {
    CMD_LED_PATTERN = 0x01,
    CMD_VIBRATE_PATTERN = 0x02,
    CMD_DISPLAY_TEXT = 0x03, // Optional OLED display
    CMD_ACK = 0x04,          // Acknowledge command
    CMD_NACK = 0x05,         // Negative Acknowledge command
    CMD_REQUEST_STATUS = 0x06,
    // Add more commands as needed
} nexus_command_type_t;

// Command Packet Structure (phone -> pendant)
typedef struct __attribute__((packed)) {
    uint8_t  magic[2];     // 0xNX (Nexus magic bytes)
    uint8_t  version;      // Protocol version (0x01)
    uint8_t  type;         // nexus_command_type_t
    uint16_t length;       // Payload length
    uint8_t  payload[250]; // Command-specific data
    uint16_t crc;          // CRC-16 (simple checksum for now)
} nx_packet_t;

// Audio Packet Structure (pendant -> phone, MTU fragmented)
typedef struct __attribute__((packed)) {
    uint16_t seq;          // Sequence number
    uint8_t  flags;        // 0x01=first, 0x02=last, 0x04=VAD_active
    uint8_t  payload[];    // PCM 16kHz mono 16-bit, compressed
} nx_audio_packet_t;

// Status Packet Structure (pendant -> phone)
typedef struct __attribute__((packed)) {
    uint8_t  battery_level; // 0-100%
    uint8_t  ble_connected; // 0=disconnected, 1=connected
    uint8_t  current_led_pattern; // led_pattern_t
    // Add more status fields as needed
} nx_status_packet_t;

#endif // COMMAND_PROTOCOL_H
