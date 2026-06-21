#ifndef SNMP_CLIENT_H
#define SNMP_CLIENT_H

#include <Arduino.h>

// Minimal SNMP v1 manager: performs a single GetRequest for the two standard
// RFC 1628 UPS-MIB scalars and returns their integer values.
//
//   upsOutputSource   1.3.6.1.2.1.33.1.4.1.0  -> outputSource
//   upsBatteryStatus  1.3.6.1.2.1.33.1.2.1.0  -> batteryStatus
//
// Returns true only when a well-formed response with both integers is received
// before the timeout. No third-party dependency; uses a UDP socket directly.
bool snmpGetUpsState(const String& host, uint16_t port, const String& community,
                     int& outputSource, int& batteryStatus, uint32_t timeoutMs = 800);

#endif // SNMP_CLIENT_H
