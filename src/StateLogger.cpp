#include "StateLogger.h"
#include "config.h"
#include "TimeManager.h"
#include <LittleFS.h>

StateLogger::StateLogger() : currentLogIndex(0), totalEntries(0), lastSaveTime(0), timeManager(nullptr) {
    // Initialize log entries
    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        logEntries[i] = LogEntry{};
    }
}

void StateLogger::begin() {
    preferences.begin("state_log", false);
    loadLogFromFlash();
    Serial.println("StateLogger initialized with " + String(totalEntries) + " entries");
}

void StateLogger::logStateChange(int channel, const String& eventType, const String& oldValue, 
                                const String& newValue, const String& description) {
    LogEntry entry;
    entry.timestamp = timeManager ? timeManager->getCurrentTime() : (time_t)(millis() / 1000);
    entry.channel = channel;
    entry.eventType = eventType;
    entry.deviceId = "";
    entry.oldValue = oldValue;
    entry.newValue = newValue;
    entry.description = description;
    
    // Add to circular buffer
    logEntries[currentLogIndex] = entry;
    currentLogIndex = (currentLogIndex + 1) % MAX_LOG_ENTRIES;
    
    if (totalEntries < MAX_LOG_ENTRIES) {
        totalEntries++;
    }
    
    // Save to flash periodically
    if (millis() - lastSaveTime > LOG_SAVE_INTERVAL_MS) {
        saveLogToFlash();
        lastSaveTime = millis();
    }
    
    // Also log to serial for debugging
    String logText = formatLogEntry(entry);
    Serial.println("LOG: " + logText);
}

void StateLogger::logPairingEvent(const String& eventType, const String& deviceId, const String& description) {
    LogEntry entry;
    entry.timestamp = timeManager ? timeManager->getCurrentTime() : (time_t)(millis() / 1000);
    entry.channel = 0;  // System event
    entry.eventType = "PAIRING";
    entry.deviceId = deviceId;
    entry.oldValue = "";
    entry.newValue = eventType;
    entry.description = description;
    
    // Add to circular buffer
    logEntries[currentLogIndex] = entry;
    currentLogIndex = (currentLogIndex + 1) % MAX_LOG_ENTRIES;
    
    if (totalEntries < MAX_LOG_ENTRIES) {
        totalEntries++;
    }
    
    // Save to flash periodically
    if (millis() - lastSaveTime > LOG_SAVE_INTERVAL_MS) {
        saveLogToFlash();
        lastSaveTime = millis();
    }
    
    // Also log to serial for debugging
    String logText = formatLogEntry(entry);
    Serial.println("LOG: " + logText);
}

void StateLogger::logSystemEvent(const String& description) {
    LogEntry entry;
    entry.timestamp = timeManager ? timeManager->getCurrentTime() : (time_t)(millis() / 1000);
    entry.channel = 0;  // System event
    entry.eventType = "SYSTEM";
    entry.deviceId = "";
    entry.oldValue = "";
    entry.newValue = "";
    entry.description = description;
    
    // Add to circular buffer
    logEntries[currentLogIndex] = entry;
    currentLogIndex = (currentLogIndex + 1) % MAX_LOG_ENTRIES;
    
    if (totalEntries < MAX_LOG_ENTRIES) {
        totalEntries++;
    }
    
    // Save to flash periodically
    if (millis() - lastSaveTime > LOG_SAVE_INTERVAL_MS) {
        saveLogToFlash();
        lastSaveTime = millis();
    }
    
    // Also log to serial for debugging
    String logText = formatLogEntry(entry);
    Serial.println("LOG: " + logText);
}

String StateLogger::formatLogEntry(const LogEntry& entry) {
    String result = "[" + String(entry.timestamp) + "] ";
    
    if (entry.channel > 0) {
        result += "CH" + String(entry.channel) + " ";
    }
    
    result += entry.eventType;
    
    if (entry.deviceId.length() > 0) {
        result += " (" + entry.deviceId + ")";
    }
    
    if (entry.oldValue.length() > 0 || entry.newValue.length() > 0) {
        result += ": " + entry.oldValue + " -> " + entry.newValue;
    }
    
    if (entry.description.length() > 0) {
        result += " - " + entry.description;
    }
    
    return result;
}

