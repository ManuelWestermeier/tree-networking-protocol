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
        Serial.printf("[Web] loaded %u connections\n", (unsigned)connections.size());

        Serial.println("[Web] spawning task");
        xTaskCreatePinnedToCore(
            webTask, "WebServer", 8192, this, 1, nullptr, 1);
    }

    const std::vector<Connection> &getConnections() const { return connections; }

private:
    WebServer server;
    uint16_t serverPort;
    String wifiSSID, wifiPassword;
    uint16_t apSuffix;
    std::vector<Connection> connections;

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
        String rows;
        for (auto &c : connections)
        {
            String a;
            for (size_t i = 0; i < c.address.size(); ++i)
            {
                a += String(c.address[i]);
                if (i + 1 < c.address.size())
                    a += ",";
            }
            rows += "<tr><td><input name='address[]' value='" + a + "' class='form-control'></td>";
            rows += "<td><input type='number' name='pin[]' value='" + String(c.pin) + "' class='form-control'></td>";
            rows += "<td><button onclick='removeRow(this)' class='btn btn-outline-danger'>Remove</button></td></tr>";
        }

        String page2 = R"rawc(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
  <title>Connections</title>
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
<body class="bg-light">
<nav class="navbar navbar-expand-lg navbar-dark bg-primary">
  <div class="container-fluid">
    <a class="navbar-brand" href="#">Node Web UI</a>
    <span class="navbar-text ms-auto">%SERVER_URL%</span>
  </div>
</nav>
<div class="container py-4">
    <h3 class="card-title">Sensor Connections</h3>
    <form action="/connections/save" method="post">
    <table class="table">
        <thead class="table-light"><tr><th>Address</th><th>Pin</th><th></th></tr></thead>
        <tbody>%ROWS%</tbody>
    </table>
    <button type="button" onclick="addRow()" class="btn btn-outline-secondary mb-3">Add Row</button>
    <button type="submit" class="btn btn-success">Save Changes</button>
    </form>
</div>
<script>
  function addRow() {
    let t = document.querySelector('tbody');
    let r = document.createElement('tr');
    r.innerHTML = "<td><input name='address[]' class='form-control'></td><td><input type='number' name='pin[]' class='form-control'></td><td><button onclick='removeRow(this)' class='btn btn-outline-danger'>Remove</button></td>";
    t.appendChild(r);
  }
  function removeRow(b) {
    b.closest('tr').remove();
  }
</script>
</body>
</html>
)rawc";
        page2.replace("%ROWS%", rows);
        page2.replace("%SERVER_URL%", getServerURL());
        server.send(200, "text/html", page2);
    }

    void handleConnectionsSave()
    {
        Serial.println("[Web] handleConnectionsSave");
        std::vector<String> addrs, pins;
        for (int i = 0; i < server.args(); ++i)
        {
            if (server.argName(i) == "address[]")
                addrs.push_back(server.arg(i));
            if (server.argName(i) == "pin[]")
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
            c.pin = uint8_t(pins[i].toInt());
            connections.push_back(c);
        }
        saveConnections();
        server.sendHeader("Location", "/connections");
        server.send(302, "text/plain", "");
    }

    bool connectWiFi(const String &ssid, const String &pwd)
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
        connections.clear();
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
            while (f.available())
            {
                String ln = f.readStringUntil('\n');
                if (ln.isEmpty())
                    continue;
                int p = ln.indexOf(':');
                if (p < 0)
                    continue;
                String as = ln.substring(0, p), ps = ln.substring(p + 1);
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
                connections.push_back(c);
            }
            f.close();
        }
        Serial.printf("[Web] %u connections loaded\n", (unsigned)connections.size());
    }

    void saveConnections()
    {
        Serial.println("[Web] saveConnections");
        File f = LittleFS.open(CONN_FILE, "w");
        if (f)
        {
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
            Serial.println("[Web] connections saved");
        }
    }
};
