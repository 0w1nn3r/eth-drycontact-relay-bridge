#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

class Display {
private:
    Adafruit_SSD1306 display;
    bool displayAvailable;
    unsigned long lastUpdateTime;
    unsigned long updateInterval;
    
    // Display data
    String deviceID;
    OperationMode currentMode;
    String localIP;
    String remoteIP;
    bool inputState;     // For sender: dry contact state
    bool outputState;    // For receiver: relay state
    bool ethernetConnected;
    
public:
    Display();
    bool init();
    void update();
    void showStartupScreen();
    void showConnecting();
    void showConnected();
    void setDeviceID(const String& id) { deviceID = id; }
    void setMode(OperationMode mode) { currentMode = mode; }
    void setLocalIP(const String& ip) { localIP = ip; }
    void setRemoteIP(const String& ip) { remoteIP = ip; }
    void setInputState(bool state) { inputState = state; }
    void setOutputState(bool state) { outputState = state; }
    void setEthernetConnected(bool connected) { ethernetConnected = connected; }
    bool isAvailable() const { return displayAvailable; }
    
private:
    void drawHeader();
    void drawModeInfo();
    void drawNetworkInfo();
    void drawStateInfo();
    void drawStatusBar();
    String getModeText();
    String getModeIcon();
};

#endif // DISPLAY_H
