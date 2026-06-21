#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

class TimeManager {
private:
    WiFiUDP ntpUDP;
    NTPClient* ntpClient;
    bool timeInitialized;
    unsigned long lastSyncTime;
    const unsigned long syncInterval = 3600000; // Sync every hour
    int32_t timezoneOffset = 0; // Timezone offset in seconds
    bool timezoneInitialized = false;
    int lastTimezoneRequeryDay = -1; // Track day of last timezone requery
    unsigned long lastTimezoneCheck = 0; // Track last time we checked for requery
    const unsigned long timezoneCheckInterval = 10000; // Check every 10 seconds
    
    String getPublicIP();
    bool detectTimezone(const String& ip);
    void requeryTimezoneIfNeeded();

public:
    TimeManager();
    ~TimeManager();
    
    void begin();
    void update();
    bool isTimeInitialized() const { return timeInitialized; }
    // GMT/UTC time methods (for storage)
    time_t getCurrentGMTTime() const;
    String formatGMTTime(time_t t) const;
    String formatGMTDate(time_t t) const;
    String getFormattedGMTDateTime() const;
    
    // Local time methods (for display)
    time_t getCurrentTime() const;
    String getCurrentTimeStr() const;
    String formatTime(time_t t) const;
    String formatDate(time_t t) const;
    String getFormattedDateTime() const;
    
    // Convert between GMT and local time
    time_t gmtToLocal(time_t gmtTime) const;
    time_t localToGMT(time_t localTime) const;
    int32_t getTimezoneOffset() const { return timezoneOffset; }
    bool isTimezoneInitialized() const { return timezoneInitialized; }
};

#endif // TIMEMANAGER_H
