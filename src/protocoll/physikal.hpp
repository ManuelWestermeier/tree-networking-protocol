#pragma once

#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <string.h>

using namespace std;

struct PhysikalConnection
{
  uint8_t inpPin;
  uint8_t outPin;
};

struct PhysikalPocket
{
};

struct PhysikalNode
{
  vector<PhysikalConnection> pins;
  Node logicalNode;

  void init()
  {
  }

  void send(Address to, uint8_t *data)
  {
  }

  void loop()
  {
  }
};
