#include "UnifiClient.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

bool unifiFetchHealth(const String& host, const String& apiKey, const String& site,
                      UnifiHealth& out, uint32_t timeoutMs) {
    if (host.length() == 0 || apiKey.length() == 0) {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();  // UniFi gateways serve a self-signed certificate
    client.setTimeout(timeoutMs / 1000 > 0 ? timeoutMs / 1000 : 1);

    String url = "https://" + host + "/proxy/network/api/s/" + site + "/stat/health";

    HTTPClient https;
    https.setTimeout(timeoutMs);
    if (!https.begin(client, url)) {
        Serial.println("UniFi: https.begin() failed");
        return false;
    }
    https.addHeader("X-API-KEY", apiKey);
    https.addHeader("Accept", "application/json");

    int code = https.GET();
    if (code != HTTP_CODE_OK) {
        Serial.println("UniFi: HTTP GET failed, code=" + String(code));
        https.end();
        return false;
    }

    String body = https.getString();
    https.end();

    // Log the raw body so the exact WAN fields can be verified against a real
    // console (especially dual-WAN / failover, which this build infers).
    Serial.println("UniFi health raw: " + body);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.println("UniFi: JSON parse error: " + String(err.c_str()));
        return false;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    if (data.isNull()) {
        return false;
    }

    JsonObject wan;
    for (JsonObject o : data) {
        const char* sub = o["subsystem"] | "";
        if (strcmp(sub, "wan") == 0) {
            wan = o;
            break;
        }
    }
    if (wan.isNull()) {
        return false;
    }

    out.wanStatus = (const char*)(wan["status"] | "unknown");
    bool statusError = (out.wanStatus == "error");

    // Per-WAN detail. A second uplink appears as uptime_stats.WAN2; availability
    // is a rolling percentage (100.0 = healthy, 0.0 = fully down).
    JsonObject us = wan["uptime_stats"];
    bool hasWan2 = !us["WAN2"].isNull();
    float wanAvail = us["WAN"]["availability"] | -1.0f;
    float wan2Avail = us["WAN2"]["availability"] | -1.0f;

    // Primary down: instantaneous WAN subsystem error, or primary availability
    // pinned at zero.
    out.primaryDown = statusError || (wanAvail >= 0.0f && wanAvail == 0.0f);
    // Secondary down only makes sense with a second uplink present.
    out.secondaryDown = hasWan2 && (wan2Avail >= 0.0f && wan2Avail == 0.0f);
    // Failover: primary is down while a healthy secondary is carrying traffic.
    out.failover = hasWan2 && out.primaryDown && !out.secondaryDown;

    return true;
}
