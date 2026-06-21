#include "WebServer.h"

WebServer::WebServer(String& deviceID, OperationMode& currentMode, bool& ethernetConnected,
                     bool& jumperModeDetected, bool& isPaired, String& pairedDeviceID,
                     String& pairedDeviceIP, String& receiverIP, bool& useTCP,
                     bool& discoveryEnabled, bool& dryContactState, bool& relayState)
    : server(WEB_PORT),
      display(nullptr),
      deviceID(deviceID),
      currentMode(currentMode),
      ethernetConnected(ethernetConnected),
      jumperModeDetected(jumperModeDetected),
      isPaired(isPaired),
      pairedDeviceID(pairedDeviceID),
      pairedDeviceIP(pairedDeviceIP),
      receiverIP(receiverIP),
      useTCP(useTCP),
      discoveryEnabled(discoveryEnabled),
      dryContactState(dryContactState),
      relayState(relayState) {
}

void WebServer::begin() {
    // Setup web server routes
    server.on("/", HTTP_GET, std::bind(&WebServer::handleRoot, this));
    server.on("/api", HTTP_ANY, std::bind(&WebServer::handleAPI, this));
    server.on("/config", HTTP_ANY, std::bind(&WebServer::handleConfig, this));
    server.on("/mode", HTTP_ANY, std::bind(&WebServer::handleMode, this));
    server.on("/status", HTTP_GET, std::bind(&WebServer::handleStatus, this));
    server.on("/discovery", HTTP_GET, std::bind(&WebServer::handleDiscovery, this));
    
    server.begin();
    Serial.println("HTTP server started");
}

void WebServer::handleClient() {
    server.handleClient();
}

void WebServer::handleRoot() {
    String html = generateRootHTML();
    server.send(200, "text/html", html);
}

String WebServer::generateRootHTML() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>)" + String(BOARD_NAME) + R"(</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .container { max-width: 600px; margin: 0 auto; }
        .status { background: #f0f0f0; padding: 10px; margin: 10px 0; border-radius: 5px; }
        .button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }
        .button:hover { background: #0056b3; }
        .form-group { margin: 10px 0; }
        label { display: block; margin-bottom: 5px; }
        input, select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>)" + String(BOARD_NAME) + R"(</h1>
        <div class="status">
            <h3>Device Status</h3>
            <p><strong>Device ID:</strong> )" + deviceID + R"(</p>
            <p><strong>Firmware:</strong> )" + FIRMWARE_VERSION + R"(</p>
            <p><strong>Mode:</strong> <span id="mode">)" + String(currentMode) + R"(</span>)" + String(jumperModeDetected ? " (Jumper Set)" : "") + R"(</p>
            <p><strong>Ethernet:</strong> <span id="ethernet">)" + String(ethernetConnected ? "Connected" : "Disconnected") + R"(</span></p>
            <p><strong>IP Address:</strong> )" + ETH.localIP().toString() + R"(</p>
            <p><strong>Subnet Mask:</strong> )" + ETH.subnetMask().toString() + R"(</p>
            <p><strong>Gateway:</strong> )" + ETH.gatewayIP().toString() + R"(</p>
            <p><strong>DNS Server:</strong> )" + ETH.dnsIP().toString() + R"(</p>
            <p><strong>Paired:</strong> <span id="paired-status">)" + String(isPaired ? "Yes" : "No") + R"(</span></p>
            <p><strong>Paired Device:</strong> <span id="paired-device">)" + (isPaired ? pairedDeviceID : "None") + R"(</span></p>
            <p><strong>Paired IP:</strong> <span id="paired-ip">)" + (isPaired ? pairedDeviceIP : "None") + R"(</span></p>
        </div>
        
        <div class="form-group">
            <h3>Configuration</h3>
            <form method="POST" action="/mode">
                <label for="operationMode">Operation Mode:</label>
                <select name="mode" id="operationMode">
                    <option value="0">Not Configured</option>
                    <option value="1">Dry Contact Sender</option>
                    <option value="2">Relay Receiver</option>
                </select>
                <br><br>
                <label for="receiverIP">Receiver IP (for Sender mode):</label>
                <input type="text" id="receiverIP" name="receiverIP" placeholder="192.168.1.100" value=")" + receiverIP + R"(">
                <br><br>
                <label>
                    <input type="checkbox" name="useTCP" )" + String(useTCP ? "checked" : "") + R"(>
                    Use TCP instead of UDP
                </label>
                <br><br>
                <button type="submit" class="button">Save Configuration</button>
            </form>
            <br>
            <div class="pairing-controls">
                <h3>Pairing Management</h3>
                <button type="button" class="button" onclick="toggleDiscovery()" id="discovery-btn">)" + String(discoveryEnabled ? "Disable Discovery" : "Enable Discovery") + R"(</button>
                <br><br>
                )" + String(currentMode == MODE_RELAY_RECEIVER && !isPaired ? "<button type=\"button\" class=\"button\" onclick=\"scanForSenders()\" style=\"background-color: #ff9800;\">Scan for Senders</button>" : "") + R"(
                )" + String(isPaired ? "<button type=\"button\" class=\"button\" onclick=\"unpairDevice()\" style=\"background-color: #f44336;\">Unpair Device</button>" : "") + R"(
            </div>
        </div>
    </div>
    
    <script>
        // Update status every 2 seconds
        setInterval(function() {
            fetch('/api')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('mode').textContent = data.mode_text || 'Unknown';
                    document.getElementById('ethernet').textContent = data.ethernet_connected ? 'Connected' : 'Disconnected';
                    
                    // Update pairing status
                    document.getElementById('paired-status').textContent = data.is_paired ? 'Yes' : 'No';
                    document.getElementById('paired-device').textContent = data.paired_device_id || 'None';
                    document.getElementById('paired-ip').textContent = data.paired_device_ip || 'None';
                    
                    // Update discovery button
                    document.getElementById('discovery-btn').textContent = data.discovery_enabled ? 'Disable Discovery' : 'Enable Discovery';
                })
                .catch(error => console.log('Error:', error));
        }, 2000);
        
        function toggleDiscovery() {
            fetch('/api', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({action: 'toggle_discovery'})
            })
            .then(response => response.json())
            .then(data => {
                console.log('Discovery toggled:', data);
                location.reload(); // Reload to update UI
            })
            .catch(error => console.log('Error:', error));
        }
        
        function scanForSenders() {
            document.getElementById('paired-device').textContent = 'Scanning...';
            fetch('/api', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({action: 'scan_senders'})
            })
            .then(response => response.json())
            .then(data => {
                console.log('Scan completed:', data);
                setTimeout(() => location.reload(), 3000); // Reload after scan
            })
            .catch(error => console.log('Error:', error));
        }
        
        function unpairDevice() {
            if (confirm('Are you sure you want to unpair this device?')) {
                fetch('/api', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({action: 'unpair_device'})
                })
                .then(response => response.json())
                .then(data => {
                    console.log('Device unpaired:', data);
                    location.reload(); // Reload to update UI
                })
                .catch(error => console.log('Error:', error));
            }
        }
    </script>
