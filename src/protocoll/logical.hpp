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
  uint16_t positive;
  uint16_t negative;
};

Match match(const Address &connection, const Address &pocket)
{
  size_t minLen = min(connection.size(), pocket.size());
  Match m = {0, 0};

  while (m.positive < minLen && connection[m.positive] == pocket[m.positive])
  {
    m.positive++;
  }
  m.negative = connection.size() - m.positive;

  return m;
}

int matchIndex(const Match &m)
{
  return m.positive - m.negative;
}

bool eq(const Address &a1, const Address &a2)
{
  if (a1.size() != a2.size())
    return false;

  for (size_t i = 0; i < a1.size(); i++)
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

    Connection sendConnection = connections.at(0);

    int bestMatchIndex = matchIndex(match(sendConnection.address, p.address));
    int bestAdressLength = sendConnection.address.size();

    for (size_t i = 1; i < connections.size(); i++)
    {
      int currentMatchIndex = matchIndex(match(connections[i].address, p.address));
      if (currentMatchIndex > bestMatchIndex)
      {
        sendConnection = connections[i];
        bestMatchIndex = currentMatchIndex;
      }
      else if (currentMatchIndex == bestMatchIndex && bestAdressLength < sendConnection.address.size())
      {
        bestAdressLength = sendConnection.address.size();
        sendConnection = connections[i];
        bestMatchIndex = currentMatchIndex;
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