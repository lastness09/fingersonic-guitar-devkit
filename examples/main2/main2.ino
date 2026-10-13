// SPDX-License-Identifier: Apache-2.0
#include "GrooveRuntime.h"

// Standalone App0 entry point for the FM Studio reference application.
void setup() {
  Serial.begin(115200);
  groove::runtimeBegin();
  groove::displayBegin();
}
void loop() {
  groove::runtimeService();
  delay(10);
}
