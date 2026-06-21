#include "WebServer.h"

WebServer::WebServer(String& deviceID, OperationMode& currentMode, bool& ethernetConnected,
                     bool& jumperModeDetected, bool& isPaired, String& pairedDeviceID,
                     String& pairedDeviceIP, String& receiverIP,
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
    server.on("/status", HTTP_GET, std::bind(&WebServer::handleStatus, this));
    server.on("/discovery", HTTP_GET, std::bind(&WebServer::handleDiscovery, this));
    server.on("/log", HTTP_GET, std::bind(&WebServer::handleLog, this));
    server.on("/pairing", HTTP_ANY, std::bind(&WebServer::handlePairing, this));
    server.on("/scan_senders", HTTP_POST, std::bind(&WebServer::handleScanSenders, this));
    server.on("/pair_with_sender", HTTP_POST, std::bind(&WebServer::handlePairWithSender, this));
    server.on("/unpair_channel", HTTP_POST, std::bind(&WebServer::handleUnpairChannel, this));
    server.on("/command", HTTP_POST, std::bind(&WebServer::handleCommand, this));
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
    String html = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>)=====" + String(BOARD_NAME) + R"=====(</title>
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
        <h1>)=====" + String(BOARD_NAME) + R"=====(</h1>
        <div class="status">
            <h3>Device Status</h3>
            <p><strong>Device ID:</strong> )=====" + deviceID + R"=====(</p>
            <p><strong>Firmware:</strong> )=====" + FIRMWARE_VERSION + R"=====(</p>
            <p><strong>Mode:</strong> <span id="mode">)=====" + String(currentMode) + R"=====(</span>)=====" + String(jumperModeDetected ? " (Jumper Set)" : "") + R"=====(</p>
            <p><strong>Ethernet:</strong> <span id="ethernet">)=====" + String(ethernetConnected ? "Connected" : "Disconnected") + R"=====(</span></p>
            <p><strong>IP Address:</strong> )=====" + ETH.localIP().toString() + R"=====(</p>
            <p><strong>Subnet Mask:</strong> )=====" + ETH.subnetMask().toString() + R"=====(</p>
            <p><strong>Gateway:</strong> )=====" + ETH.gatewayIP().toString() + R"=====(</p>
            <p><strong>DNS Server:</strong> )=====" + ETH.dnsIP().toString() + R"=====(</p>
            <p><strong>Paired:</strong> <span id="paired-status">)=====" + String(isPaired ? "Yes" : "No") + R"=====(</span></p>
            <p><strong>Paired Device:</strong> <span id="paired-device">)=====" + (isPaired ? pairedDeviceID : "None") + R"=====(</span></p>
            <p><strong>Paired IP:</strong> <span id="paired-ip">)=====" + (isPaired ? pairedDeviceIP : "None") + R"=====(</span></p>
        </div>
        
        <div class="form-group">
            <h3>Configuration</h3>
            <p><strong>Mode:</strong> <span id="mode-display">)=====" + String(currentMode == MODE_DRY_CONTACT_SENDER ? "Dry Contact Sender" : currentMode == MODE_RELAY_RECEIVER ? "Relay Receiver" : "Not Configured") + R"=====(</span> (Set by jumper)</p>
            <br>
            <label for="receiverIP">Receiver IP (for Sender mode):</label>
            <input type="text" id="receiverIP" name="receiverIP" placeholder="192.168.1.100" value=")=====" + receiverIP + R"=====(">
            <br><br>
            <button type="button" class="button" onclick="saveReceiverIP()">Save Configuration</button>
        </div>
            <br>
            <div class="channel-naming">
                <h3>Channel Names</h3>
                <form id="channelNamesForm">
                    <label for="channel1Name">Channel 1 Name:</label>
                    <input type="text" id="channel1Name" name="channel1Name" maxlength="15" placeholder="Channel1" value=")=====" + channel1Name + R"=====(">
                    <small>Single word, max 15 characters</small>
                    <br><br>
                    <label for="channel2Name">Channel 2 Name:</label>
                    <input type="text" id="channel2Name" name="channel2Name" maxlength="15" placeholder="Channel2" value=")=====" + channel2Name + R"=====(">
                    <small>Single word, max 15 characters</small>
                    <br><br>
                    <button type="button" class="button" onclick="saveChannelNames()">Save Channel Names</button>
                </form>
            </div>
            <br>
            <div class="pairing-controls">
                <h3>Pairing Management</h3>
                <button type="button" class="button" onclick="toggleDiscovery()" id="discovery-btn">)=====" + String(discoveryEnabled ? "Disable Discovery" : "Enable Discovery") + R"=====(</button>
                <br><br>
                )=====" + String(currentMode == MODE_RELAY_RECEIVER && !isPaired ? "<button type=\"button\" class=\"button\" onclick=\"scanForSenders()\" style=\"background-color: #ff9800;\">Scan for Senders</button>" : "") + R"=====(
                )=====" + String(isPaired ? "<button type=\"button\" class=\"button\" onclick=\"unpairDevice()\" style=\"background-color: #f44336;\">Unpair Device</button>" : "") + R"=====(
            </div>
        </div>
        <br>
        <div class="log-controls">
            <h3>State Change Log</h3>
            <a href="/log" class="button">View Log</a>
        </div>
    </div>
    
    <script>
        setInterval(function() {
            fetch('/api')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('mode').textContent = data.mode_text || 'Unknown';
                    document.getElementById('ethernet').textContent = data.ethernet_connected ? 'Connected' : 'Disconnected';
                    document.getElementById('paired-status').textContent = data.is_paired ? 'Yes' : 'No';
                    document.getElementById('paired-device').textContent = data.paired_device_id || 'None';
                    document.getElementById('paired-ip').textContent = data.paired_device_ip || 'None';
                    document.getElementById('discovery-btn').textContent = data.discovery_enabled ? 'Disable Discovery' : 'Enable Discovery';
                })
                .catch(error => console.log('Error:', error));
        }, 2000);
        
        function saveReceiverIP() {
            const receiverIP = document.getElementById('receiverIP').value.trim();
            
            if (!receiverIP) {
                alert('Please enter a receiver IP address');
                return;
            }
            
            fetch('/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'receiverIP=' + encodeURIComponent(receiverIP)
            })
            .then(response => response.text())
            .then(data => {
                console.log('Configuration saved:', data);
                alert('Configuration saved successfully!');
            })
            .catch(error => {
                console.error('Error:', error);
                alert('Error saving configuration');
            });
        }
        
        function toggleDiscovery() {
            fetch('/api', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({action: 'toggle_discovery'})
            })
            .then(response => response.json())
            .then(data => {
                console.log('Discovery toggled:', data);
                location.reload();
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
                setTimeout(() => location.reload(), 3000);
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
                    location.reload();
                })
                .catch(error => console.log('Error:', error));
            }
        }
        
        function saveChannelNames() {
            const channel1Name = document.getElementById('channel1Name').value.trim();
            const channel2Name = document.getElementById('channel2Name').value.trim();
            
            if (!channel1Name || !channel2Name) {
                alert('Please enter names for both channels');
                return;
            }
            
            if (channel1Name.indexOf(' ') >= 0 || channel2Name.indexOf(' ') >= 0) {
                alert('Channel names must be single words (no spaces)');
                return;
            }
            
            if (channel1Name.length > 15 || channel2Name.length > 15) {
                alert('Channel names must be 15 characters or less');
                return;
            }
            
            fetch('/channel_names', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    channel1_name: channel1Name,
                    channel2_name: channel2Name
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    console.log('Channel names saved:', data);
                    alert('Channel names updated successfully!');
                } else {
                    console.error('Error saving channel names:', data);
                    alert('Error: ' + data.message);
                }
            })
            .catch(error => {
                console.error('Error:', error);
                alert('Error saving channel names');
            });
        }
    </script>
