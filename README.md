# ESP32-P4-ETH Relay Bridge

A versatile Ethernet-based relay bridge using the Waveshare ESP32-P4-ETH board that can operate as either a dry contact sender or relay receiver, enabling remote control of electrical circuits over Ethernet networks.

## Features

- **Dual Operation Modes**:
  - **Dry Contact Sender**: Monitors dry contact inputs and transmits state changes over Ethernet
  - **Relay Receiver**: Receives commands over Ethernet and controls relay outputs

- **Hardware Interface**:
  - **OLED Display**: 128x64 SSD1306 display showing device status, IP addresses, and I/O states
  - **Mode Selection Jumper**: Hardware jumper on GPIO10 for mode selection (overrides software settings)
  - **Real-time Status**: Visual feedback for input/output states and network connectivity
  - **Dual Channel Support**: Two independent dry contact inputs and two relay outputs
  - **State Change Logging**: Comprehensive logging of all state changes with timestamps

- **Network Connectivity**:
  - Wired Ethernet via IP101GRI PHY
  - mDNS for device discovery
  - UDP and TCP communication protocols
  - Web-based configuration interface

- **Hardware Support**:
  - Waveshare ESP32-P4-ETH board
  - Configurable GPIO pins for dry contact input and relay output
  - Status LED for operational feedback
  - EEPROM for persistent configuration storage

## Hardware Requirements

### Required Components
- Waveshare ESP32-P4-ETH development board
- Ethernet cable and network connection
- Optional: External relay module (for receiver mode)
- Optional: Dry contact source (switch, sensor, etc.)

### Pin Configuration

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| Mode Jumper | GPIO10 | Mode selection (LOW=Sender, HIGH=Receiver) |
| Factory Reset Button | GPIO0 | BOOT button for factory reset (3-second hold) |
| Dry Contact Input 1 | GPIO14 | Digital input with internal pull-up (Channel 1) |
| Dry Contact Input 2 | GPIO13 | Digital input with internal pull-up (Channel 2) |
| Relay Output 1 | GPIO3 | Digital output for relay control (Channel 1) |
| Relay Output 2 | GPIO4 | Digital output for relay control (Channel 2) |
| Status LED | GPIO5 | Onboard status indicator |
| OLED SDA | GPIO7 | I2C data line for OLED display (ESP32-P4-ETH default) |
| OLED SCL | GPIO8 | I2C clock line for OLED display (ESP32-P4-ETH default) |
| Ethernet MDC | GPIO31 | IP101GRI PHY management clock |
| Ethernet MDIO | GPIO52 | IP101GRI PHY management data |

## Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/0w1nn3r/eth-drycontact-relay-bridge.git
   cd eth-drycontact-relay-bridge
   ```

2. **Open in PlatformIO**:
   - Use VS Code with PlatformIO extension or PlatformIO IDE
   - Open the project directory

3. **Build and Upload**:
   ```bash
   pio run --target upload
   ```

4. **Monitor Serial Output**:
   ```bash
   pio device monitor
   ```

## Configuration

### Web Interface
1. Connect the device to your network via Ethernet
2. Open a web browser and navigate to the device's IP address
3. Configure the operation mode and network settings

### Operation Modes

#### Dry Contact Sender Mode
- Monitors GPIO4 and GPIO12 for dry contact state changes (dual channel)
- Sends state changes to configured receiver IP address
- Supports both momentary and sustained contact detection
- Configurable debounce timing (50ms default)
- Independent channel operation with separate logging

#### Relay Receiver Mode
- Listens for commands over UDP/TCP
- Controls GPIO2 and GPIO15 relay outputs (dual channel)
- Supports momentary relay pulses (500ms default)
- Configurable active-high/active-low relay logic
- Independent control of both relay channels from a single paired sender

### Network Configuration

- **DHCP**: Automatic IP address assignment via DHCP
- **Protocol**: Choose between UDP (fast, lightweight) or TCP (reliable)
- **Ports**: UDP 8888, TCP 8889 (configurable)
- **Discovery**: UDP broadcast on port 8900 for device discovery
- **Heartbeat**: Each device broadcasts a status heartbeat every 30 seconds on port 8900
- **Peer Liveness**: A paired device tracks its peer's heartbeats and reports it offline after 90 seconds (3 missed beats) on the web UI and OLED

#### DHCP Information
The device automatically obtains network configuration via DHCP:
- **IP Address**: Assigned automatically from DHCP server
- **Subnet Mask**: Configured automatically
- **Gateway**: Default gateway assigned automatically  
- **DNS Server**: DNS server assigned automatically
- **Fallback**: Continues without network if DHCP fails

The web interface displays all DHCP-assigned network information for troubleshooting and configuration verification.

### OLED Display

The 128x64 SSD1306 OLED display provides real-time status information:

**Display Layout:**
- **Header**: Mode icon and mode name (S=Sender, R=Receiver)
- **Input/Output State**: 
  - Sender mode: "INPUT: OPEN/CLOSED"
  - Receiver mode: "RELAY: ON/OFF"
- **Network Information**:
  - "Local: xxx.xxx.xxx.xxx" - Device IP address
  - "Remote: xxx.xxx.xxx.xxx" - Receiver IP (sender mode only)
  - "Sender/Recv: ONLINE/OFFLINE" - Paired peer liveness (from heartbeats)
- **Status**: "Online/Offline" - Ethernet connection status
- **Status Bar**: Device ID and "ETH" indicator

**I2C Connections:**
- SDA: GPIO21
- SCL: GPIO22
- Address: 0x3C (default for most 128x64 displays)

### Mode Selection Jumper

A hardware jumper on GPIO16 provides reliable mode selection:

**Jumper Configuration:**
- **Jumper to GND (LOW)**: Dry Contact Sender mode
- **Jumper Open (HIGH)**: Relay Receiver mode
- **Internal Pull-up**: Default state is HIGH (Receiver mode)

**Priority:**
- Hardware jumper overrides EEPROM/software settings
- Mode is detected at startup and logged to serial
- Web interface shows "(Jumper Set)" when hardware mode is active

**Usage:**
1. Install jumper pin on GPIO10 header
2. Connect jumper to GND for Sender mode
3. Leave jumper open for Receiver mode
4. Power cycle device to apply mode change

### Factory Reset

The device includes a factory reset feature to clear all saved settings and restore defaults:

#### Factory Reset Method

**Boot-Time Factory Reset**
1. Hold the BOOT button (GPIO0) while powering on the device
2. Continue holding for 2 seconds during boot
3. Release the button after 2 seconds
4. Device will perform factory reset and reboot

#### Runtime Reboot

**Soft Reboot (No Data Loss)**
1. Press and hold the BOOT button (GPIO0) for 3 seconds while device is running
2. Release the button after 3 seconds
3. Device will reboot without clearing any data

#### What Gets Cleared
- All configuration settings (mode, network settings, etc.)
- Pairing information and device associations
- Custom channel names
- State change logs
- All preferences and saved data

#### Visual Indicators
- **OLED Display**: Shows "FACTORY RESET" message during reset
- **Status LED**: Blinks 5 times to indicate factory reset in progress
- **Serial Output**: Detailed reset messages for debugging

#### After Factory Reset
- Device reboots with default settings
- Channel names revert to "Channel1" and "Channel2"
- All pairing information is cleared
- Device must be reconfigured and re-paired

### Enhanced Web Interface

The device features a comprehensive web interface with advanced capabilities:

#### Device-to-Device Pairing
- **Single Paired Sender**: A receiver pairs with one sender, which drives both relay channels
- **Sender Discovery**: Scan the network for available sender devices
- **Visual Pairing Management**: Intuitive interface for pairing and unpairing the device
- **Real-time Status**: Live updates of pairing status and connection state

#### State Change Logging
- **Comprehensive Logging**: All state changes are logged with timestamps
- **Log Viewer**: Web-based log viewer with filtering and search capabilities
- **Event Types**: Dry contact, relay, pairing, and system events
- **Persistent Storage**: Logs are saved to flash memory for durability

#### Dual Channel Display
- **Channel Status**: Real-time display of both channel states
- **Independent Operation**: Each channel operates independently
- **Visual Feedback**: Clear indication of channel states and pairing status

## Usage Examples

### Basic Setup - Sender and Receiver Pair

1. **Configure Receiver**:
   - Set mode to "Relay Receiver"
   - Note the receiver's IP address
   - Connect relay to GPIO2

2. **Configure Sender**:
   - Set mode to "Dry Contact Sender"
   - Enter receiver's IP address
   - Connect dry contact switch to GPIO4

3. **Test Operation**:
   - Activate the dry contact on the sender
   - Verify the relay activates on the receiver
   - Monitor status via web interface

### Advanced Configuration

#### Multiple Receivers
- Configure multiple sender devices to communicate with one receiver
- Use different dry contact inputs for different functions

#### Network Integration
- Integrate with home automation systems
- Use MQTT bridges for additional connectivity
- Implement custom protocols via TCP endpoints

## API Reference

### REST API Endpoints

- `GET /` - Web interface
- `GET /api` - Device status and configuration
- `GET/POST /config` - Configuration management
- `GET/POST /mode` - Operation mode management
- `GET /status` - Basic device status
- `GET /discovery` - Device discovery information

### Communication Protocol

#### Message Format (JSON)
```json
{
  "device_id": "ETH-12345678",
  "command": "relay_on|relay_off",
  "timestamp": 1234567890,
  "protocol_version": 1
}
```

#### Heartbeat Message
Broadcast on port 8900 every 30 seconds. The paired peer uses it to track liveness.
```json
{
  "type": "heartbeat",
  "device_id": "ETH-12345678",
  "mode": 1,
  "status": "online",
  "ip_address": "192.168.1.50",
  "timestamp": 1234567890,
  "protocol_version": 1
}
```

## Troubleshooting

### Common Issues

1. **Ethernet Not Connecting**:
   - Verify cable connection
   - Check network configuration
   - Monitor serial output for error messages

2. **Relay Not Responding**:
   - Check relay wiring and power supply
   - Verify GPIO configuration
   - Test with manual activation

3. **Communication Failures**:
   - Verify IP address configuration
   - Check firewall settings
   - Test with both UDP and TCP protocols

### Debug Information

- Enable serial monitor for real-time debugging
- Check web interface status indicators
- Monitor network traffic with packet capture tools
- Review device logs for error messages

## Development

### Project Structure
```
eth-relay-bridge/
├── src/
│   └── main.cpp          # Main application code
├── include/
│   └── config.h          # Configuration constants
├── lib/                  # Additional libraries
├── platformio.ini        # PlatformIO configuration
└── README.md            # This file
```

### Adding Features
- Implement additional communication protocols
- Add sensor inputs for monitoring
- Create mobile app interface
- Add encryption for secure communication

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For support and questions:
- Create an issue on GitHub
- Check the troubleshooting section
- Review the API documentation
