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

Match match(const Address &connection, const Address &pocket)
{
  size_t maxLen = max(connection.size(), pocket.size());
  size_t minLen = min(connection.size(), pocket.size());
  Match m = {0, 0};

  while (m.positive < minLen && connection[m.positive] == pocket[m.positive])
  {
    m.positive++;
  }

  m.negative = maxLen - m.positive;
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

#include "./pocket.hpp"

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