</body>
</html>
)";
    return html;
}

void WebServer::handleAPI() {
    // Handle pairing management POST requests
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        JsonDocument requestDoc;
        DeserializationError error = deserializeJson(requestDoc, body);
        
        if (!error) {
            String postAction = requestDoc["action"] | "";
            
            if (postAction == "toggle_discovery") {
                discoveryEnabled = !discoveryEnabled;
                JsonDocument response;
                response["status"] = "success";
                response["discovery_enabled"] = discoveryEnabled;
                String resp;
                serializeJson(response, resp);
                server.send(200, "application/json", resp);
                return;
            } else if (postAction == "scan_senders") {
                if (currentMode == MODE_RELAY_RECEIVER && !isPaired) {
                    scanForSenders();
                }
                JsonDocument response;
                response["status"] = "success";
                response["message"] = "Scan completed";
                String resp;
                serializeJson(response, resp);
                server.send(200, "application/json", resp);
                return;
            } else if (postAction == "unpair_device") {
                unpairDevice();
                JsonDocument response;
                response["status"] = "success";
                response["message"] = "Device unpaired";
                String resp;
                serializeJson(response, resp);
                server.send(200, "application/json", resp);
                return;
            }
        }
    }
    
    // Handle GET requests for status
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["mode"] = currentMode;
    doc["mode_text"] = currentMode == MODE_DRY_CONTACT_SENDER ? "Dry Contact Sender" : 
                      currentMode == MODE_RELAY_RECEIVER ? "Relay Receiver" : "Not Configured";
    doc["jumper_mode"] = jumperModeDetected;
    doc["ethernet_connected"] = ethernetConnected;
    doc["ip_address"] = ETH.localIP().toString();
    doc["subnet_mask"] = ETH.subnetMask().toString();
    doc["gateway"] = ETH.gatewayIP().toString();
    doc["dns_server"] = ETH.dnsIP().toString();
    doc["mac_address"] = ETH.macAddress();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    
    if (currentMode == MODE_DRY_CONTACT_SENDER) {
        doc["dry_contact_state"] = dryContactState;
        doc["receiver_ip"] = receiverIP;
        doc["use_tcp"] = useTCP;
    } else if (currentMode == MODE_RELAY_RECEIVER) {
        doc["relay_state"] = relayState;
    }
    
    // Add pairing information
    doc["is_paired"] = isPaired;
    doc["paired_device_id"] = pairedDeviceID;
    doc["paired_device_ip"] = pairedDeviceIP;
    doc["discovery_enabled"] = discoveryEnabled;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServer::handleConfig() {
    if (server.method() == HTTP_POST) {
        if (server.hasArg("receiverIP")) {
            receiverIP = server.arg("receiverIP");
        }
        if (server.hasArg("useTCP")) {
            useTCP = server.arg("useTCP") == "on";
        }
        server.send(200, "text/plain", "Configuration saved");
    } else {
        JsonDocument doc;
        doc["receiver_ip"] = receiverIP;
        doc["use_tcp"] = useTCP;
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}

void WebServer::handleMode() {
    if (server.method() == HTTP_POST) {
        if (server.hasArg("mode")) {
            int newMode = server.arg("mode").toInt();
            if (newMode >= MODE_UNDEFINED && newMode <= MODE_RELAY_RECEIVER) {
                currentMode = (OperationMode)newMode;
                server.send(200, "text/plain", "Mode updated");
            } else {
                server.send(400, "text/plain", "Invalid mode");
            }
        } else {
            server.send(400, "text/plain", "Missing mode parameter");
        }
    } else {
        JsonDocument doc;
        doc["current_mode"] = currentMode;
        doc["modes"] = JsonArray();
        JsonArray modes = doc["modes"].to<JsonArray>();
        modes.add("Not Configured");
        modes.add("Dry Contact Sender");
        modes.add("Relay Receiver");
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}

void WebServer::handleStatus() {
    JsonDocument doc;
    doc["status"] = "online";
    doc["device_id"] = deviceID;
    doc["mode"] = currentMode;
    doc["ethernet_connected"] = ethernetConnected;
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServer::handleDiscovery() {
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["name"] = BOARD_NAME;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["mode"] = currentMode;
    doc["ip_address"] = ETH.localIP().toString();
    doc["protocol_version"] = PROTOCOL_VERSION;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServer::handlePairingRequest(const String& action) {
    if (action == "toggle_discovery") {
        discoveryEnabled = !discoveryEnabled;
        sendPairingResponse("success", "Discovery toggled");
    } else if (action == "scan_senders") {
        if (currentMode == MODE_RELAY_RECEIVER && !isPaired) {
            scanForSenders();
        }
        sendPairingResponse("success", "Scan completed");
    } else if (action == "unpair_device") {
        unpairDevice();
        sendPairingResponse("success", "Device unpaired");
    }
}

void WebServer::updateDisplayWithPairingInfo() {
    if (display && display->isAvailable()) {
        display->setRemoteIP(pairedDeviceIP);
    }
}

void WebServer::sendPairingResponse(const String& status, const String& message) {
    JsonDocument response;
    response["status"] = status;
    if (message.length() > 0) {
        response["message"] = message;
    }
    String resp;
    serializeJson(response, resp);
    server.send(200, "application/json", resp);
}

// Discovery and pairing methods (these will need to be implemented with UDP functionality)
void WebServer::sendDiscoveryBroadcast() {
    // This method will need access to UDP functionality
    // Implementation will be moved from main.cpp
}

void WebServer::scanForSenders() {
    // This method will need access to UDP functionality
    // Implementation will be moved from main.cpp
}

void WebServer::pairWithDevice(const String& deviceID, const String& deviceIP) {
    // Update pairing state
    isPaired = true;
    pairedDeviceID = deviceID;
    pairedDeviceIP = deviceIP;
    receiverIP = deviceIP;
    
    // Update display
    updateDisplayWithPairingInfo();
    
    Serial.println("Paired with sender: " + deviceID + " at " + deviceIP);
}

void WebServer::unpairDevice() {
    if (!isPaired) {
        Serial.println("No device paired to unpair");
        return;
    }
    
    Serial.println("Unpairing device: " + pairedDeviceID);
    
    // Clear pairing state
    isPaired = false;
    pairedDeviceID = "";
    pairedDeviceIP = "";
    receiverIP = "";
    
    // Update display
    updateDisplayWithPairingInfo();
    
    Serial.println("Device unpaired successfully");
}
