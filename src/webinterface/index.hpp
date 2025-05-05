#pragma once

#include <Arduino.h>
#include <esp_system.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <vector>
#include <sstream>
#include "../protocoll/index.hpp"

#define WIFI_CRED_FILE "/wifi.txt"
#define CONN_FILE "/connections.txt"
#define AP_SUFFIX_FILE "/apsuffix.txt"
#define DEFAULT_AP_SSID "NodeAP"
#define WIFI_CONNECT_TIMEOUT_MS 10000

class WebInterface
{
public:
    explicit WebInterface(uint16_t port = 80)
        : server(port), serverPort(port), apSuffix(0) {}

    void begin()
    {
        Serial.println("[Web] begin");
        Serial.println("[Web] mounting LittleFS...");
        if (!LittleFS.begin())
        {
            Serial.println("[Web] mount failed, formatting...");
            if (!LittleFS.format() || !LittleFS.begin())
            {
                Serial.println("[Web] FS init failed");
                return;
            }
        }
        Serial.println("[Web] FS ready");

        loadAPSuffix();
        loadCredentials();

        Serial.printf("[Web] creds SSID='%s' passlen=%u\n", wifiSSID.c_str(), wifiPassword.length());
        bool ok = false;
        if (!wifiSSID.isEmpty())
        {
            Serial.println("[Web] connect STA");
            WiFi.mode(WIFI_STA);
            ok = connectWiFi(wifiSSID, wifiPassword);
        }
        if (!ok)
        {
            Serial.println("[Web] start AP");
            WiFi.mode(WIFI_AP_STA);
            String ss = String(DEFAULT_AP_SSID) + "_" + apSuffix;
            Serial.printf("[Web] AP SSID=%s\n", ss.c_str());
            WiFi.softAP(ss.c_str());
        }

        loadConnections();
        Serial.printf("[Web] loaded %u physikalNode.logicalNode.connections\n", physikalNode.logicalNode.connections.size());

        Serial.println("[Web] spawning task");
        xTaskCreatePinnedToCore(
            webTask, "WebServer", 8192, this, 1, nullptr, 1);
    }

private:
    WebServer server;
    uint16_t serverPort;
    String wifiSSID, wifiPassword;
    uint16_t apSuffix;
    // std::vector<Connection> physikalNode.logicalNode.connections;
    PhysikalNode physikalNode;

    static void webTask(void *p)
    {
        auto *self = static_cast<WebInterface *>(p);
        Serial.println("[WebTask] setupRoutes");
        self->setupRoutes();
        Serial.println("[WebTask] server.begin");
        self->server.begin();
        Serial.printf("[WebTask] running on port %u\n", self->serverPort);

        Serial.println("[WebTask] started");
        Serial.println("Server URL: " + self->getServerURL());

        while (true)
        {
            self->server.handleClient();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    void setupRoutes()
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

        server.on("/favicon.ico", HTTP_GET, [&]()
                  { server.send(204, "text/plain", ""); });
    }

    // Helper to build server URL
    String getServerURL()
    {
        IPAddress ip = WiFi.localIP();
        String addr = ip.toString();
        if (serverPort != 80)
            addr += ":" + String(serverPort);
        String out = String("http://") + addr;
        if (out == "http://0.0.0.0")
            return String("http://192.168.4.1/");
        return out;
    }

    void handleConnectionsSave()
    {
        Serial.println("[Web] handleConnectionsSave");
        String ownAddr;
        std::vector<String> addrs, pins;

        // Process parameters
        for (int i = 0; i < server.args(); ++i)
        {
            if (server.argName(i) == "ownAddr")
            {
                ownAddr = server.arg(i);
            }
            else if (server.argName(i) == "address[]")
            {
                addrs.push_back(server.arg(i));
            }
            else if (server.argName(i) == "pin[]")
            {
                pins.push_back(server.arg(i));
            }
        }

        // Update Own Address
        if (!ownAddr.isEmpty())
        {
            Address newAddr;
            std::istringstream ss(ownAddr.c_str());
            uint16_t v;
            while (ss >> v)
            {
                newAddr.push_back(v);
                if (ss.peek() == ',')
                    ss.ignore();
            }
            physikalNode.logicalNode.you = newAddr;
        }

        // Update Connections
        physikalNode.logicalNode.connections.clear();
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
            c.pin = uint8_t(pins[i].toInt());
            physikalNode.logicalNode.connections.push_back(c);
        }

        saveConnections();
        server.sendHeader("Location", "/connections");
        server.send(302, "text/plain", "");
    }

