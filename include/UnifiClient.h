#ifndef UNIFI_CLIENT_H
#define UNIFI_CLIENT_H

#include <Arduino.h>

// Derived WAN state from a UniFi Network "stat/health" response.
struct UnifiHealth {
    bool primaryDown;
    bool secondaryDown;
    bool failover;
    String wanStatus;   // raw wan subsystem status, for display/diagnostics
};

// Fetch /proxy/network/api/s/{site}/stat/health from the UniFi gateway over
// HTTPS (self-signed cert accepted) using X-API-KEY auth, and derive WAN state.
// Returns true on a successful fetch + parse. The raw body is logged to serial.
bool unifiFetchHealth(const String& host, const String& apiKey, const String& site,
                      UnifiHealth& out, uint32_t timeoutMs = 4000);

#endif // UNIFI_CLIENT_H
