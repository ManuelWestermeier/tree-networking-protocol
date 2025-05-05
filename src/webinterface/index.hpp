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

#define WIFI_CRED_FILE "/wifi.txt"
#define CONN_FILE "/connections.txt"
#define AP_SUFFIX_FILE "/apsuffix.txt"
#define DEFAULT_AP_SSID "NodeAP"
#define WIFI_CONNECT_TIMEOUT_MS 10000

WebInterface::WebInterface(uint16_t port)
    : serverPort(port), server(port)
{
}

void WebInterface::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    // Load or generate persistent AP suffix
    loadAPSuffix();

    // Load Wi-Fi credentials and attempt connection
    loadCredentials();
    if (!wifiSSID.isEmpty() && !connectWiFi(wifiSSID, wifiPassword))
    {
        // Start AP with persistent suffix
        String fullSSID = String(DEFAULT_AP_SSID) + "_" + String(apSuffix);
        Serial.printf("Starting AP mode: %s\n", fullSSID.c_str());
        WiFi.softAP(fullSSID.c_str());
    }

    loadConnections();

    xTaskCreatePinnedToCore(
        webTask,
        "WebServerTask",
        8192,
        this,
        1,
        nullptr,
        1);
}

void WebInterface::loadAPSuffix()
{
    if (LittleFS.exists(AP_SUFFIX_FILE))
    {
        File f = LittleFS.open(AP_SUFFIX_FILE, "r+");
        apSuffix = f.readStringUntil('\n').toInt();
        f.close();
    }
    else
    {
        // Generate a random 16-bit suffix
        apSuffix = esp_random() & 0xFFFF;
        File f = LittleFS.open(AP_SUFFIX_FILE, "w+");
        if (f)
        {
            f.println(apSuffix);
            f.close();
        }
    }
}

void WebInterface::webTask(void *param)
{
    WebInterface *self = static_cast<WebInterface *>(param);

    self->loadCredentials();
    if (!self->wifiSSID.isEmpty() && !self->connectWiFi(self->wifiSSID, self->wifiPassword))
    {
        String fullSSID = String(DEFAULT_AP_SSID) + "_" + String(self->apSuffix);
        Serial.printf("Starting AP mode: %s\n", fullSSID.c_str());
        WiFi.softAP(fullSSID.c_str());
    }

    self->loadConnections();

    self->setupRoutes();
    self->server.begin();
    Serial.printf("Web server running on port %u\n", self->serverPort);

    while (true)
    {
        self->server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void WebInterface::setupRoutes()
{
    server.on("/", HTTP_GET, [&]()
              { handleRoot(); });
    server.on("/wifi", HTTP_GET, [&]()
              { handleWifi(); });
    server.on("/wifi/connect", HTTP_GET, [&]()
              { handleWifiConnect(); });
    server.on("/connections", HTTP_GET, [&]()
              { handleConnections(); });
    server.on("/connections/save", HTTP_POST, [&]()
              { handleConnectionsSave(); });
    server.onNotFound([&]()
                      { server.send(404, "text/plain", "Not Found"); });
}

void WebInterface::handleRoot()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        server.sendHeader("Location", "/connections");
        server.send(302, "text/plain", "");
    }
    else
    {
        server.sendHeader("Location", "/wifi");
        server.send(302, "text/plain", "");
    }
}

void WebInterface::handleWifi()
{
    int n = WiFi.scanNetworks(true);
    String options;
    for (int i = 0; i < n; ++i)
    {
        options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
    }

    static const char *page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
  <title>Wi-Fi Setup</title>
</head>
<body class="p-4">
  <div class="container">
    <h2>Wi-Fi Setup</h2>
    <form action="/wifi/connect" method="get">
      <div class="mb-3">
        <label class="form-label">SSID</label>
        <select name="ssid" class="form-select">")rawliteral";
    String html = String(page) + options + R"rawliteral(
        </select>
      </div>
      <div class="mb-3">
        <label class="form-label">Password</label>
        <input type="password" name="pass" class="form-control" required>
      </div>
      <button type="submit" class="btn btn-primary">Connect</button>
    </form>
  </div>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}

void WebInterface::handleWifiConnect()
{
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    if (connectWiFi(ssid, pass))
    {
        wifiSSID = ssid;
        wifiPassword = pass;
        saveCredentials();
        server.sendHeader("Location", "/connections");
        server.send(302, "text/plain", "");
    }
    else
    {
        server.send(200, "text/html", R"rawliteral(
<p>Connection failed. <a href='/wifi'>Try again</a></p>
)rawliteral");
    }
}

