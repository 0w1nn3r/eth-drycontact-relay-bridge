#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "Display.h"
#include "StateLogger.h"
#include "UpsState.h"

// Forward declarations
class Display;
class StateLogger;

class WebServer {
private:
    ESP32WebServer server;
    Display* display;
    StateLogger* stateLogger;
    
    // References to main application variables
    String& deviceID;
    OperationMode& currentMode;
    bool& ethernetConnected;
    bool& jumperModeDetected;
    bool& isPaired;
    String& pairedDeviceID;
    String& pairedDeviceIP;
    String& receiverIP;
    bool& discoveryEnabled;
    bool& peerOnline;
    UpsState& ups;
    bool* dryContactState;
    bool* relayState;
    String& channel1Name;
    String& channel2Name;
    std::function<void()> saveChannelNamesCallback;
    std::function<void()> saveUpsConfigCallback;
    
    // Web server handlers
    void handleRoot();
    void handleAPI();
    void handleConfig();
    void handleStatus();
    void handleDiscovery();
    void handleLog();
    void handlePairing();
    void handleCommand();

    // Enhanced handler methods
    void handleChannelNames();
    void handleUpsConfig();

    // Helper methods
    String generateRootHTML();
    String upsSourceOptions(int selected);
    void updateDisplayWithPairingInfo();
    void sendPairingResponse(const String& status, const String& message = "");
    
public:
    WebServer(String& deviceID, OperationMode& currentMode, bool& ethernetConnected,
              bool& jumperModeDetected, bool& isPaired, String& pairedDeviceID,
              String& pairedDeviceIP, String& receiverIP,
              bool& discoveryEnabled, bool& peerOnline, UpsState& ups,
              bool* dryContactState, bool* relayState,
              String& channel1Name, String& channel2Name);

    void setDisplay(Display* displayPtr) { display = displayPtr; }
    void setStateLogger(StateLogger* loggerPtr) { stateLogger = loggerPtr; }
    void setChannelNameSaveCallback(std::function<void()> callback) { saveChannelNamesCallback = callback; }
    void setUpsConfigSaveCallback(std::function<void()> callback) { saveUpsConfigCallback = callback; }
    void begin();
    void handleClient();
    
    // Pairing management methods
    void handlePairingRequest(const String& action);
    void sendDiscoveryBroadcast();
    void scanForSenders();
    void pairWithDevice(const String& deviceID, const String& deviceIP);
    void unpairDevice();
};

#endif // WEBSERVER_H
