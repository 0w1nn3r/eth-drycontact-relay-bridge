#ifndef STATELOGGER_H
#define STATELOGGER_H

#include "config.h"
#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>

// Forward declaration
class TimeManager;

// Log entry structure
struct LogEntry {
    time_t timestamp;  // Use proper time_t instead of millis
    int channel;  // 1 or 2
    String eventType;  // "DRY_CONTACT", "RELAY", "PAIRING"
    String deviceId;  // Device ID for pairing events
    String oldValue;
    String newValue;
    String description;
};

class StateLogger {
private:
    LogEntry logEntries[MAX_LOG_ENTRIES];
    int currentLogIndex;
    int totalEntries;
    Preferences preferences;
    unsigned long lastSaveTime;
    TimeManager* timeManager;  // Pointer to TimeManager for real timestamps
    
    void saveLogToFlash();
    void loadLogFromFlash();
    String formatLogEntry(const LogEntry& entry);
    
public:
    StateLogger();
    void setTimeManager(TimeManager* tm) { timeManager = tm; }
    void begin();
    void logStateChange(int channel, const String& eventType, const String& oldValue, 
                       const String& newValue, const String& description = "");
    void logPairingEvent(const String& eventType, const String& deviceId, const String& description = "");
    void logSystemEvent(const String& description);
    String getLogAsJSON(int maxEntries = 50);
    String getLogAsText(int maxEntries = 50);
    void clearLog();
    int getLogCount() { return totalEntries; }
    void update();
    
    // Get recent entries for display
    LogEntry getRecentEntry(int offset = 0);
    String getRecentLogHTML(int maxEntries = 10);
};

#endif // STATELOGGER_H
