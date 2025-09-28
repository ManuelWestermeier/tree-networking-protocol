# MW-TREE-ROUTER

open preview https://manuelwestermeier.github.io/tree-networking-protocol/network-sim

breadboard + esp1 = http://192.168.4.3/connections

breadboard + esp2 = http://192.168.4.2/connections

module + esp1 = http://192.168.4.1/connections

how to use:

1. clone project.
2. install pio
3. run for exemple: `.\upload.bat COM7 COM4`

```bash
.\upload.bat ... all ports (with esp32) to ulpoad the project
```

# Algorithmus‑Beschreibung

Dieser Abschnitt erklärt den inneren Ablauf des Tree Networking Protocol (TNP) ohne konkreten Code.

---

## 1. Adressabgleich

- Zwei Adressen (dynamische Listen von 16‑Bit‑Werten) werden Element für Element verglichen.
- Zunächst wird die Anzahl der hintereinander gleichen Elemente vom Listenanfang an gezählt („positive“ Treffer).
- Sobald ein Unterschied auftritt oder das Ende der kürzeren Liste erreicht ist, stoppt die Zählung.
- Die verbleibende Länge der getesteten Adresse (Restlänge) ergibt die „negative“ Größe.

---

## 2. Match‑Index

- Aus den Werten „positive“ und „negative“ wird ein einfacher Score berechnet:
  **Match‑Index = positive − negative**
- Ein hoher Index bedeutet:
  1. viele aneinanderhängende Übereinstimmungen am Anfang
  2. kurze Restlänge der Adresse
- Dieser Wert dient als Entscheidungsgrundlage für das Routing.

---

## 3. Gleichheitsprüfung

- Zwei Adressen gelten als identisch, wenn sie gleich lang sind und an jeder Position exakt denselben Wert haben.
- Wird eine exakte Übereinstimmung festgestellt, interpretiert der Knoten dies als „Ziel erreicht“.

---

## 4. Routing‑Logik

1. **Eingangskontrolle**

   - Liegen keine Verbindungen vor, wird abgebrochen und ein Fehlersignal („keine Verbindungen“) ausgegeben.

2. **Auswahl der besten Verbindung**

   - Für jede vorhandene Verbindung wird der Match‑Index gegen die gewünschte Zieladresse berechnet.
   - Es wird die Verbindung mit dem höchsten Match‑Index ausgewählt.

3. **Weiterleitung**

   - Die gewählte Verbindung liefert einen Pin‑Wert zurück und gibt zur Diagnose eine Meldung über die serielle Schnittstelle aus.

4. **Empfangsfall**

   - Wenn die Zieladresse mit der eigenen Adresse des Knotens übereinstimmt, wird nicht weitergeleitet, sondern die Verarbeitung endet mit dem Rückgabewert 0.

5. **Loadbalancing**
   - Wenn es zwei "gleich gute" (Matchindex equivalente indexe) gibt, wird die Lösung, mit der kleineren Adress-Länge ausgewählt.

---

## Zusammenfassung

- **Präfix‑Matching** bestimmt, wie gut eine Verbindung zur Zieladresse passt.
- **Match‑Index** gewichtet lange gemeinsame Präfixe und kurze Reste positiv.
- **Exakte Gleichheit** stoppt das Routing am Bestimmungsort.
- **Iterative Auswahl** stellt sicher, dass jeder Knoten stets den optimalen Pfad nutzt.

```

```

this is the routing logic: 
```cpp
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
    
    if (eq(you, p.address))
    {
      Serial.println("[Protocol] recieve: packet is for us");
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

    // if the pocket is for a direct child, but the node is the last (its a virtual children)
    if (isDirectChildren && !isChildren(sendConnection.address, you))
    {
      return 0;
    }

    Serial.print("[Protocol] send: sending via pin ");
    Serial.println(sendConnection.pin);

    return sendConnection.pin;
  }

  uint8_t recieve(Pocket p)
  {
    return send(p);
  }
};
```