String StateLogger::getLogAsJSON(int maxEntries) {
    JsonDocument doc;
    JsonArray logArray = doc.to<JsonArray>();
    
    int entriesToProcess = min(maxEntries, totalEntries);
    int startIndex = (currentLogIndex - entriesToProcess + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    
    for (int i = 0; i < entriesToProcess; i++) {
        int index = (startIndex + i) % MAX_LOG_ENTRIES;
        const LogEntry& entry = logEntries[index];
        
        JsonObject entryObj = logArray.createNestedObject();
        entryObj["timestamp"] = entry.timestamp;
        entryObj["channel"] = entry.channel;
        entryObj["eventType"] = entry.eventType;
        entryObj["deviceId"] = entry.deviceId;
        entryObj["oldValue"] = entry.oldValue;
        entryObj["newValue"] = entry.newValue;
        entryObj["description"] = entry.description;
        entryObj["formatted"] = formatLogEntry(entry);
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

String StateLogger::getLogAsText(int maxEntries) {
    String result = "";
    int entriesToProcess = min(maxEntries, totalEntries);
    int startIndex = (currentLogIndex - entriesToProcess + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    
    for (int i = 0; i < entriesToProcess; i++) {
        int index = (startIndex + i) % MAX_LOG_ENTRIES;
        const LogEntry& entry = logEntries[index];
        
        result += formatLogEntry(entry) + "\n";
    }
    
    return result;
}

String StateLogger::getRecentLogHTML(int maxEntries) {
    String html = "";
    int entriesToProcess = min(maxEntries, totalEntries);
    int startIndex = (currentLogIndex - entriesToProcess + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    
    for (int i = 0; i < entriesToProcess; i++) {
        int index = (startIndex + i) % MAX_LOG_ENTRIES;
        const LogEntry& entry = logEntries[index];
        
        html += "<div class='log-entry'>";
        
        // Timestamp
        unsigned long seconds = entry.timestamp / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        html += "<span class='log-time'>" + String(hours % 24) + ":" + String(minutes % 60) + ":" + String(seconds % 60) + "</span>";
        
        // Channel
        if (entry.channel > 0) {
            html += "<span class='log-channel'>CH" + String(entry.channel) + "</span>";
        }
        
        // Event type
        String eventClass = entry.eventType;
eventClass.toLowerCase();
        html += "<span class='log-event " + eventClass + "'>" + entry.eventType + "</span>";
        
        // Device ID
        if (entry.deviceId.length() > 0) {
            html += "<span class='log-device'>" + entry.deviceId + "</span>";
        }
        
        // Value change
        if (entry.oldValue.length() > 0 || entry.newValue.length() > 0) {
            html += "<span class='log-change'>" + entry.oldValue + " → " + entry.newValue + "</span>";
        }
        
        // Description
        if (entry.description.length() > 0) {
            html += "<span class='log-desc'>" + entry.description + "</span>";
        }
        
        html += "</div>";
    }
    
    return html;
}

LogEntry StateLogger::getRecentEntry(int offset) {
    if (totalEntries == 0) {
        return LogEntry{};  // Return empty entry
    }
    
    int index = (currentLogIndex - 1 - offset + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    return logEntries[index];
}

void StateLogger::clearLog() {
    currentLogIndex = 0;
    totalEntries = 0;
    
    // Clear all entries
    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        logEntries[i] = LogEntry{};
    }
    
    // Clear flash storage
    preferences.clear();
    preferences.end();
    preferences.begin("state_log", false);
    
    Serial.println("Log cleared");
}

void StateLogger::saveLogToFlash() {
    // Save current log state
    preferences.putInt("log_index", currentLogIndex);
    preferences.putInt("log_total", totalEntries);
    preferences.putULong("last_save", lastSaveTime);
    
    // Save recent entries (limit to avoid flash wear)
    int entriesToSave = min(50, totalEntries);
    preferences.putInt("saved_count", entriesToSave);
    
    for (int i = 0; i < entriesToSave; i++) {
        int index = (currentLogIndex - entriesToSave + i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
        const LogEntry& entry = logEntries[index];
        
        String key = "log_" + String(i);
        String value = String(entry.timestamp) + "|" + String(entry.channel) + "|" + 
                      entry.eventType + "|" + entry.deviceId + "|" + 
                      entry.oldValue + "|" + entry.newValue + "|" + entry.description;
        
        preferences.putString(key.c_str(), value);
    }
}

void StateLogger::loadLogFromFlash() {
    currentLogIndex = preferences.getInt("log_index", 0);
    totalEntries = preferences.getInt("log_total", 0);
    lastSaveTime = preferences.getULong("last_save", 0);
    
    int savedCount = preferences.getInt("saved_count", 0);
    
    // Load saved entries
    for (int i = 0; i < savedCount && i < MAX_LOG_ENTRIES; i++) {
        String key = "log_" + String(i);
        String value = preferences.getString(key.c_str(), "");
        
        if (value.length() > 0) {
            // Parse the saved entry
            int parts[7] = {0};
            String partStrings[7];
            int partIndex = 0;
            int start = 0;
            
            for (int j = 0; j < value.length() && partIndex < 7; j++) {
                if (value.charAt(j) == '|') {
                    partStrings[partIndex] = value.substring(start, j);
                    partIndex++;
                    start = j + 1;
                }
            }
            if (partIndex < 7) {
                partStrings[partIndex] = value.substring(start);
            }
            
            LogEntry entry;
            entry.timestamp = partStrings[0].toInt();
            entry.channel = partStrings[1].toInt();
            entry.eventType = partStrings[2];
            entry.deviceId = partStrings[3];
            entry.oldValue = partStrings[4];
            entry.newValue = partStrings[5];
            entry.description = partStrings[6];
            
            logEntries[i] = entry;
        }
    }
    
    Serial.println("Loaded " + String(savedCount) + " log entries from flash");
}

void StateLogger::update() {
    // Periodic maintenance - save to flash if needed
    if (millis() - lastSaveTime > LOG_SAVE_INTERVAL_MS) {
        saveLogToFlash();
        lastSaveTime = millis();
    }
}
