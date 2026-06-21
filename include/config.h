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

// Mode selection jumper
#define MODE_JUMPER_PIN 16  // GPIO pin for mode selection jumper (LOW = Sender, HIGH = Receiver)

// Dry contact sender configuration (dual channel)
#define DRY_CONTACT_PIN_1 4   // GPIO pin for dry contact input channel 1
#define DRY_CONTACT_PIN_2 12  // GPIO pin for dry contact input channel 2
#define DRY_CONTACT_DEBOUNCE_MS 50
#define DRY_CONTACT_LONG_PRESS_MS 1000

// Relay receiver configuration (dual channel)
#define RELAY_PIN_1 2   // GPIO pin for relay output channel 1
#define RELAY_PIN_2 15  // GPIO pin for relay output channel 2
#define RELAY_ACTIVE_LOW true  // Set to true if relay is active low
#define RELAY_PULSE_MS 500  // Pulse duration for momentary relay activation

// State change logging configuration
#define MAX_LOG_ENTRIES 100
#define LOG_ENTRY_SIZE 128
#define LOG_FLASH_SECTOR 0  // Flash sector for log storage
#define LOG_SAVE_INTERVAL_MS 30000  // Save log to flash every 30 seconds

// OLED Display configuration (128x64 SSD1306)
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1  // Reset pin (-1 if sharing Arduino reset pin)
#define OLED_SDA_PIN 21  // I2C SDA pin
#define OLED_SCL_PIN 22  // I2C SCL pin
#define OLED_ADDRESS 0x3C  // I2C address (0x3C for most 128x64 displays)

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
