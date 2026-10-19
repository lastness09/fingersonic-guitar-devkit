#include "GameRuntime.h"
// Independent experimental App0 entry point; preserve App1 and the manufacturer bootloader.
void setup() {
  Serial.begin(115200);
  pea::runtimeBegin();
  pea::displayBegin();
}
void loop() {
  pea::runtimeService();
  delay(10);
}
