#include <Arduino.h>

// Connected to the BMS A line
#define RX_PIN 32
// Connected to the MB A line
#define TX_PIN 14
// Connected to the MB B line
#define TX_INVERSE_PIN 27

unsigned long lastByteReceivedMicros;
bool nextlineCocked = false;

const int payloadLengthByHeader[] = {3, -1, 34, 3,  7, 4, 6,
                                     9, 3,  3,  -1, 4, 4, 5};

// Emulate the RS485 B line by bitbanging the inverse
// of the A line.
void IRAM_ATTR txPinChangeInterrupt() {
  digitalWrite(TX_INVERSE_PIN, !digitalRead(TX_PIN));
}

void setup() {
  // The B line idle is 0
  digitalWrite(TX_INVERSE_PIN, 0);
  pinMode(TX_INVERSE_PIN, OUTPUT);

  Serial.begin(2000000);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // Not sure what Arduino core does to the ports when
  // configured for uart so I'm enabling the pullup
  // after the Serial1 setup.
  pinMode(RX_PIN, INPUT_PULLUP);

  attachInterrupt(TX_PIN, txPinChangeInterrupt, CHANGE);

  Serial.println("Booted up");
}

void loop() {
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