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

  Serial.print("[Protocol] match: positive=");
  Serial.print(m.positive);
  Serial.print(", negative=");
  Serial.println(m.negative);

  return m;
}

int matchIndex(const Match &m)
{
  int index = m.positive - m.negative;
  Serial.print("[Protocol] matchIndex: index=");
  Serial.println(index);
  return index;
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

bool isChildren(const Address &other, const Address &you)
{
  if (other.size() <= you.size())
    return false;

  for (size_t i = 0; i < you.size(); i++)
  {
    if (other.at(i) != you.at(i))
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
      Serial.println("[Protocol] send: no connections available");
      return 0;
    }

    Serial.println("[Protocol] send: selecting best connection...");

    Connection sendConnection = connections.at(0);
    int bestMatchIndex = matchIndex(match(sendConnection.address, p.address));
    int bestAdressLength = sendConnection.address.size();

    bool isDirectChildren = isChildren(p.address, you);

    for (size_t i = 1; i < connections.size(); i++)
    {
      int currentMatchIndex = matchIndex(match(connections[i].address, p.address));

      if (currentMatchIndex > bestMatchIndex)
      {
        sendConnection = connections[i];
        bestMatchIndex = currentMatchIndex;
      }
      else if (currentMatchIndex == bestMatchIndex && bestAdressLength < connections[i].address.size())
      {
        bestAdressLength = connections[i].address.size();
        sendConnection = connections[i];
        bestMatchIndex = currentMatchIndex;
      }
    }

    if (isDirectChildren && !isChildren(sendConnection.address, you))
    {
      return -1;
    }

    Serial.print("[Protocol] send: sending via pin ");
    Serial.println(sendConnection.pin);

    return sendConnection.pin;
  }

  uint8_t recieve(Pocket p)
  {
    if (eq(you, p.address))
    {
      Serial.println("[Protocol] recieve: packet is for us");
      return 0;
    }
    else
    {
      Serial.println("[Protocol] recieve: forwarding packet");
      return send(p);
    }
  }
};
