# ESP32-P4-ETH Relay Bridge

A versatile Ethernet-based relay bridge using the Waveshare ESP32-P4-ETH board that can operate as either a dry contact sender or relay receiver, enabling remote control of electrical circuits over Ethernet networks.

## Features

- **Dual Operation Modes**:
  - **Dry Contact Sender**: Monitors dry contact inputs and transmits state changes over Ethernet
  - **Relay Receiver**: Receives commands over Ethernet and controls relay outputs

- **Hardware Interface**:
  - **OLED Display**: 128x64 SSD1306 display showing device status, IP addresses, and I/O states
  - **Mode Selection Jumper**: Hardware jumper on GPIO16 for mode selection (overrides software settings)
  - **Real-time Status**: Visual feedback for input/output states and network connectivity

- **Network Connectivity**:
  - Wired Ethernet via LAN8720 PHY
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
| Mode Jumper | GPIO16 | Mode selection (LOW=Sender, HIGH=Receiver) |
| Dry Contact Input | GPIO4 | Digital input with internal pull-up |
| Relay Output | GPIO2 | Digital output for relay control |
| Status LED | GPIO5 | Onboard status indicator |
| OLED SDA | GPIO21 | I2C data line for OLED display |
| OLED SCL | GPIO22 | I2C clock line for OLED display |
| Ethernet MDC | GPIO23 | Ethernet PHY management clock |
| Ethernet MDIO | GPIO18 | Ethernet PHY management data |

## Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/your-username/eth-relay-bridge.git
   cd eth-relay-bridge
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
- Monitors GPIO4 for dry contact state changes
- Sends state changes to configured receiver IP address
- Supports both momentary and sustained contact detection
- Configurable debounce timing (50ms default)

#### Relay Receiver Mode
- Listens for commands over UDP/TCP
- Controls GPIO2 relay output
- Supports momentary relay pulses (500ms default)
- Configurable active-high/active-low relay logic

### Network Configuration

- **DHCP**: Automatic IP address assignment via DHCP
- **Protocol**: Choose between UDP (fast, lightweight) or TCP (reliable)
- **Ports**: UDP 8888, TCP 8889 (configurable)
- **Discovery**: UDP broadcast on port 8900 for device discovery
- **Heartbeat**: Periodic status broadcasts every 30 seconds

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
1. Install jumper pin on GPIO16 header
2. Connect jumper to GND for Sender mode
3. Leave jumper open for Receiver mode
4. Power cycle device to apply mode change

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
```json
{
  "device_id": "ETH-12345678",
  "mode": 1,
  "status": "online",
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
