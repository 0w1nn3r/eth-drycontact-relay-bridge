#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Board configuration for Waveshare ESP32-P4-ETH
#define BOARD_NAME "ESP32-P4-ETH Relay Bridge"
#define FIRMWARE_VERSION "1.0.0"

// Ethernet configuration
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_ADDR 0
#define ETH_PHY_MDC_PIN 23
#define ETH_PHY_MDIO_PIN 18
#define ETH_PHY_POWER_PIN -1
#define ETH_CLK_MODE ETH_CLK_GPIO0_IN

// Network configuration
#define DEFAULT_HOSTNAME "eth-relay-bridge"
#define MDNS_NAME "eth-relay-bridge"
#define WEB_PORT 80

// Operation modes
enum OperationMode {
    MODE_UNDEFINED = 0,
    MODE_DRY_CONTACT_SENDER = 1,
    MODE_RELAY_RECEIVER = 2
};

// Dry contact sender configuration
#define DRY_CONTACT_PIN 4  // GPIO pin for dry contact input
#define DRY_CONTACT_DEBOUNCE_MS 50
#define DRY_CONTACT_LONG_PRESS_MS 1000

// Relay receiver configuration
#define RELAY_PIN 2  // GPIO pin for relay output
#define RELAY_ACTIVE_LOW true  // Set to true if relay is active low
#define RELAY_PULSE_MS 500  // Pulse duration for momentary relay activation

// Communication protocol
#define PROTOCOL_VERSION 1
#define UDP_PORT 8888
#define TCP_PORT 8889
#define HEARTBEAT_INTERVAL_MS 30000
#define DEVICE_DISCOVERY_PORT 8900

// Web interface
#define WEB_UPDATE_INTERVAL_MS 2000
#define MAX_CLIENTS 4
#define CONFIG_SSID "ETH-Relay-Bridge-Setup"

// Debug and logging
#define SERIAL_BAUD_RATE 115200
#define DEBUG_LEVEL 3  // 0=No debug, 1=Error, 2=Warning, 3=Info, 4=Debug

// EEPROM configuration
#define EEPROM_SIZE 512
#define EEPROM_MODE_ADDR 0
#define EEPROM_CONFIG_ADDR 10

// Status LED
#define STATUS_LED_PIN 5  // Onboard status LED
#define LED_BLINK_NORMAL_MS 1000
#define LED_BLINK_ERROR_MS 200
#define LED_BLINK_CONNECTING_MS 500

#endif // CONFIG_H
