#include <Arduino.h>

// UART RX is connected to the BMS A line
// UART TX is connected to the MB A line
#define TX_PIN 1
// Connected to the MB B line
#define TX_INVERSE_PIN 2

namespace {
unsigned long lastByteReceivedMicros;
bool nextlineCocked = false;

const int payloadLengthByHeader[] = {3, -1, 34, 3,  7, 4, 6,
                                     9, 3,  3,  -1, 4, 4, 5};
// Emulate the RS485 B line by bitbanging the inverse
// of the A line.
void IRAM_ATTR txPinRiseInterrupt() { digitalWrite(TX_INVERSE_PIN, 0); }
void IRAM_ATTR txPinFallInterrupt() { digitalWrite(TX_INVERSE_PIN, 1); }
}  // namespace

void bms_setup() {
  // The B line idle is 0
  digitalWrite(TX_INVERSE_PIN, 0);
  pinMode(TX_INVERSE_PIN, OUTPUT);

  // Serial1.begin(115200);

  // Not sure what Arduino core does to the ports when
  // configured for uart so I'm enabling the pullup
  // after the Serial1 setup.
  pinMode(3, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(TX_PIN), txPinRiseInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(TX_PIN), txPinFallInterrupt, FALLING);

  Serial.println("BMS mode");
}

void bms_loop() {
  if (Serial1.available()) {
    while (Serial1.available()) {
      int byte = Serial1.read();
      Serial.printf("%02X ", byte);
      Serial1.write(byte);
    }
    nextlineCocked = true;
    lastByteReceivedMicros = micros();
  }
  if (nextlineCocked == true && micros() - lastByteReceivedMicros > 1000) {
    nextlineCocked = false;
    Serial.write('\n');
  }
}