    void handleRoot()
    {
        Serial.println("[Web] handleRoot");
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

    void handleWifi()
    {
        Serial.println("[Web] handleWifi: scan");
        int n = WiFi.scanNetworks();
        String opts;
        for (int i = 0; i < n; ++i)
        {
            opts += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        }
        String page = R"rawl(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Wi-Fi Setup</title>
  <style>
    body { font-family: sans-serif; background: #f4f6f8; margin: 0; padding: 0; }
    .navbar { background: #007bff; padding: 1em; color: white; display: flex; justify-content: space-between; }
    .container { padding: 2em; max-width: 600px; margin: auto; }
    .card { background: white; border-radius: 8px; padding: 1.5em; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
    label { display: block; margin-top: 1em; }
    input, select { width: 100%; padding: 0.5em; margin-top: 0.5em; }
    button { margin-top: 1.5em; padding: 0.7em 1.5em; background: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer; }
    button:hover { background: #218838; }
  </style>
</head>
<body>
  <div class="navbar">
    <div>Node Web UI</div>
    <div>%SERVER_URL%</div>
  </div>
  <div class="container">
    <div class="card">
      <h2>Connect to Wi-Fi</h2>
      <form action="/wifi/connect" method="get">
        <label>SSID</label>
        <select name="ssid">%OPTIONS%</select>
        <label>Password</label>
        <input type="password" name="pass" required>
        <button type="submit">Connect</button>
      </form>
    </div>
  </div>
</body>
</html>
)rawl";
        page.replace("%OPTIONS%", opts);
        page.replace("%SERVER_URL%", getServerURL());
        server.send(200, "text/html", page);
    }

    void handleWifiConnect()
    {
        Serial.println("[Web] handleWifiConnect");
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
            server.send(200, "text/html", R"rawf(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
<title>Connection Failed</title>
  <style>
    body { font-family: sans-serif; background: #f4f6f8; margin: 0; padding: 0; }
    .navbar { background: #007bff; padding: 1em; color: white; display: flex; justify-content: space-between; }
    .container { padding: 2em; max-width: 600px; margin: auto; }
    .card { background: white; border-radius: 8px; padding: 1.5em; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
    label { display: block; margin-top: 1em; }
    input, select { width: 100%; padding: 0.5em; margin-top: 0.5em; }
    button { margin-top: 1.5em; padding: 0.7em 1.5em; background: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer; }
    button:hover { background: #218838; }
  </style>
</head>
<body class="bg-light p-4">
<div class="alert alert-danger">Failed to connect to Wi-Fi. <a href="/wifi" class="alert-link">Try again</a>.</div>
</body>
</html>
)rawf");
        }
    }

    void handleConnections()
    {
        Serial.println("[Web] handleConnections: scan");

        // Build Own Address section
        String ownAddrStr;
        for (size_t i = 0; i < physikalNode.logicalNode.you.size(); ++i)
        {
            ownAddrStr += String(physikalNode.logicalNode.you[i]);
            if (i + 1 < physikalNode.logicalNode.you.size())
            {
                ownAddrStr += ",";
            }
        }

        // Build connection rows
        String connectionRows;
        for (auto &c : physikalNode.logicalNode.connections)
        {
            String a;
            for (size_t i = 0; i < c.address.size(); ++i)
            {
                a += String(c.address[i]);
                if (i + 1 < c.address.size())
                {
                    a += ",";
                }
            }
            connectionRows += R"(
                <tr>
                    <td><input name='address[]' value=')" +
                              a + R"(' class='form-input'></td>
                    <td><input type='number' name='pin[]' value=')" +
                              String(c.pin) + R"(' class='form-input'></td>
                    <td><button type='button' onclick='removeRow(this)' class='btn-danger btn'>Remove</button></td>
                </tr>)";
        }

        String page = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Node Configuration</title>
    <style>
        :root {
            --primary: #007bff;
            --success: #28a745;
            --danger: #dc3545;
            --background: #f8f9fa;
            --card-bg: #ffffff;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            margin: 0;
            padding: 0;
            background-color: var(--background);
        }
        
        .navbar {
            background-color: var(--primary);
            color: white;
            padding: 1rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        
        .container {
            max-width: 800px;
            margin: 2rem auto;
            padding: 0 1rem;
        }
        
        .config-card {
            background: var(--card-bg);
            border-radius: 8px;
            padding: 2rem;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        
        .form-group {
            margin-bottom: 1.5rem;
        }
        
        .form-label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: 500;
        }
        
        .form-input {
            width: 100%;
            padding: 0.5rem;
            border: 1px solid #ced4da;
            border-radius: 4px;
            box-sizing: border-box;
        }
        
        .btn {
            padding: 0.5rem 1rem;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-weight: 500;
        }
        
        .btn-success {
            background-color: var(--success);
            color: white;
        }
        
        .btn-danger {
            background-color: var(--danger);
            color: white;
        }
        
        .btn-outline {
            background: transparent;
            border: 1px solid #ced4da;
            color: #495057;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 1.5rem 0;
        }
        
        th, td {
            padding: 0.75rem;
            text-align: left;
            border-bottom: 1px solid #dee2e6;
        }
        
        th {
            background-color: var(--background);
            font-weight: 500;
        }
        
        .help-text {
            color: #6c757d;
            font-size: 0.9rem;
            margin-top: 0.5rem;
        }
    </style>
</head>
<body>
    <nav class="navbar">
        <div>Node Configuration</div>
        <div>%SERVER_URL%</div>
    </nav>
    
    <div class="container">
        <div class="config-card">
            <form action="/connections/save" method="post">
                <div class="form-group">
                    <label class="form-label">Own Address</label>
                    <input type="text" name="ownAddr" value="%OWN_ADDR%" class="form-input">
                    <div class="help-text">
                        Enter your node's address as comma-separated numbers (e.g., "1,2,3")<br>
                        Each number represents a level in the network hierarchy
                    </div>
                </div>

                <div class="form-group">
                    <label class="form-label">Connections</label>
                    <table>
                        <thead>
                            <tr>
                                <th>Address</th>
                                <th>Pin</th>
                                <th>Actions</th>
                            </tr>
                        </thead>
                        <tbody>
                            %CONNECTION_ROWS%
                        </tbody>
                    </table>
                    <button type="button" onclick="addRow() " class
            = "btn btn-outline" > Add Connection</ button>
              </ div>

              <button type = "submit" class = "btn btn-success" style="margin-left: 5px;"> Save Configuration</ button>
              </ form>
              </ div>
              </ div>

              <script>
                  function addRow()
        {
            const tbody = document.querySelector('tbody');
            const row = document.createElement('tr');
            row.innerHTML = ` <td><input name = "address[]" class = "form-input"></ td>
                <td><input type = "number" name = "pin[]" class = "form-input"></ td>
                <td><button type = "button" onclick = "removeRow(this) " class
            = "btn-danger btn" > Remove</ button></ td>
            `;
        tbody.appendChild(row);
    }

    function removeRow(btn)
    {
        btn.closest('tr').remove();
    }
    </script>
</body>
</html>
        )";

        page.replace("%OWN_ADDR%", ownAddrStr);
        page.replace("%CONNECTION_ROWS%", connectionRows);
        page.replace("%SERVER_URL%", getServerURL());
        server.send(200, "text/html", page);
    }

    bool
    connectWiFi(const String &ssid, const String &pwd)
    {
        Serial.printf("[Web] connectWiFi '%s'\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pwd.c_str());
        uint32_t t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < WIFI_CONNECT_TIMEOUT_MS)
            delay(500);
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("[Web] WiFi connected");
            return true;
        }
        Serial.println("[Web] WiFi failed");
        return false;
    }

    void loadAPSuffix()
    {
        Serial.println("[Web] loadAPSuffix");
        if (!LittleFS.exists(AP_SUFFIX_FILE))
        {
            apSuffix = esp_random() & 0xFFFF;
            File f = LittleFS.open(AP_SUFFIX_FILE, "w");
            if (f)
            {
                f.println(apSuffix);
                f.close();
                Serial.printf("[Web] new suffix=%u\n", apSuffix);
            }
        }
        else
        {
            File f = LittleFS.open(AP_SUFFIX_FILE, "r");
            if (f)
            {
                apSuffix = f.readStringUntil('\n').toInt();
                f.close();
                Serial.printf("[Web] loaded suffix=%u\n", apSuffix);
            }
        }
    }

    void loadCredentials()
    {
        Serial.println("[Web] loadCredentials");
        if (!LittleFS.exists(WIFI_CRED_FILE))
        {
            File f = LittleFS.open(WIFI_CRED_FILE, "w");
            if (f)
                f.close();
            return;
        }
        File f = LittleFS.open(WIFI_CRED_FILE, "r");
        if (f)
        {
            wifiSSID = f.readStringUntil('\n');
            wifiSSID.trim();
            wifiPassword = f.readStringUntil('\n');
            wifiPassword.trim();
            f.close();
            Serial.printf("[Web] creds loaded SSID='%s'\n", wifiSSID.c_str());
        }
    }

    void saveCredentials()
    {
        Serial.println("[Web] saveCredentials");
        File f = LittleFS.open(WIFI_CRED_FILE, "w");
        if (f)
        {
            f.println(wifiSSID);
            f.println(wifiPassword);
            f.close();
            Serial.println("[Web] creds saved");
        }
    }

    void loadConnections()
    {
        Serial.println("[Web] loadConnections");
        physikalNode.logicalNode.connections.clear();

        if (!LittleFS.exists(CONN_FILE))
        {
            File f = LittleFS.open(CONN_FILE, "w");
            if (f)
                f.close();
            return;
        }

        File f = LittleFS.open(CONN_FILE, "r");
        if (f)
        {
            bool firstLine = true;
            while (f.available())
            {
                String ln = f.readStringUntil('\n');
                if (ln.isEmpty())
                    continue;

                int p = ln.indexOf(':');
                if (p < 0)
                    continue;

                String as = ln.substring(0, p);
                String ps = ln.substring(p + 1);

                if (firstLine)
                {
                    // Load Own Address
                    Address addr;
                    std::istringstream ss(as.c_str());
                    uint16_t v;
                    while (ss >> v)
                    {
                        addr.push_back(v);
                        if (ss.peek() == ',')
                            ss.ignore();
                    }
                    physikalNode.logicalNode.you = addr;
                    firstLine = false;
                }
                else
                {
                    // Load Connection
                    Connection c;
                    std::istringstream ss(as.c_str());
                    uint16_t v;
                    while (ss >> v)
                    {
                        c.address.push_back(v);
                        if (ss.peek() == ',')
                            ss.ignore();
                    }
                    c.pin = uint8_t(ps.toInt());
                    physikalNode.logicalNode.connections.push_back(c);
                }
            }
            f.close();
        }
        Serial.printf("[Web] %u connections loaded\n", physikalNode.logicalNode.connections.size());
    }

    void saveConnections()
    {
        Serial.println("[Web] saveConnections");
        File f = LittleFS.open(CONN_FILE, "w");
        if (f)
        {
            // Save Own Address (first line)
            for (size_t i = 0; i < physikalNode.logicalNode.you.size(); ++i)
            {
                f.print(physikalNode.logicalNode.you[i]);
                if (i + 1 < physikalNode.logicalNode.you.size())
                    f.print(',');
            }
            f.println(":0");

            // Save Connections
            for (auto &c : physikalNode.logicalNode.connections)
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
            Serial.println("[Web] connections saved");
        }
    }
};
