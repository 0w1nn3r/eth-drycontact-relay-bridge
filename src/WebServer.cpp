#include "WebServer.h"

WebServer::WebServer(String& deviceID, OperationMode& currentMode, bool& ethernetConnected,
                     bool& jumperModeDetected, bool& isPaired, String& pairedDeviceID,
                     String& pairedDeviceIP, String& receiverIP, bool& useTCP,
                     bool& discoveryEnabled, bool* dryContactState, bool* relayState,
                     String& channel1Name, String& channel2Name)
    : server(WEB_PORT),
      display(nullptr),
      stateLogger(nullptr),
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
      relayState(relayState),
      channel1Name(channel1Name),
      channel2Name(channel2Name) {
}

void WebServer::begin() {
    // Setup web server routes
    server.on("/", HTTP_GET, std::bind(&WebServer::handleRoot, this));
    server.on("/api", HTTP_ANY, std::bind(&WebServer::handleAPI, this));
    server.on("/config", HTTP_ANY, std::bind(&WebServer::handleConfig, this));
    server.on("/mode", HTTP_ANY, std::bind(&WebServer::handleMode, this));
    server.on("/status", HTTP_GET, std::bind(&WebServer::handleStatus, this));
    server.on("/discovery", HTTP_GET, std::bind(&WebServer::handleDiscovery, this));
    server.on("/log", HTTP_GET, std::bind(&WebServer::handleLog, this));
    server.on("/pairing", HTTP_ANY, std::bind(&WebServer::handlePairing, this));
    server.on("/scan_senders", HTTP_POST, std::bind(&WebServer::handleScanSenders, this));
    server.on("/pair_with_sender", HTTP_POST, std::bind(&WebServer::handlePairWithSender, this));
    server.on("/unpair_channel", HTTP_POST, std::bind(&WebServer::handleUnpairChannel, this));
    server.on("/channel_names", HTTP_ANY, std::bind(&WebServer::handleChannelNames, this));
    
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
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>" + String(BOARD_NAME) + "</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; }";
    html += ".status { background: #f0f0f0; padding: 10px; margin: 10px 0; border-radius: 5px; }";
    html += ".button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }";
    html += ".button:hover { background: #0056b3; }";
    html += ".form-group { margin: 10px 0; }";
    html += "label { display: block; margin-bottom: 5px; }";
    html += "input, select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>" + String(BOARD_NAME) + "</h1>";
    html += "<div class=\"status\">";
    html += "<h3>Device Status</h3>";
    html += "<p><strong>Device ID:</strong> " + deviceID + "</p>";
    html += "<p><strong>Firmware:</strong> " + String(FIRMWARE_VERSION) + "</p>";
    html += "<p><strong>Mode:</strong> <span id=\"mode\">" + String(currentMode) + "</span>" + String(jumperModeDetected ? " (Jumper Set)" : "") + "</p>";
    html += "<p><strong>Ethernet:</strong> <span id=\"ethernet\">" + String(ethernetConnected ? "Connected" : "Disconnected") + "</span></p>";
    html += "<p><strong>IP Address:</strong> " + ETH.localIP().toString() + "</p>";
    html += "<p><strong>Subnet Mask:</strong> " + ETH.subnetMask().toString() + "</p>";
    html += "<p><strong>Gateway:</strong> " + ETH.gatewayIP().toString() + "</p>";
    html += "<p><strong>DNS Server:</strong> " + ETH.dnsIP().toString() + "</p>";
    html += "<p><strong>Paired:</strong> <span id=\"paired-status\">" + String(isPaired ? "Yes" : "No") + "</span></p>";
    html += "<p><strong>Paired Device:</strong> <span id=\"paired-device\">" + (isPaired ? pairedDeviceID : "None") + "</span></p>";
    html += "<p><strong>Paired IP:</strong> <span id=\"paired-ip\">" + (isPaired ? pairedDeviceIP : "None") + "</span></p>";
    html += "</div>";
    
    html += "<div class=\"form-group\">";
    html += "<h3>Configuration</h3>";
    html += "<form method=\"POST\" action=\"/mode\">";
    html += "<label for=\"operationMode\">Operation Mode:</label>";
    html += "<select name=\"mode\" id=\"operationMode\">";
    html += "<option value=\"0\">Not Configured</option>";
    html += "<option value=\"1\">Dry Contact Sender</option>";
    html += "<option value=\"2\">Relay Receiver</option>";
    html += "</select>";
    html += "<br><br>";
    html += "<label for=\"receiverIP\">Receiver IP (for Sender mode):</label>";
    html += "<input type=\"text\" id=\"receiverIP\" name=\"receiverIP\" placeholder=\"192.168.1.100\" value=\"" + receiverIP + "\">";
    html += "<br><br>";
    html += "<label>";
    html += "<input type=\"checkbox\" name=\"useTCP\" " + String(useTCP ? "checked" : "") + ">";
    html += "Use TCP instead of UDP";
    html += "</label>";
    html += "<br><br>";
    html += "<button type=\"submit\" class=\"button\">Save Configuration</button>";
    html += "</form>";
    html += "<br>";
    html += "<div class=\"channel-naming\">";
    html += "<h3>Channel Names</h3>";
    html += "<form id=\"channelNamesForm\">";
    html += "<label for=\"channel1Name\">Channel 1 Name:</label>";
    html += "<input type=\"text\" id=\"channel1Name\" name=\"channel1Name\" maxlength=\"15\" placeholder=\"Channel1\" value=\"" + channel1Name + "\">";
    html += "<small>Single word, max 15 characters</small>";
    html += "<br><br>";
    html += "<label for=\"channel2Name\">Channel 2 Name:</label>";
    html += "<input type=\"text\" id=\"channel2Name\" name=\"channel2Name\" maxlength=\"15\" placeholder=\"Channel2\" value=\"" + channel2Name + "\">";
    html += "<small>Single word, max 15 characters</small>";
    html += "<br><br>";
    html += "<button type=\"button\" class=\"button\" onclick=\"saveChannelNames()\">Save Channel Names</button>";
    html += "</form>";
    html += "</div>";
    html += "<br>";
    html += "<div class=\"pairing-controls\">";
    html += "<h3>Pairing Management</h3>";
    html += "<button type=\"button\" class=\"button\" onclick=\"toggleDiscovery()\" id=\"discovery-btn\">" + String(discoveryEnabled ? "Disable Discovery" : "Enable Discovery") + "</button>";
    html += "<br><br>";
    html += String(currentMode == MODE_RELAY_RECEIVER && !isPaired ? "<button type=\"button\" class=\"button\" onclick=\"scanForSenders()\" style=\"background-color: #ff9800;\">Scan for Senders</button>" : "");
    html += String(isPaired ? "<button type=\"button\" class=\"button\" onclick=\"unpairDevice()\" style=\"background-color: #f44336;\">Unpair Device</button>" : "");
    html += "</div>";
    html += "</div>";
    html += "<br>";
    html += "<div class=\"log-controls\">";
    html += "<h3>State Change Log</h3>";
    html += "<a href=\"/log\" class=\"button\">View Log</a>";
    html += "</div>";
    html += "</div>";
    
    html += "<script>";
    html += "setInterval(function() {";
    html += "fetch('/api')";
    html += ".then(response => response.json())";
    html += ".then(data => {";
    html += "document.getElementById('mode').textContent = data.mode_text || 'Unknown';";
    html += "document.getElementById('ethernet').textContent = data.ethernet_connected ? 'Connected' : 'Disconnected';";
    html += "document.getElementById('paired-status').textContent = data.is_paired ? 'Yes' : 'No';";
    html += "document.getElementById('paired-device').textContent = data.paired_device_id || 'None';";
    html += "document.getElementById('paired-ip').textContent = data.paired_device_ip || 'None';";
    html += "document.getElementById('discovery-btn').textContent = data.discovery_enabled ? 'Disable Discovery' : 'Enable Discovery';";
    html += "})";
    html += ".catch(error => console.log('Error:', error));";
    html += "}, 2000);";
    html += "function toggleDiscovery() {";
    html += "fetch('/api', {";
    html += "method: 'POST',";
    html += "headers: {'Content-Type': 'application/json'},";
    html += "body: JSON.stringify({action: 'toggle_discovery'})";
    html += "})";
    html += ".then(response => response.json())";
    html += ".then(data => {";
    html += "console.log('Discovery toggled:', data);";
    html += "location.reload();";
    html += "})";
    html += ".catch(error => console.log('Error:', error));";
    html += "}";
    html += "function scanForSenders() {";
    html += "document.getElementById('paired-device').textContent = 'Scanning...';";
    html += "fetch('/api', {";
    html += "method: 'POST',";
    html += "headers: {'Content-Type': 'application/json'},";
    html += "body: JSON.stringify({action: 'scan_senders'})";
    html += "})";
    html += ".then(response => response.json())";
    html += ".then(data => {";
    html += "console.log('Scan completed:', data);";
    html += "setTimeout(() => location.reload(), 3000);";
    html += "})";
    html += ".catch(error => console.log('Error:', error));";
    html += "}";
    html += "function unpairDevice() {";
    html += "if (confirm('Are you sure you want to unpair this device?')) {";
    html += "fetch('/api', {";
    html += "method: 'POST',";
    html += "headers: {'Content-Type': 'application/json'},";
    html += "body: JSON.stringify({action: 'unpair_device'})";
    html += "})";
    html += ".then(response => response.json())";
    html += ".then(data => {";
    html += "console.log('Device unpaired:', data);";
    html += "location.reload();";
    html += "})";
    html += ".catch(error => console.log('Error:', error));";
    html += "}";
    html += "}";
    html += "function saveChannelNames() {";
    html += "const channel1Name = document.getElementById('channel1Name').value.trim();";
    html += "const channel2Name = document.getElementById('channel2Name').value.trim();";
    html += "if (!channel1Name || !channel2Name) {";
    html += "alert('Please enter names for both channels');";
    html += "return;";
    html += "}";
    html += "if (channel1Name.indexOf(' ') >= 0 || channel2Name.indexOf(' ') >= 0) {";
    html += "alert('Channel names must be single words (no spaces)');";
    html += "return;";
    html += "}";
    html += "if (channel1Name.length > 15 || channel2Name.length > 15) {";
    html += "alert('Channel names must be 15 characters or less');";
    html += "return;";
    html += "}";
    html += "fetch('/channel_names', {";
    html += "method: 'POST',";
    html += "headers: {'Content-Type': 'application/json'},";
    html += "body: JSON.stringify({";
    html += "channel1_name: channel1Name,";
    html += "channel2_name: channel2Name";
    html += "})";
    html += "})";
    html += ".then(response => response.json())";
    html += ".then(data => {";
    html += "if (data.status === 'success') {";
    html += "console.log('Channel names saved:', data);";
    html += "alert('Channel names updated successfully!');";
    html += "} else {";
    html += "console.error('Error saving channel names:', data);";
    html += "alert('Error: ' + data.message);";
    html += "}";
    html += "})";
    html += ".catch(error => {";
    html += "console.error('Error:', error);";
    html += "alert('Error saving channel names');";
    html += "});";
    html += "}";
    html += "</script>";
    html += "</body></html>";
    
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
        doc["dry_contact_state"] = JsonArray();
        JsonArray dcArray = doc["dry_contact_state"].to<JsonArray>();
        dcArray.add(dryContactState[0]);
        dcArray.add(dryContactState[1]);
        doc["receiver_ip"] = receiverIP;
        doc["use_tcp"] = useTCP;
    } else if (currentMode == MODE_RELAY_RECEIVER) {
        doc["relay_state"] = JsonArray();
        JsonArray relayArray = doc["relay_state"].to<JsonArray>();
        relayArray.add(relayState[0]);
        relayArray.add(relayState[1]);
    }
    
    // Add pairing information
    doc["is_paired"] = isPaired;
    doc["paired_device_id"] = pairedDeviceID;
    doc["paired_device_ip"] = pairedDeviceIP;
    doc["discovery_enabled"] = discoveryEnabled;
    
    // Add channel names
    doc["channel_names"] = JsonArray();
    JsonArray namesArray = doc["channel_names"].to<JsonArray>();
    namesArray.add(channel1Name);
    namesArray.add(channel2Name);
    
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

