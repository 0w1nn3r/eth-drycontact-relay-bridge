#ifndef UPS_STATE_H
#define UPS_STATE_H

#include <Arduino.h>

// Per-channel input source for sender mode. A channel can be driven by its
// physical dry-contact input (default) or by a state polled from a network UPS.
enum ChannelSource {
    SRC_GPIO = 0,             // Physical dry-contact GPIO input (default)
    SRC_UPS_ON_BATTERY = 1,   // UPS upsOutputSource == battery(5)
    SRC_UPS_BATTERY_LOW = 2,  // UPS upsBatteryStatus == batteryLow(3)/depleted(4)
    SRC_DISABLED = 3          // Channel ignored, never sends commands
};

// UPS polling configuration + most recent polled status (sender mode only).
struct UpsState {
    // Configuration (persisted)
    String ip;                 // empty -> UPS polling disabled
    String community;          // SNMP v1/v2c community string
    uint16_t port;             // SNMP UDP port (161)
    uint16_t pollIntervalSec;  // seconds between polls
    int channelSource[2];      // ChannelSource for CH1 / CH2

    // Runtime status (not persisted)
    bool onBattery;            // last polled "running on battery"
    bool batteryLow;           // last polled "battery low"
    bool reachable;            // UPS answered the most recent poll cycle

    UpsState()
        : ip(""), community("public"), port(161), pollIntervalSec(10),
          onBattery(false), batteryLow(false), reachable(false) {
        channelSource[0] = SRC_GPIO;
        channelSource[1] = SRC_GPIO;
    }

    bool usesUps() const {
        for (int i = 0; i < 2; i++) {
            if (channelSource[i] == SRC_UPS_ON_BATTERY ||
                channelSource[i] == SRC_UPS_BATTERY_LOW) {
                return true;
            }
        }
        return false;
    }
};

#endif // UPS_STATE_H
