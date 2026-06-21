#include "TimeManager.h"

TimeManager::TimeManager() : timeInitialized(false), lastSyncTime(0) {
    ntpClient = new NTPClient(ntpUDP, "pool.ntp.org", 0); // Start with UTC, we'll adjust for timezone later
}

TimeManager::~TimeManager() {
    if (ntpClient) {
        delete ntpClient;
    }
}

String TimeManager::getPublicIP() {
    if (WiFi.status() != WL_CONNECTED) {
        return "";
    }

    HTTPClient http;
    http.begin("http://api.ipify.org");
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String ip = http.getString();
        http.end();
        return ip;
    }

    http.end();
    return "";
}

bool TimeManager::detectTimezone(const String& ip) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No WiFi connection");
        return false;
    }

    // Wait a bit after WiFi connection before making the request
    delay(1000);

    const int maxRetries = 5;
    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            // Wait longer between retries
            delay(2000);
        }

        HTTPClient http;
        String url = "http://ip-api.com/json/?fields=timezone,offset,status,message";
        http.begin(url);
        int httpCode = http.GET();

        Serial.print("IP-API response code (attempt ");
        Serial.print(retry + 1);
        Serial.print("/");
        Serial.print(maxRetries);
        Serial.print("): ");
        Serial.println(httpCode);

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.print("IP-API response: ");
            Serial.println(payload);

            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, payload);

            http.end();

            if (error) {
                Serial.print("JSON parse error: ");
                Serial.println(error.c_str());
                continue;
            }

            // Check if the request was successful
            const char* status = doc["status"].as<const char*>();
            if (strcmp(status, "success") != 0) {
                Serial.print("API error: ");
                Serial.println(doc["message"].as<const char*>());
                continue;
            }

            // Get timezone offset in seconds
            timezoneOffset = doc["offset"].as<int32_t>();
            timezoneInitialized = true;
            
            // Keep NTP client in GMT/UTC
            ntpClient->setTimeOffset(0);
            
            Serial.print("Timezone detected with offset: ");
            Serial.print(timezoneOffset);
            Serial.println(" seconds");
            
            return true;
        }
        
        http.end();
        Serial.println("Request failed, retrying...");
    }
    
    Serial.println("Failed to detect timezone after all retries");
    return false;
}

void TimeManager::begin() {
    ntpClient->begin();
    
    // Try to detect timezone
    String ip = getPublicIP();
    if (ip.length() > 0) {
        if (detectTimezone(ip)) {
            Serial.println("Timezone detection successful");
        } else {
            Serial.println("Timezone detection failed, using UTC");
        }
    } else {
        Serial.println("Could not get public IP, using UTC");
    }
    
    update(); // Initial time sync
}

void TimeManager::requeryTimezoneIfNeeded() {
    if (!timeInitialized || !timezoneInitialized) {
        return; // Skip if time or timezone not initialized yet
    }
    
    unsigned long currentMillis = millis();
    
    // Only check every second to avoid excessive time function calls
    if (currentMillis - lastTimezoneCheck < timezoneCheckInterval) {
        return;
    }
    lastTimezoneCheck = currentMillis;
    
    // Only get current time when we need to check for requery
    time_t localTime = getCurrentTime();
    struct tm* timeinfo = localtime(&localTime);
    int currentDay = timeinfo->tm_yday; // Day of year (0-365)
    
    // Check if we're at 5 seconds past midnight and haven't requeried today
    if (timeinfo->tm_hour == 0 && timeinfo->tm_min == 0 && timeinfo->tm_sec == 5 && currentDay != lastTimezoneRequeryDay) {
        Serial.println("Daily timezone requery at 5 seconds past midnight");
        
        // Get current IP and requery timezone
        String ip = getPublicIP();
        if (ip.length() > 0) {
            int32_t oldOffset = timezoneOffset;
            if (detectTimezone(ip)) {
                Serial.println("Timezone requery successful");
                if (oldOffset != timezoneOffset) {
                    Serial.print("Timezone offset changed from ");
                    Serial.print(oldOffset);
                    Serial.print(" to ");
                    Serial.print(timezoneOffset);
                    Serial.println(" seconds (DST change detected)");
                } else {
                    Serial.println("Timezone offset unchanged");
                }
            } else {
                Serial.println("Timezone requery failed, keeping previous settings");
            }
        } else {
            Serial.println("Could not get public IP for timezone requery");
        }
        
        // Update last requery day regardless of success to avoid multiple attempts
        lastTimezoneRequeryDay = currentDay;
    }
}

void TimeManager::update() {
    unsigned long currentMillis = millis();
    
    // Only sync if not initialized or if sync interval has passed
    if (!timeInitialized || (currentMillis - lastSyncTime >= syncInterval)) {
        if (ntpClient->update()) {
            // Convert NTP time to time_t
            time_t epochTime = ntpClient->getEpochTime();
            struct timeval tv = { .tv_sec = epochTime, .tv_usec = 0 };
            settimeofday(&tv, nullptr);
            
            timeInitialized = true;
            lastSyncTime = currentMillis;
            
            Serial.println("Time synchronized with NTP server");
            Serial.print("Current time: ");
            Serial.println(getFormattedDateTime());
        } else {
            Serial.println("Failed to sync time with NTP server");
        }
    }
    
    // Check if we need to requery timezone (DST changes)
    requeryTimezoneIfNeeded();
}

time_t TimeManager::getCurrentGMTTime() const {
    return ntpClient->getEpochTime();
}

String TimeManager::formatGMTTime(time_t t) const {
    char timeStr[9];
    struct tm* timeinfo = gmtime(&t);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
    return String(timeStr);
}

String TimeManager::formatGMTDate(time_t t) const {
    char dateStr[11];
    struct tm* timeinfo = gmtime(&t);
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);
    return String(dateStr);
}

String TimeManager::getFormattedGMTDateTime() const {
    time_t t = getCurrentGMTTime();
    return formatGMTDate(t) + " " + formatGMTTime(t);
}

time_t TimeManager::gmtToLocal(time_t gmtTime) const {
    return gmtTime + timezoneOffset;
}

time_t TimeManager::localToGMT(time_t localTime) const {
    return localTime - timezoneOffset;
}

time_t TimeManager::getCurrentTime() const {
    return gmtToLocal(getCurrentGMTTime());
}

String TimeManager::getCurrentTimeStr() const {
    return formatTime(getCurrentTime());
}

String TimeManager::formatTime(time_t t) const {
    char buffer[9]; // HH:MM:SS + null terminator
    struct tm* timeinfo = localtime(&t);
    sprintf(buffer, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return String(buffer);
}

String TimeManager::formatDate(time_t t) const {
    char buffer[16]; // YYYY-MM-DD + null terminator with extra space
    struct tm* timeinfo = localtime(&t);
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
    return String(buffer);
}

String TimeManager::getFormattedDateTime() const {
    char buffer[32]; // YYYY-MM-DD HH:MM:SS + null terminator with extra space
    time_t t = getCurrentTime();
    struct tm* timeinfo = localtime(&t);
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", 
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return String(buffer);
}
