#ifndef UNIFI_STATE_H
#define UNIFI_STATE_H

#include <Arduino.h>

// UniFi (UniFi Network API) polling configuration + most recent WAN status.
// The per-channel source mapping itself lives in UpsState::channelSource (the
// shared channel-routing array); this struct only holds the gateway connection
// and the derived WAN state.
struct UnifiState {
    // Configuration (persisted)
    String host;               // gateway IP/hostname (HTTPS), empty -> disabled
    String apiKey;             // X-API-KEY from Settings -> Control Plane / Integrations
    String site;               // controller site (usually "default")
    uint16_t pollIntervalSec;  // seconds between polls

    // Runtime status (not persisted)
    bool primaryDown;          // primary WAN down
    bool secondaryDown;        // secondary WAN down (dual-WAN only)
    bool failover;             // failover active (running on secondary)
    bool reachable;            // console answered the most recent poll cycle
    String wanStatus;          // raw wan subsystem status ("ok"/"warning"/"error")

    UnifiState()
        : host(""), apiKey(""), site("default"), pollIntervalSec(15),
          primaryDown(false), secondaryDown(false), failover(false),
          reachable(false), wanStatus("") {}
};

#endif // UNIFI_STATE_H
