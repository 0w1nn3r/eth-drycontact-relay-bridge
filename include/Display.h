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
    bool inputState[2];     // For sender: dry contact state for channels 1 & 2
    bool outputState[2];    // For receiver: relay state for channels 1 & 2
    bool ethernetConnected;
    String channelNames[2]; // Custom channel names
    
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
    void setInputState(int channel, bool state) { 
        if (channel >= 1 && channel <= 2) inputState[channel - 1] = state; 
    }
    void setOutputState(int channel, bool state) { 
        if (channel >= 1 && channel <= 2) outputState[channel - 1] = state; 
    }
    void setEthernetConnected(bool connected) { ethernetConnected = connected; }
    void setChannelNames(const String& name1, const String& name2) { 
        channelNames[0] = name1; 
        channelNames[1] = name2; 
    }
    void showFactoryReset();
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