void WebInterface::handleConnections()
{
    // Scan available Wi-Fi networks and list them
    int n = WiFi.scanNetworks(true);
    String netList = "<ul class='list-group mb-3'>";
    for (int i = 0; i < n; ++i)
    {
        netList += "<li class='list-group-item'>" + WiFi.SSID(i) + "</li>";
    }
    netList += "</ul>";

    // Existing connections table rows
    String rows;
    for (auto &c : connections)
    {
        String addr;
        for (size_t i = 0; i < c.address.size(); ++i)
        {
            addr += String(c.address[i]) + (i + 1 < c.address.size() ? "," : "");
        }
        rows += "<tr>"
                "<td><input name='address[]' value='" +
                addr + "' class='form-control' required></td>"
                       "<td><input type='number' name='pin[]' value='" +
                String(c.pin) + "' class='form-control' required></td>"
                                "<td><button type='button' class='btn btn-danger' onclick='removeRow(this)'>X</button></td>"
                                "</tr>";
    }

    static const char *page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
  <title>Connection Manager</title>
</head>
<body class="p-4">
  <div class="container">
    <h2>Available Wi-Fi Networks</h2>
)rawliteral";
    String html = String(page) + netList + R"rawliteral(
    <h2>Connections</h2>
    <form action="/connections/save" method="post">
      <table class="table">
        <thead>
          <tr><th>Address</th><th>Pin</th><th></th></tr>
        </thead>
        <tbody>
)rawliteral" + rows +
                  R"rawliteral(
        </tbody>
      </table>
      <button type="button" class="btn btn-secondary mb-3" onclick="addRow()">Add</button>
      <button type="submit" class="btn btn-primary">Save</button>
    </form>
  </div>
  <script>
    function addRow() {
      const tbl = document.querySelector('tbody');
      const row = document.createElement('tr');
      row.innerHTML = `
        <td><input name='address[]' class='form-control' required></td>
        <td><input type='number' name='pin[]' class='form-control' required></td>
        <td><button type='button' class='btn btn-danger' onclick='removeRow(this)'>X</button></td>
      `;
      tbl.appendChild(row);
    }
    function removeRow(btn) {
      btn.closest('tr').remove();
    }
  </script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}

void WebInterface::handleConnectionsSave()
{
    std::vector<String> addrs, pins;
    for (int i = 0; i < server.args(); ++i)
    {
        if (server.argName(i) == "address[]")
            addrs.push_back(server.arg(i));
        else if (server.argName(i) == "pin[]")
            pins.push_back(server.arg(i));
    }
    connections.clear();
    for (size_t i = 0; i < addrs.size() && i < pins.size(); ++i)
    {
        Connection c;
        std::istringstream ss(addrs[i].c_str());
        uint16_t v;
        while (ss >> v)
        {
            c.address.push_back(v);
            if (ss.peek() == ',')
                ss.ignore();
        }
        c.pin = static_cast<uint8_t>(pins[i].toInt());
        connections.push_back(c);
    }
    saveConnections();
    server.sendHeader("Location", "/connections");
    server.send(302, "text/plain", "");
}

bool WebInterface::connectWiFi(const String &ssid, const String &password)
{
    WiFi.begin(ssid.c_str(), password.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS)
    {
        delay(500);
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Connected to ");
        Serial.println(ssid);
        return true;
    }
    Serial.print("Failed to connect to ");
    Serial.println(ssid);
    return false;
}

void WebInterface::loadCredentials()
{
    if (LittleFS.exists(WIFI_CRED_FILE))
    {
        File f = LittleFS.open(WIFI_CRED_FILE, "r+");
        wifiSSID = f.readStringUntil('\n');
        wifiPassword = f.readStringUntil('\n');
        f.close();
    }
    else
    {
        File f = LittleFS.open(WIFI_CRED_FILE, "w+");
        f.close(); // Datei nur erzeugen
    }
}

void WebInterface::saveCredentials()
{
    File f = LittleFS.open(WIFI_CRED_FILE, "w+");
    f.println(wifiSSID);
    f.println(wifiPassword);
    f.close();
}

void WebInterface::loadConnections()
{
    connections.clear();
    if (!LittleFS.exists(CONN_FILE))
        return;
    File f = LittleFS.open(CONN_FILE, "r+");
    while (f.available())
    {
        String line = f.readStringUntil('\n');
        int sep = line.indexOf(':');
        if (sep < 0)
            continue;
        String addrStr = line.substring(0, sep);
        String pinStr = line.substring(sep + 1);
        Connection c;
        std::istringstream ss(addrStr.c_str());
        uint16_t v;
        while (ss >> v)
        {
            c.address.push_back(v);
            if (ss.peek() == ',')
                ss.ignore();
        }
        c.pin = static_cast<uint8_t>(pinStr.toInt());
        connections.push_back(c);
    }
    f.close();
}

void WebInterface::saveConnections()
{
    File f = LittleFS.open(CONN_FILE, "w+");
    for (auto &c : connections)
    {
        for (size_t i = 0; i < c.address.size(); ++i)
        {
            f.print(c.address[i]);
            if (i + 1 < c.address.size())
                f.print(',');
        }
        f.print(':');
        f.println(c.pin);
    }
    f.close();
}
