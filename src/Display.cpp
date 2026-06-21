#include "Display.h"

Display::Display() : display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET),
                     displayAvailable(false),
                     lastUpdateTime(0),
                     updateInterval(1000),
                     currentMode(MODE_UNDEFINED),
                     ethernetConnected(false) {
    inputState[0] = false;
    inputState[1] = false;
    outputState[0] = false;
    outputState[1] = false;
}

bool Display::init() {
    // Initialize I2C
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    
    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("SSD1306 allocation failed - continuing without display");
        displayAvailable = false;
        return false;
    }
    
    displayAvailable = true;
    
    // Set display properties
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    Serial.println("OLED display initialized successfully");
    showStartupScreen();
    
    return true;
}

void Display::showStartupScreen() {
    if (!displayAvailable) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ETH Relay Bridge");
    display.println("v" + String(FIRMWARE_VERSION));
    
    // Device ID
    display.setCursor(0, 20);
    display.println("ID: " + deviceID.substring(0, 12));
    
    // Status
    display.setCursor(0, 35);
    display.println("Initializing...");
    
    display.display();
    delay(2000);
}

void Display::showConnecting() {
    if (!displayAvailable) return;
    
    display.clearDisplay();
    
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ETH Relay Bridge");
    
    display.setCursor(0, 20);
    display.println("Connecting...");
    
    display.setCursor(0, 35);
    display.println("Ethernet...");
    
    display.display();
}

void Display::showConnected() {
    if (!displayAvailable) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ETH Relay Bridge");
    
    display.setCursor(0, 20);
    display.println("Connected!");
    
    display.setCursor(0, 35);
    display.println("IP: " + localIP);
    
    display.display();
    delay(2000);
}

void Display::update() {
    if (!displayAvailable) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < updateInterval) {
        return;
    }
    
    lastUpdateTime = currentTime;
    
    display.clearDisplay();
    
    // Draw all sections
    drawHeader();
    drawModeInfo();
    drawNetworkInfo();
    drawStateInfo();
    drawStatusBar();
    
    display.display();
}

void Display::drawHeader() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    
    // Title with mode icon
    String title = getModeIcon() + " " + getModeText();
    display.println(title);
    
    // Separator line
    display.drawLine(0, 12, OLED_WIDTH - 1, 12, SSD1306_WHITE);
}

void Display::drawModeInfo() {
    display.setTextSize(1);
    display.setCursor(0, 16);
    
    if (currentMode == MODE_DRY_CONTACT_SENDER) {
        display.println("CH1: " + String(inputState[0] ? "CLOSED" : "OPEN"));
        display.setCursor(0, 24);
        display.println("CH2: " + String(inputState[1] ? "CLOSED" : "OPEN"));
    } else if (currentMode == MODE_RELAY_RECEIVER) {
        display.println("CH1: " + String(outputState[0] ? "ON" : "OFF"));
        display.setCursor(0, 24);
        display.println("CH2: " + String(outputState[1] ? "ON" : "OFF"));
    } else {
        display.println("Mode: Not Set");
    }
}

void Display::drawNetworkInfo() {
    display.setTextSize(1);
    display.setCursor(0, 38);
    
    // Local IP (shortened for dual channel display)
    if (ethernetConnected && localIP.length() > 0) {
        display.println("IP: " + localIP.substring(0, 12));
    } else {
        display.println("IP: ---.---.---");
    }
    
    // Remote IP (for sender mode) - only show if space allows
    if (currentMode == MODE_DRY_CONTACT_SENDER && remoteIP.length() > 0) {
        display.setCursor(0, 46);
        display.println("R: " + remoteIP.substring(0, 12));
    }
}

void Display::drawStateInfo() {
    display.setTextSize(1);
    display.setCursor(0, 54);
    
    // Connection status (shortened)
    if (ethernetConnected) {
        display.println("Online");
    } else {
        display.println("Offline");
    }
}

void Display::drawStatusBar() {
    // Bottom status bar
    display.drawLine(0, OLED_HEIGHT - 9, OLED_WIDTH - 1, OLED_HEIGHT - 9, SSD1306_WHITE);
    
    display.setTextSize(1);
    display.setCursor(0, OLED_HEIGHT - 8);
    
    // Show device ID (shortened)
    String shortID = deviceID.length() > 8 ? deviceID.substring(0, 8) : deviceID;
    display.print(shortID);
    
    // Show connection indicator on the right
    String connIndicator = ethernetConnected ? "ETH" : "---";
    int textWidth = connIndicator.length() * 6; // Approximate width
    display.setCursor(OLED_WIDTH - textWidth - 2, OLED_HEIGHT - 8);
    display.print(connIndicator);
}

String Display::getModeText() {
    switch (currentMode) {
        case MODE_DRY_CONTACT_SENDER:
            return "Sender";
        case MODE_RELAY_RECEIVER:
            return "Receiver";
        default:
            return "Unknown";
    }
}

String Display::getModeIcon() {
    switch (currentMode) {
        case MODE_DRY_CONTACT_SENDER:
            return "S"; // Sender icon
        case MODE_RELAY_RECEIVER:
            return "R"; // Receiver icon
        default:
            return "?";
    }
}
