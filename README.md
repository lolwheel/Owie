# ow_bms_mitm

This project is inspired by the JWFFM modchip and aims to unlock battery expansion possibilities on otherwise
locked Onewheels such as Onewheel XR with FW version 4210+
and Onewheel Pint with FW 5059+.

# Necessary hardware

This project is being developed and is planned to release on an ESP8266 board, likely the cheapest Wemos D1 Mini, available for $2. No other hardware is necessary.

# For developers/collaborators:

## Onewheel BMS -> MB communiction

The BMS (Battery Management System) board, located in the battery side of the onewheel, communicates with the main board via [RS485](https://en.wikipedia.org/wiki/RS-485) protocol. Details that I've managed to discover so far:

- The communication is unidirectional. The data flows only from BMS to MB. It's 115200 baud, standard 8N1 framing.
- The RS485 bus signaling voltage seems to be 3.3v. This makes it possible to read the bus voltages directly via ESP32.
- Both ends of the RS485 bus are terminated properly - 120 ohm resistors across the A/B lines and pullup and pulldown resistors are connected to A and B lines correspondingly.

## Doing away with MAX485 drivers

Technically, we'd need to use RS485 drivers such as MAX485 to intercept and retransmit bits on the line, however so far it seems like we can do away with them:

### Receiving RS485 directly via hardware UART:

The A (high) line of the RS485 bus coming from the BMS can be read directly via hardware UART with a little care.

When the line isn't driven by the BMS transmitter, it hovers around 3.3v/2 = 1.6v due to the terminating resistors on the BMS side. 1.6v isn't a defined logic level for a GPIO pin so chances are we'll read spurious data. However, if we turn on the pullup resistor of the UART RX pin to which the A line is connected, the bus idle voltage gets pulled up to right above 3 volts, which is more than 0.75\*VDD necessary for a logic 1 input, mentioned in ESP32 datasheet. This way the A wire from the BMS can be fed directly into the UART Rx pin and its logical state will read 1 at line idle, just as we want for UART communication.

## Transmitting RS485 without the driver

Transmitting the data to the MB requires us to signal on two wires. The A wire of the RS485 can be connected directly to the output of a hardware UART. The B line must be inverse of the A line as RS485 uses differential signalling.
I've achieved this by simply attaching a pin change interrupt to the UART TX pin and bitbanging the inverse value of this pin to the B line of the RS485.

# Communication protocol

The data frames sent by BMS are of the following general format:

1. Preamble - 3 bytes, fixed: `FF 55 AA`
1. Message type - 1 byte. I've so far observed all values between `0` and `0xD`, inclusive, except `1` and `0x10`
1. Message body - variable length but fixed based on the message type above.
1. Checksum - last two bytes of the frame - simply sum of all of the bytes in the frame, including the preamble.

## Message types:

TODO(lolonewheel) fill in as I go.

The `02` message seems to encode the individual cell voltages, e.g.

```
FF 55 AA 02 0E EB 0E EF 0E EC 0E ED 0E EF 0E ED 0E EF 0E EF 0E ED 0E F0 0E ED 0E F0 0E ED 0E F0 0E F0 00 2B 10 F1
```

First three bytes - preamble, fourth byte - message type. Next 30 bytes are 15 `uint16` representing cell voltages in volts \* 1000, e.g. `0EEB` = 3819 in decimal = 3.819 volts.