void WebServer::handleLog() {
    if (stateLogger) {
        String logHTML = stateLogger->getRecentLogHTML(50);
        String html = "<!DOCTYPE html><html><head>";
        html += "<title>State Change Log - " + String(BOARD_NAME) + "</title>";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        html += "<style>";
        html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }";
        html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }";
        html += ".log-entry { padding: 8px; margin: 4px 0; border-left: 3px solid #ddd; background: #f9f9f9; }";
        html += ".log-time { color: #666; font-size: 0.9em; margin-right: 10px; }";
        html += ".log-channel { background: #007bff; color: white; padding: 2px 6px; border-radius: 3px; font-size: 0.8em; margin-right: 8px; }";
        html += ".log-event { font-weight: bold; margin-right: 8px; }";
        html += ".log-event.dry_contact { color: #28a745; }";
        html += ".log-event.relay { color: #dc3545; }";
        html += ".log-event.pairing { color: #ffc107; }";
        html += ".log-event.system { color: #6c757d; }";
        html += ".log-device { color: #495057; margin-right: 8px; }";
        html += ".log-change { color: #007bff; margin-right: 8px; }";
        html += ".log-desc { color: #6c757d; }";
        html += ".header { text-align: center; margin-bottom: 30px; }";
        html += ".refresh-btn { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 10px; }";
        html += ".refresh-btn:hover { background: #0056b3; }";
        html += "</style></head><body>";
        html += "<div class=\"container\">";
        html += "<div class=\"header\">";
        html += "<h1>State Change Log</h1>";
        html += "<p>Device: " + deviceID + "</p>";
        html += "<button class=\"refresh-btn\" onclick=\"location.reload()\">Refresh</button>";
        html += "<a href=\"/\" class=\"refresh-btn\">Back to Main</a>";
        html += "</div>";
        html += "<div class=\"log-container\">";
        html += logHTML;
        html += "</div>";
        html += "</div>";
        html += "</body></html>";
        server.send(200, "text/html", html);
    } else {
        server.send(500, "text/plain", "Logger not available");
    }
}

