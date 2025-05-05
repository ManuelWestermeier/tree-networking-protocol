#include <Arduino.h>

#include "webinterface/index.hpp"

WebInterface web(80);

void setup()
{
  Serial.begin(115200);
  web.begin();
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}