#include "SnmpClient.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <string.h>

// Pre-encoded OID bodies (the bytes that follow the "06 <len>" OID header).
// Encoding the OIDs as fixed byte arrays removes the most error-prone part of
// hand-rolling SNMP. First two sub-identifiers 1.3 collapse to 0x2B; 33 -> 0x21.
//   1.3.6.1.2.1.33.1.4.1.0  (upsOutputSource.0)
static const uint8_t OID_OUTPUT_SOURCE[] =
    {0x2B, 0x06, 0x01, 0x02, 0x01, 0x21, 0x01, 0x04, 0x01, 0x00};
//   1.3.6.1.2.1.33.1.2.1.0  (upsBatteryStatus.0)
static const uint8_t OID_BATTERY_STATUS[] =
    {0x2B, 0x06, 0x01, 0x02, 0x01, 0x21, 0x01, 0x02, 0x01, 0x00};

// Append a varbind (OID + NULL value) to buf at pos; returns the new position.
// All lengths here are < 128, so single-byte BER length encoding is sufficient.
static size_t appendVarbind(uint8_t* buf, size_t pos,
                            const uint8_t* oid, size_t oidLen) {
    size_t innerLen = 2 + oidLen + 2;     // (06 len oid) + (05 00)
    buf[pos++] = 0x30;                     // SEQUENCE (varbind)
    buf[pos++] = (uint8_t)innerLen;
    buf[pos++] = 0x06;                     // OID
    buf[pos++] = (uint8_t)oidLen;
    memcpy(buf + pos, oid, oidLen);
    pos += oidLen;
    buf[pos++] = 0x05;                     // NULL value
    buf[pos++] = 0x00;
    return pos;
}

// Build an SNMP v1 GetRequest for the two UPS OIDs. Returns the packet length.
static size_t buildGetRequest(uint8_t* out, const String& community,
                              uint32_t requestId) {
    // Varbind list body
    uint8_t vbl[64];
    size_t vblLen = 0;
    vblLen = appendVarbind(vbl, vblLen, OID_OUTPUT_SOURCE, sizeof(OID_OUTPUT_SOURCE));
    vblLen = appendVarbind(vbl, vblLen, OID_BATTERY_STATUS, sizeof(OID_BATTERY_STATUS));

    // GetRequest PDU body
    uint8_t pdu[128];
    size_t p = 0;
    pdu[p++] = 0x02; pdu[p++] = 0x04;      // INTEGER request-id (4 bytes)
    pdu[p++] = (requestId >> 24) & 0xFF;
    pdu[p++] = (requestId >> 16) & 0xFF;
    pdu[p++] = (requestId >> 8) & 0xFF;
    pdu[p++] = requestId & 0xFF;
    pdu[p++] = 0x02; pdu[p++] = 0x01; pdu[p++] = 0x00;  // INTEGER error-status 0
    pdu[p++] = 0x02; pdu[p++] = 0x01; pdu[p++] = 0x00;  // INTEGER error-index 0
    pdu[p++] = 0x30; pdu[p++] = (uint8_t)vblLen;        // SEQUENCE varbind list
    memcpy(pdu + p, vbl, vblLen);
    p += vblLen;

    // Message: SEQUENCE { version, community, PDU }
    size_t m = 0;
    out[m++] = 0x30;                       // SEQUENCE
    size_t lenPos = m++;                   // length placeholder (single byte)
    out[m++] = 0x02; out[m++] = 0x01; out[m++] = 0x00;  // INTEGER version = 0 (v1)
    out[m++] = 0x04;                       // OCTET STRING community
    out[m++] = (uint8_t)community.length();
    memcpy(out + m, community.c_str(), community.length());
    m += community.length();
    out[m++] = 0xA0;                       // GetRequest PDU
    out[m++] = (uint8_t)p;
    memcpy(out + m, pdu, p);
    m += p;
    out[lenPos] = (uint8_t)(m - lenPos - 1);
    return m;
}

// Find "06 <oidLen> <oid>" in buf and, if the following value is an INTEGER,
// decode it into value. Returns false if not found or the value isn't an int.
static bool readIntAfterOid(const uint8_t* buf, size_t len,
                            const uint8_t* oid, size_t oidLen, int& value) {
    for (size_t i = 0; i + 2 + oidLen + 2 <= len; i++) {
        if (buf[i] == 0x06 && buf[i + 1] == oidLen &&
            memcmp(buf + i + 2, oid, oidLen) == 0) {
            size_t v = i + 2 + oidLen;     // start of the value TLV
            if (v + 1 < len && buf[v] == 0x02) {  // INTEGER
                size_t ilen = buf[v + 1];
                if (ilen >= 1 && ilen <= 4 && v + 2 + ilen <= len) {
                    int32_t val = (buf[v + 2] & 0x80) ? -1 : 0;  // sign-extend
                    for (size_t k = 0; k < ilen; k++) {
                        val = (val << 8) | buf[v + 2 + k];
                    }
                    value = (int)val;
                    return true;
                }
            }
            return false;  // OID present but value is NULL / noSuchObject / malformed
        }
    }
    return false;
}

bool snmpGetUpsState(const String& host, uint16_t port, const String& community,
                     int& outputSource, int& batteryStatus, uint32_t timeoutMs) {
    if (host.length() == 0 || community.length() > 64) {
        return false;
    }

    IPAddress ip;
    if (!ip.fromString(host)) {
        return false;
    }

    WiFiUDP sock;
    if (!sock.begin(SNMP_LOCAL_PORT)) {
        return false;
    }

    static uint32_t counter = 0;
    counter++;
    uint32_t requestId = (millis() ^ (counter << 8)) & 0x7FFFFFFF;

    uint8_t request[160];
    size_t reqLen = buildGetRequest(request, community, requestId);

    if (sock.beginPacket(ip, port) == 0) {
        sock.stop();
        return false;
    }
    sock.write(request, reqLen);
    sock.endPacket();

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        int psize = sock.parsePacket();
        if (psize > 0) {
            uint8_t resp[256];
            int n = sock.read(resp, sizeof(resp));
            sock.stop();
            if (n <= 0) {
                return false;
            }
            int os = 0, bs = 0;
            bool gotOs = readIntAfterOid(resp, (size_t)n, OID_OUTPUT_SOURCE,
                                         sizeof(OID_OUTPUT_SOURCE), os);
            bool gotBs = readIntAfterOid(resp, (size_t)n, OID_BATTERY_STATUS,
                                         sizeof(OID_BATTERY_STATUS), bs);
            if (gotOs && gotBs) {
                outputSource = os;
                batteryStatus = bs;
                return true;
            }
            return false;
        }
        delay(5);
    }

    sock.stop();
    return false;
}
