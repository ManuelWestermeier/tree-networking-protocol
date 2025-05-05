#pragma once

#include <esp_system.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <vector>
#include <sstream>

#include "../protocoll/index.hpp"

class WebInterface
{
public:
    explicit WebInterface(uint16_t port = 80);
    void begin();

    // Retrieve saved connection configurations
    const std::vector<Connection> &getConnections() const { return connections; }

private:
    // FreeRTOS task entry point for handling web requests
    static void webTask(void *param);

    // Route setup and handlers
    void setupRoutes();
    void handleRoot();
    void handleWifi();
    void handleWifiConnect();
    void handleConnections();
    void handleConnectionsSave();

    // Wi-Fi and storage utilities
    bool connectWiFi(const String &ssid, const String &password);
    void loadCredentials();
    void saveCredentials();
    void loadConnections();
    void saveConnections();
    void loadAPSuffix();

    // Underlying HTTP server
    WebServer server;

    // Stored Wi-Fi credentials
    String wifiSSID;
    String wifiPassword;

    // Persistent random suffix for AP name
    uint16_t apSuffix = 0;

    // Saved network connections (addresses and pins)
    std::vector<Connection> connections;

    // TCP port for HTTP server
    uint16_t serverPort;
};