void WebServer::handlePairing() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Pairing Management - )" + String(BOARD_NAME) + R"(</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        .channel { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        .channel h3 { margin-top: 0; color: #333; }
        .sender-list { margin: 10px 0; }
        .sender-item { padding: 10px; margin: 5px 0; background: #f8f9fa; border-radius: 4px; cursor: pointer; }
        .sender-item:hover { background: #e9ecef; }
        .sender-item.paired { background: #d4edda; border: 1px solid #c3e6cb; }
        .btn { background: #007bff; color: white; padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }
        .btn:hover { background: #0056b3; }
        .btn.danger { background: #dc3545; }
        .btn.danger:hover { background: #c82333; }
        .btn.success { background: #28a745; }
        .btn.success:hover { background: #218838; }
        .status { padding: 10px; margin: 10px 0; border-radius: 4px; }
        .status.scanning { background: #fff3cd; border: 1px solid #ffeaa7; }
        .status.error { background: #f8d7da; border: 1px solid #f5c6cb; }
        .status.success { background: #d4edda; border: 1px solid #c3e6cb; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Pairing Management</h1>
        <p>Device: )" + deviceID + R"(</p>
        <a href="/" class="btn">Back to Main</a>
        
        <div class="channel">
            <h3>Channel 1</h3>
            <p>Current pairing: <span id="ch1-status">)" + (isPaired ? pairedDeviceID : "Not paired") + R"(</span></p>
            <button class="btn" onclick="scanSenders(1) ">Scan for Senders</button>
            )" + (isPaired ? "<button class=\"btn danger\" onclick=\"unpairChannel(1)\">Unpair</button>" : "") + R"(
            <div id="ch1-senders" class="sender-list"></div>
        </div>
        
        <div class="channel">
            <h3>Channel 2</h3>
            <p>Current pairing: <span id="ch2-status">)" + (isPaired ? pairedDeviceID : "Not paired") + R"(</span></p>
            <button class="btn" onclick="scanSenders(2) ">Scan for Senders</button>
            <button class="btn danger" onclick="unpairChannel(2) ">Unpair</button>
            <div id="ch2-senders" class="sender-list"></div>
        </div>
        
        <div id="status-message"></div>
    </div>
    
    <script>
        function scanSenders(channel) {
            const statusDiv = document.getElementById('status-message');
            statusDiv.innerHTML = '<div class="status scanning">Scanning for senders...</div>';
            
            fetch('/scan_senders', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({channel: channel})
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    displaySenders(channel, data.senders || []);
                    statusDiv.innerHTML = '<div class="status success">Scan completed</div>';
                } else {
                    statusDiv.innerHTML = '<div class="status error">Scan failed</div>';
                }
            })
            .catch(error => {
                statusDiv.innerHTML = '<div class="status error">Error: ' + error.message + '</div>';
            });
        }
        
        function displaySenders(channel, senders) {
            const sendersDiv = document.getElementById('ch' + channel + '-senders');
            sendersDiv.innerHTML = '';
            
            senders.forEach(sender => {
                const div = document.createElement('div');
                div.className = 'sender-item';
                div.innerHTML = '<strong>' + sender.device_id + '</strong><br>IP: ' + sender.ip_address + '<br>Mode: ' + sender.mode_text + '<br><button class="btn success" onclick="pairWithSender(' + channel + ', \'' + sender.device_id + '\', \'' + sender.ip_address + '\') ">Pair</button>';
                sendersDiv.appendChild(div);
            });
        }
        
        function pairWithSender(channel, deviceId, ipAddress) {
            const statusDiv = document.getElementById('status-message');
            statusDiv.innerHTML = '<div class="status scanning">Pairing...</div>';
            
            fetch('/pair_with_sender', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    channel: channel,
                    device_id: deviceId,
                    ip_address: ipAddress
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    statusDiv.innerHTML = '<div class="status success">Paired successfully</div>';
                    document.getElementById('ch' + channel + '-status').textContent = deviceId;
                } else {
                    statusDiv.innerHTML = '<div class="status error">Pairing failed</div>';
                }
            })
            .catch(error => {
                statusDiv.innerHTML = '<div class="status error">Error: ' + error.message + '</div>';
            });
        }
        
        function unpairChannel(channel) {
            if (confirm('Are you sure you want to unpair this channel?')) {
                const statusDiv = document.getElementById('status-message');
                statusDiv.innerHTML = '<div class="status scanning">Unpairing...</div>';
                
                fetch('/unpair_channel', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({channel: channel})
                })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        statusDiv.innerHTML = '<div class="status success">Unpaired successfully</div>';
                        document.getElementById('ch' + channel + '-status').textContent = 'Not paired';
                    } else {
                        statusDiv.innerHTML = '<div class="status error">Unpairing failed</div>';
                    }
                })
                .catch(error => {
                    statusDiv.innerHTML = '<div class="status error">Error: ' + error.message + '</div>';
                });
            }
        }
    </script>
</body>
</html>)";
    server.send(200, "text/html", html);
}

void WebServer::handleScanSenders() {
    // This would scan for available senders and return them as JSON
    JsonDocument response;
    response["status"] = "success";
    response["senders"] = JsonArray();
    JsonArray senders = response["senders"].to<JsonArray>();
    
    // TODO: Implement actual sender discovery
    // For now, return empty array
    String resp;
    serializeJson(response, resp);
    server.send(200, "application/json", resp);
}

void WebServer::handlePairWithSender() {
    String body = server.arg("plain");
    JsonDocument request;
    DeserializationError error = deserializeJson(request, body);
    
    if (!error) {
        int channel = request["channel"] | 1;
        String deviceId = request["device_id"] | "";
        String ipAddress = request["ip_address"] | "";
        
        // TODO: Implement actual pairing logic
        if (stateLogger) {
            stateLogger->logPairingEvent("PAIR", deviceId, "Channel " + String(channel));
        }
        
        JsonDocument response;
        response["status"] = "success";
        response["message"] = "Paired with " + deviceId;
        String resp;
        serializeJson(response, resp);
        server.send(200, "application/json", resp);
    } else {
        server.send(400, "text/plain", "Invalid JSON");
    }
}

void WebServer::handleUnpairChannel() {
    String body = server.arg("plain");
    JsonDocument request;
    DeserializationError error = deserializeJson(request, body);
    
    if (!error) {
        int channel = request["channel"] | 1;
        
        // TODO: Implement actual unpairing logic
        if (stateLogger) {
            stateLogger->logPairingEvent("UNPAIR", "Channel " + String(channel), "Unpaired");
        }
        
        JsonDocument response;
        response["status"] = "success";
        response["message"] = "Channel " + String(channel) + " unpaired";
        String resp;
        serializeJson(response, resp);
        server.send(200, "application/json", resp);
    } else {
        server.send(400, "text/plain", "Invalid JSON");
    }
}

void WebServer::handleChannelNames() {
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        JsonDocument request;
        DeserializationError error = deserializeJson(request, body);
        
        if (!error) {
            String newChannel1Name = request["channel1_name"] | "";
            String newChannel2Name = request["channel2_name"] | "";
            
            // Validate channel names (one word, max length)
            newChannel1Name.trim();
            newChannel2Name.trim();
            
            bool valid1 = true, valid2 = true;
            
            if (newChannel1Name.length() > MAX_CHANNEL_NAME_LENGTH || 
                newChannel1Name.indexOf(' ') >= 0 || 
                newChannel1Name.length() == 0) {
                valid1 = false;
                newChannel1Name = channel1Name; // Keep current value
            }
            
            if (newChannel2Name.length() > MAX_CHANNEL_NAME_LENGTH || 
                newChannel2Name.indexOf(' ') >= 0 || 
                newChannel2Name.length() == 0) {
                valid2 = false;
                newChannel2Name = channel2Name; // Keep current value
            }
            
            if (valid1 && valid2) {
                channel1Name = newChannel1Name;
                channel2Name = newChannel2Name;
                
                // Update display
                if (display) {
                    display->setChannelNames(channel1Name, channel2Name);
                }
                
                // Log the change
                if (stateLogger) {
                    stateLogger->logSystemEvent("Channel names updated: " + channel1Name + ", " + channel2Name);
                }
                
                // Call save callback
                if (saveChannelNamesCallback) {
                    saveChannelNamesCallback();
                }
                
                JsonDocument response;
                response["status"] = "success";
                response["message"] = "Channel names updated";
                response["channel1_name"] = channel1Name;
                response["channel2_name"] = channel2Name;
                String resp;
                serializeJson(response, resp);
                server.send(200, "application/json", resp);
            } else {
                JsonDocument response;
                response["status"] = "error";
                response["message"] = "Invalid channel names. Use single words only, max " + String(MAX_CHANNEL_NAME_LENGTH) + " characters.";
                String resp;
                serializeJson(response, resp);
                server.send(400, "application/json", resp);
            }
        } else {
            server.send(400, "text/plain", "Invalid JSON");
        }
    } else {
        // GET request - return current channel names
        JsonDocument doc;
        doc["channel1_name"] = channel1Name;
        doc["channel2_name"] = channel2Name;
        doc["max_length"] = MAX_CHANNEL_NAME_LENGTH;
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}
