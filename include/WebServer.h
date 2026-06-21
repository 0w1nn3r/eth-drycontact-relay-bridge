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

// Forward declarations
class Display;

class WebServer {
private:
    ESP32WebServer server;
    Display* display;
    
    // References to main application variables
    String& deviceID;
    OperationMode& currentMode;
    bool& ethernetConnected;
    bool& jumperModeDetected;
    bool& isPaired;
    String& pairedDeviceID;
    String& pairedDeviceIP;
    String& receiverIP;
    bool& useTCP;
    bool& discoveryEnabled;
    bool& dryContactState;
    bool& relayState;
    
    // Web server handlers
    void handleRoot();
    void handleAPI();
    void handleConfig();
    void handleMode();
    void handleStatus();
    void handleDiscovery();
    
    // Helper methods
    String generateRootHTML();
    void updateDisplayWithPairingInfo();
    void sendPairingResponse(const String& status, const String& message = "");
    
public:
    WebServer(String& deviceID, OperationMode& currentMode, bool& ethernetConnected,
              bool& jumperModeDetected, bool& isPaired, String& pairedDeviceID,
              String& pairedDeviceIP, String& receiverIP, bool& useTCP,
              bool& discoveryEnabled, bool& dryContactState, bool& relayState);
    
    void setDisplay(Display* displayPtr) { display = displayPtr; }
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
