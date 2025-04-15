#pragma once

#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <string.h>

using namespace std;

struct Address : public vector<uint16_t>
{
};

struct Match
{
  int positive;
  int negative;
};

Match match(const Address &a1, const Address &a2)
{
  size_t maxLen = max(a1.size(), a2.size());
  size_t minLen = min(a1.size(), a2.size());
  Match m = {0, 0};

  while (m.positive < minLen && a1[m.positive] == a2[m.positive])
  {
    m.positive++;
  }

  m.negative = maxLen - 1 - m.positive;
  return m;
}

bool eq(const Address &a1, const Address &a2)
{
  if (a1.size() != a2.size())
    return false;

  for (int i = 0; i < a1.size(); i++)
  {
    if (a1.at(i) != a2.at(i))
      return false;
  }

  return true;
}

struct Connection
{
  Address address;
  uint8_t pin;
};

struct Pocket
{
  Address address;
  char data[11] = "HELLOWORLD";
  uint16_t checksum;

  void computeCheckSum()
  {
    checksum = 0;
    for (const auto &part : address)
    {
      checksum ^ part;
    }
    for (uint8_t i = 0; i < sizeof(data); i++)
    {
      checksum ^= data[i];
    }
  }

  Pocket(Address a, const char *d)
  {
    address = a;
    strncpy(data, d, 10);
    data[sizeof(data) - 1] = '\0';
    computeCheckSum();
  }
};

struct Node
{
  vector<Connection> connections;
  Address you;

  uint8_t send(Pocket p)
  {
    if (connections.empty())
    {
      Serial.println("No available connections to send data.");
      return 0;
    }

    Match bestMatch = {0, 0};
    vector<Connection> sendConnections;
    Connection sendConnection = connections.at(0);

    for (const auto &connection : connections)
    {
      Match currentMatch = match(connection.address, p.address);
      if (bestMatch.positive <= currentMatch.positive)
      {
        if (bestMatch.positive < currentMatch.positive)
        {
          sendConnections.clear();
        }
        bestMatch = currentMatch;
        sendConnections.push_back(connection);
      }
    }

    bestMatch = match(sendConnection.address, p.address);
    for (const auto &goodConnection : sendConnections)
    {
      Match currentMatch = match(goodConnection.address, p.address);
      if (currentMatch.negative <= bestMatch.negative)
      {
        sendConnection = goodConnection;
        bestMatch = currentMatch;
      }
    }

    Serial.print("Sending data via pin ");
    Serial.println(sendConnection.pin);

    return sendConnection.pin;
  }

  uint8_t recieve(Pocket p)
  {
    if (eq(you, p.address))
    {
      return 0;
    }
    else
    {
      return send(p);
    }
  }
};