</body>
</html>
)=====";
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
        server.send(200, "text/plain", "Configuration saved");
    } else {
        JsonDocument doc;
        doc["receiver_ip"] = receiverIP;
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
        String html = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>State Change Log - )=====" + String(BOARD_NAME) + R"=====(</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        .log-entry { padding: 8px; margin: 4px 0; border-left: 3px solid #ddd; background: #f9f9f9; }
        .log-time { color: #666; font-size: 0.9em; margin-right: 10px; }
        .log-channel { background: #007bff; color: white; padding: 2px 6px; border-radius: 3px; font-size: 0.8em; margin-right: 8px; }
        .log-event { font-weight: bold; margin-right: 8px; }
        .log-event.dry_contact { color: #28a745; }
        .log-event.relay { color: #dc3545; }
        .log-event.pairing { color: #ffc107; }
        .log-event.system { color: #6c757d; }
        .log-device { color: #495057; margin-right: 8px; }
        .log-change { color: #007bff; margin-right: 8px; }
        .log-desc { color: #6c757d; }
        .header { text-align: center; margin-bottom: 30px; }
        .refresh-btn { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 10px; }
        .refresh-btn:hover { background: #0056b3; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>State Change Log</h1>
            <p>Device: )=====" + deviceID + R"=====(</p>
            <button class="refresh-btn" onclick="location.reload()">Refresh</button>
            <a href="/" class="refresh-btn">Back to Main</a>
        </div>
        <div class="log-container">
            )=====" + logHTML + R"=====(
        </div>
    </div>
</body>
</html>
)=====";
        server.send(200, "text/html", html);
    } else {
        server.send(500, "text/plain", "Logger not available");
    }
}

void WebServer::handlePairing() {
    String html = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Pairing Management - )=====" + String(BOARD_NAME) + R"=====(</title>
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
        <p>Device: )=====" + deviceID + R"=====(</p>
        <a href="/" class="btn">Back to Main</a>
        
        <div class="channel">
            <h3>Channel 1</h3>
            <p>Current pairing: <span id="ch1-status">)=====" + (isPaired ? pairedDeviceID : "Not paired") + R"=====(</span></p>
            <button class="btn" onclick="scanSenders(1) ">Scan for Senders</button>
            )=====" + (isPaired ? "<button class=\"btn danger\" onclick=\"unpairChannel(1)\">Unpair</button>" : "") + R"=====(
            <div id="ch1-senders" class="sender-list"></div>
        </div>
        
        <div class="channel">
            <h3>Channel 2</h3>
            <p>Current pairing: <span id="ch2-status">)=====" + (isPaired ? pairedDeviceID : "Not paired") + R"=====(</span></p>
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
</html>
)=====";
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

void WebServer::handleCommand() {
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        String clientIP = server.client().remoteIP().toString();
        
        Serial.println("Received command from " + clientIP + ": " + body);
        
        // Process the command using the main processIncomingMessage function
        extern void processIncomingMessage(const String& message, const String& senderIP);
        processIncomingMessage(body, clientIP);
        
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        server.send(405, "text/plain", "Method not allowed");
    }
}
