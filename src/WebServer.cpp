#include "WebServer.h"

WebServer::WebServer(String& deviceID, OperationMode& currentMode, bool& ethernetConnected,
                     bool& jumperModeDetected, bool& isPaired, String& pairedDeviceID,
                     String& pairedDeviceIP, String& receiverIP,
                     bool& discoveryEnabled, bool& peerOnline, UpsState& ups, UnifiState& unifi,
                     bool* dryContactState, bool* relayState,
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
      peerOnline(peerOnline),
      ups(ups),
      unifi(unifi),
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
    server.on("/command", HTTP_POST, std::bind(&WebServer::handleCommand, this));
    server.on("/channel_names", HTTP_ANY, std::bind(&WebServer::handleChannelNames, this));
    server.on("/ups_config", HTTP_ANY, std::bind(&WebServer::handleUpsConfig, this));
    server.on("/unifi", HTTP_ANY, std::bind(&WebServer::handleUnifi, this));
    server.on("/unifi_config", HTTP_ANY, std::bind(&WebServer::handleUnifiConfig, this));
    
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
    // UPS (SNMP) input config is only relevant in sender mode.
    String upsSection = "";
    if (currentMode == MODE_DRY_CONTACT_SENDER) {
        String status;
        if (ups.ip.length() == 0) {
            status = "Not configured";
        } else if (!ups.reachable) {
            status = "Unreachable";
        } else {
            status = "Reachable";
            if (ups.onBattery) status += ", ON BATTERY";
            if (ups.batteryLow) status += ", BATTERY LOW";
        }
        upsSection += "<br><div class=\"ups-config\">";
        upsSection += "<h3>UPS (SNMP) Input</h3>";
        upsSection += "<p>Poll a network UPS over SNMP and map its states to channels instead of the GPIO inputs.</p>";
        upsSection += "<label for=\"upsIP\">UPS IP address:</label>";
        upsSection += "<input type=\"text\" id=\"upsIP\" placeholder=\"192.168.1.20\" value=\"" + ups.ip + "\"><br><br>";
        upsSection += "<label for=\"upsCommunity\">SNMP community:</label>";
        upsSection += "<input type=\"text\" id=\"upsCommunity\" value=\"" + ups.community + "\"><br><br>";
        upsSection += "<label for=\"upsPort\">SNMP port:</label>";
        upsSection += "<input type=\"number\" id=\"upsPort\" value=\"" + String(ups.port) + "\"><br><br>";
        upsSection += "<label for=\"upsInterval\">Poll interval (seconds):</label>";
        upsSection += "<input type=\"number\" id=\"upsInterval\" min=\"1\" value=\"" + String(ups.pollIntervalSec) + "\"><br><br>";
        upsSection += "<label for=\"upsSrc0\">Channel 1 source:</label>";
        upsSection += "<select id=\"upsSrc0\">" + upsSourceOptions(ups.channelSource[0]) + "</select><br><br>";
        upsSection += "<label for=\"upsSrc1\">Channel 2 source:</label>";
        upsSection += "<select id=\"upsSrc1\">" + upsSourceOptions(ups.channelSource[1]) + "</select><br><br>";
        upsSection += "<button type=\"button\" class=\"button\" onclick=\"saveUpsConfig()\">Save UPS Config</button>";
        upsSection += "<p><strong>UPS status:</strong> <span id=\"ups-status\">" + status + "</span></p>";
        upsSection += "</div>";
        upsSection += "<br><div class=\"unifi-link\"><h3>UniFi WAN Input</h3>";
        upsSection += "<p>Map UniFi gateway WAN states (primary/secondary down, failover) to channels.</p>";
        upsSection += "<a href=\"/unifi\" class=\"button\">UniFi WAN Config</a></div>";
    }

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
            <p><strong>Peer Status:</strong> <span id="peer-status">)=====" + String(((currentMode == MODE_RELAY_RECEIVER && isPaired) || (currentMode == MODE_DRY_CONTACT_SENDER && receiverIP.length() > 0)) ? (peerOnline ? "Online" : "Offline") : "N/A") + R"=====(</span></p>
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
            )=====" + upsSection + R"=====(
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
                    document.getElementById('peer-status').textContent = data.has_peer ? (data.peer_online ? 'Online' : 'Offline') : 'N/A';
                    document.getElementById('discovery-btn').textContent = data.discovery_enabled ? 'Disable Discovery' : 'Enable Discovery';
                    var ue = document.getElementById('ups-status');
                    if (ue && data.ups) {
                        if (!data.ups.ip) { ue.textContent = 'Not configured'; }
                        else if (!data.ups.reachable) { ue.textContent = 'Unreachable'; }
                        else { ue.textContent = 'Reachable' + (data.ups.on_battery ? ', ON BATTERY' : '') + (data.ups.battery_low ? ', BATTERY LOW' : ''); }
                    }
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

        function saveUpsConfig() {
            const payload = {
                ip: document.getElementById('upsIP').value.trim(),
                community: document.getElementById('upsCommunity').value.trim(),
                port: parseInt(document.getElementById('upsPort').value) || 161,
                poll_interval: parseInt(document.getElementById('upsInterval').value) || 10,
                channel_source: [
                    parseInt(document.getElementById('upsSrc0').value),
                    parseInt(document.getElementById('upsSrc1').value)
                ]
            };
            fetch('/ups_config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(payload)
            })
            .then(response => response.json())
            .then(data => alert(data.message || 'UPS configuration saved'))
            .catch(error => alert('Error: ' + error.message));
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

    // Paired-peer liveness (from heartbeats)
    bool hasPeer = (currentMode == MODE_RELAY_RECEIVER && isPaired) ||
                   (currentMode == MODE_DRY_CONTACT_SENDER && receiverIP.length() > 0);
    doc["has_peer"] = hasPeer;
    doc["peer_online"] = peerOnline;

    // UPS (SNMP) configuration + status (sender mode)
    JsonObject upsObj = doc["ups"].to<JsonObject>();
    upsObj["ip"] = ups.ip;
    upsObj["community"] = ups.community;
    upsObj["port"] = ups.port;
    upsObj["poll_interval"] = ups.pollIntervalSec;
    upsObj["enabled"] = ups.usesUps() && ups.ip.length() > 0;
    upsObj["reachable"] = ups.reachable;
    upsObj["on_battery"] = ups.onBattery;
    upsObj["battery_low"] = ups.batteryLow;
    JsonArray srcArr = upsObj["channel_source"].to<JsonArray>();
    srcArr.add(ups.channelSource[0]);
    srcArr.add(ups.channelSource[1]);

    // UniFi (UniFi Network API) configuration + WAN status (sender mode)
    bool unifiUsed = false;
    for (int i = 0; i < 2; i++) {
        if (ups.channelSource[i] == SRC_UNIFI_PRIMARY_DOWN ||
            ups.channelSource[i] == SRC_UNIFI_SECONDARY_DOWN ||
            ups.channelSource[i] == SRC_UNIFI_FAILOVER) {
            unifiUsed = true;
        }
    }
    JsonObject unifiObj = doc["unifi"].to<JsonObject>();
    unifiObj["host"] = unifi.host;
    unifiObj["site"] = unifi.site;
    unifiObj["poll_interval"] = unifi.pollIntervalSec;
    unifiObj["enabled"] = unifiUsed && unifi.host.length() > 0;
    unifiObj["reachable"] = unifi.reachable;
    unifiObj["wan_status"] = unifi.wanStatus;
    unifiObj["primary_down"] = unifi.primaryDown;
    unifiObj["secondary_down"] = unifi.secondaryDown;
    unifiObj["failover"] = unifi.failover;
    
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
            <h3>Paired Sender</h3>
            <p>Pairing is device-to-device: a single paired sender drives both relay channels.</p>
            <p>Current pairing: <span id="pair-status">)=====" + (isPaired ? pairedDeviceID + " (" + pairedDeviceIP + ")" : "Not paired") + R"=====(</span></p>
            )=====" + String(currentMode == MODE_RELAY_RECEIVER && !isPaired ? "<button class=\"btn\" onclick=\"scanForSenders()\">Scan for Senders</button>" : "") + R"=====(
            )=====" + (isPaired ? "<button class=\"btn danger\" onclick=\"unpairDevice()\">Unpair Device</button>" : "") + R"=====(
        </div>

        <div id="status-message"></div>
    </div>

    <script>
        function scanForSenders() {
            const statusDiv = document.getElementById('status-message');
            statusDiv.innerHTML = '<div class="status scanning">Scanning for senders...</div>';

            fetch('/api', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({action: 'scan_senders'})
            })
            .then(response => response.json())
            .then(data => {
                statusDiv.innerHTML = '<div class="status success">Scan completed</div>';
                setTimeout(() => location.reload(), 3000);
            })
            .catch(error => {
                statusDiv.innerHTML = '<div class="status error">Error: ' + error.message + '</div>';
            });
        }

        function unpairDevice() {
            if (confirm('Are you sure you want to unpair this device?')) {
                const statusDiv = document.getElementById('status-message');
                statusDiv.innerHTML = '<div class="status scanning">Unpairing...</div>';

                fetch('/api', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({action: 'unpair_device'})
                })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        statusDiv.innerHTML = '<div class="status success">Unpaired successfully</div>';
                        document.getElementById('pair-status').textContent = 'Not paired';
                        setTimeout(() => location.reload(), 1000);
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

String WebServer::upsSourceOptions(int selected) {
    struct { int value; const char* label; } opts[] = {
        {SRC_GPIO,                 "GPIO input"},
        {SRC_UPS_ON_BATTERY,       "UPS: On battery"},
        {SRC_UPS_BATTERY_LOW,      "UPS: Battery low"},
        {SRC_UNIFI_PRIMARY_DOWN,   "UniFi: Primary WAN down"},
        {SRC_UNIFI_SECONDARY_DOWN, "UniFi: Secondary WAN down"},
        {SRC_UNIFI_FAILOVER,       "UniFi: Failover active"},
        {SRC_DISABLED,             "Disabled"},
    };
    String out;
    for (auto& o : opts) {
        out += "<option value=\"" + String(o.value) + "\"" +
               (o.value == selected ? " selected" : "") + ">" + o.label + "</option>";
    }
    return out;
}

void WebServer::handleUpsConfig() {
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        JsonDocument req;
        if (deserializeJson(req, body)) {
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }

        ups.ip = (const char*)(req["ip"] | "");
        ups.ip.trim();
        ups.community = (const char*)(req["community"] | "public");
        ups.community.trim();
        if (ups.community.length() == 0) ups.community = "public";

        int port = req["port"] | (int)SNMP_DEFAULT_PORT;
        ups.port = (port > 0 && port <= 65535) ? (uint16_t)port : SNMP_DEFAULT_PORT;

        int interval = req["poll_interval"] | (int)UPS_DEFAULT_POLL_INTERVAL_S;
        if (interval < 1) interval = 1;
        if (interval > 3600) interval = 3600;
        ups.pollIntervalSec = (uint16_t)interval;

        for (int i = 0; i < 2; i++) {
            int s = req["channel_source"][i] | (int)SRC_GPIO;
            if (s < SRC_GPIO || s > SRC_DISABLED) s = SRC_GPIO;
            ups.channelSource[i] = s;
        }

        if (saveUpsConfigCallback) {
            saveUpsConfigCallback();
        }

        JsonDocument response;
        response["status"] = "success";
        response["message"] = "UPS configuration saved";
        String resp;
        serializeJson(response, resp);
        server.send(200, "application/json", resp);
    } else {
        // GET - return current UPS config + status
        JsonDocument doc;
        doc["ip"] = ups.ip;
        doc["community"] = ups.community;
        doc["port"] = ups.port;
        doc["poll_interval"] = ups.pollIntervalSec;
        doc["reachable"] = ups.reachable;
        doc["on_battery"] = ups.onBattery;
        doc["battery_low"] = ups.batteryLow;
        JsonArray srcArr = doc["channel_source"].to<JsonArray>();
        srcArr.add(ups.channelSource[0]);
        srcArr.add(ups.channelSource[1]);
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}

void WebServer::handleUnifiConfig() {
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        JsonDocument req;
        if (deserializeJson(req, body)) {
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }

        unifi.host = (const char*)(req["host"] | "");
        unifi.host.trim();
        unifi.site = (const char*)(req["site"] | "default");
        unifi.site.trim();
        if (unifi.site.length() == 0) unifi.site = "default";

        // Only overwrite the API key when a new one is supplied (the form does
        // not echo the stored key back).
        String newKey = (const char*)(req["api_key"] | "");
        newKey.trim();
        if (newKey.length() > 0) {
            unifi.apiKey = newKey;
        }

        int interval = req["poll_interval"] | (int)UNIFI_DEFAULT_POLL_INTERVAL_S;
        if (interval < 1) interval = 1;
        if (interval > 3600) interval = 3600;
        unifi.pollIntervalSec = (uint16_t)interval;

        for (int i = 0; i < 2; i++) {
            int s = req["channel_source"][i] | ups.channelSource[i];
            if (s < SRC_GPIO || s > SRC_UNIFI_FAILOVER) s = SRC_GPIO;
            ups.channelSource[i] = s;
        }

        if (saveUnifiConfigCallback) saveUnifiConfigCallback();
        if (saveUpsConfigCallback) saveUpsConfigCallback();  // persists channel_source

        JsonDocument response;
        response["status"] = "success";
        response["message"] = "UniFi configuration saved";
        String resp;
        serializeJson(response, resp);
        server.send(200, "application/json", resp);
    } else {
        // GET - current config + status (API key is never returned)
        JsonDocument doc;
        doc["host"] = unifi.host;
        doc["site"] = unifi.site;
        doc["poll_interval"] = unifi.pollIntervalSec;
        doc["has_key"] = unifi.apiKey.length() > 0;
        doc["reachable"] = unifi.reachable;
        doc["wan_status"] = unifi.wanStatus;
        doc["primary_down"] = unifi.primaryDown;
        doc["secondary_down"] = unifi.secondaryDown;
        doc["failover"] = unifi.failover;
        JsonArray srcArr = doc["channel_source"].to<JsonArray>();
        srcArr.add(ups.channelSource[0]);
        srcArr.add(ups.channelSource[1]);
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}

void WebServer::handleUnifi() {
    String keyPlaceholder = unifi.apiKey.length() > 0 ? "(unchanged - leave blank to keep)" : "X-API-KEY";
    String html = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>UniFi WAN Monitoring - )=====" + String(BOARD_NAME) + R"=====(</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 700px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        h1 { color: #333; }
        label { display: block; margin: 10px 0 4px; font-weight: bold; }
        input, select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        .btn { background: #007bff; color: white; padding: 10px 18px; border: none; border-radius: 4px; cursor: pointer; margin-top: 14px; }
        .btn:hover { background: #0056b3; }
        .status { padding: 10px; margin: 12px 0; border-radius: 4px; background: #f0f0f0; }
        .muted { color: #666; font-size: 0.9em; }
    </style>
</head>
<body>
    <div class="container">
        <h1>UniFi WAN Monitoring</h1>
        <a href="/" class="btn">Back to Main</a>
        <p class="muted">Poll a UniFi gateway over HTTPS and map WAN states to channels. Generate an API key in the Network app under Settings &rarr; Control Plane / Integrations.</p>

        <label for="unifiHost">Gateway IP / host:</label>
        <input type="text" id="unifiHost" placeholder="192.168.1.1" value=")=====" + unifi.host + R"=====(">

        <label for="unifiKey">API key:</label>
        <input type="password" id="unifiKey" placeholder=")=====" + keyPlaceholder + R"=====(">

        <label for="unifiSite">Site:</label>
        <input type="text" id="unifiSite" value=")=====" + unifi.site + R"=====(">

        <label for="unifiInterval">Poll interval (seconds):</label>
        <input type="number" id="unifiInterval" min="1" value=")=====" + String(unifi.pollIntervalSec) + R"=====(">

        <label for="unifiSrc0">Channel 1 source:</label>
        <select id="unifiSrc0">)=====" + upsSourceOptions(ups.channelSource[0]) + R"=====(</select>

        <label for="unifiSrc1">Channel 2 source:</label>
        <select id="unifiSrc1">)=====" + upsSourceOptions(ups.channelSource[1]) + R"=====(</select>

        <button type="button" class="btn" onclick="saveUnifi()">Save UniFi Config</button>

        <div class="status">
            <strong>WAN status:</strong> <span id="unifi-status">checking...</span>
        </div>
        <div id="status-message"></div>
    </div>

    <script>
        function refreshStatus() {
            fetch('/api')
                .then(r => r.json())
                .then(d => {
                    var el = document.getElementById('unifi-status');
                    if (!d.unifi || !d.unifi.host) { el.textContent = 'Not configured'; return; }
                    if (!d.unifi.reachable) { el.textContent = 'Console unreachable'; return; }
                    var parts = ['wan=' + (d.unifi.wan_status || '?')];
                    if (d.unifi.primary_down) parts.push('PRIMARY DOWN');
                    if (d.unifi.secondary_down) parts.push('SECONDARY DOWN');
                    if (d.unifi.failover) parts.push('FAILOVER');
                    el.textContent = parts.join(', ');
                })
                .catch(e => {});
        }

        function saveUnifi() {
            const key = document.getElementById('unifiKey').value;
            const payload = {
                host: document.getElementById('unifiHost').value.trim(),
                site: document.getElementById('unifiSite').value.trim() || 'default',
                poll_interval: parseInt(document.getElementById('unifiInterval').value) || 15,
                channel_source: [
                    parseInt(document.getElementById('unifiSrc0').value),
                    parseInt(document.getElementById('unifiSrc1').value)
                ]
            };
            if (key.length > 0) payload.api_key = key;
            fetch('/unifi_config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(payload)
            })
            .then(r => r.json())
            .then(d => {
                document.getElementById('status-message').innerHTML =
                    '<div class="status">' + (d.message || 'Saved') + '</div>';
                document.getElementById('unifiKey').value = '';
            })
            .catch(e => {
                document.getElementById('status-message').innerHTML =
                    '<div class="status">Error: ' + e.message + '</div>';
            });
        }

        refreshStatus();
        setInterval(refreshStatus, 3000);
    </script>
</body>
</html>
)=====";
    server.send(200, "text/html", html);
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
