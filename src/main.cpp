#include <Arduino.h>

#include "webinterface/index.hpp"

WebInterface webif;

void setup()
{
  Serial.begin(115200);
  webif.begin();
}

void loop()
{
}
