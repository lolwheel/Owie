# OWEnhancer

This project is inspired by the JWFFM modchip and aims to unlock battery expansion possibilities on otherwise
locked Onewheels such as Onewheel XR with FW version 4210+
and Onewheel Pint with FW 5059+.

# Disclaimer

The authors and contributors of this project are in no way affiliated with Future Motion Inc. Onewheel, Onewheel XR, Onewheel Pint, Onewheel Plus, etc are registered trademarks of Future Motion Inc.

This is a hobby projet for its contributors and comes with absolutely no guarantees of any sorts. **Messing with your Onewheel in any way voids its warranty and could potentially lead to property loss, injuries or even death.** Don't be silly and use this project at your own risk.

# Features

- The software is absolutely free and runs on a very cheap (~$3) board - Wemos D1 Mini Lite. **This tiny board has WiFi on board so we can offer future updates without you ever having to open the board again. HOW COOL IS THAT!!!**
- Removes BMS pairing - Enables you to use BMS boards from any Onewheel of the same model in your board. (Currently tested between different Pints of the same revision)

- UPCOMING - fix the battery percentage reporting for Vamp-and-Ride setups and expanded battery packs.
- UPCOMING - Removes the factory locking of battery capacity expansion, implemented in OW XRs with revisions 4210+ and Pints with revisions 5059+.

# Installing OWEnhancer into your board

## Prerequisites:

- Have essential soldering skills and tools: Soldering iron, some 22 gauge or otherwise thin wires, fish tape or isolating tape.
- Be comfortable with opening your board's battery enclosure. This requires a somewhat exotic Torx 5 point security bit, size TS20. [Amazon link](ttps://www.amazon.com/gp/product/B07TC79LVH).
- Wemos D1 Mini Lite - the cheapest and most compact ESP8266 board that I'm aware of. You can find those on Aliexpress and Amazon. Buy version without the metal shield or ceramic WiFi antenna on it as they're too bulky to fit inside of the battery enclosure.

## Installation:

1. Install OWEnhancer fimrware onto your Wemos D1 mini. TODO(lolwheel): Write a page in details on how to do this.
1. Disassemble your board and open the battery enclosure.
1. Disconnect all wires from BMS, strictly in the following order:
   1. Battery main lead - an XT60 connector on the rightmost side of the BMS.
   1. Battery balance lead - the leftmost connector on the BMS.
   1. All the other wires to the BMS, the order here doesn't matter.
1. Prepare your Wemos D1 Mini and BMS:
   1. Tin 4 consecutive pins on Wemos D1 Mini marked as **TX, RX, D1, D2** as well as **5v, GND** pins.
   1. Solder a small wire **on the top of the board** connecting the pin marked as **TX** to the pin marked as **D2**
   1. Solder power pickup wires to the BMS. The JWFFM chip installation video demonstrates this well - [Youtube: Power pickup from BMS](https://youtu.be/kSWicH8hUFo?t=1028)
   1. Cut the WHITE and GREEN wires from the three-wire connector around 3/4 of an inch from the connector. Wrap the Green wire **leading to the BMS**(the 3/4 inch stub) in an isolating wire as we won't be needing it. Tin the other three wire endings, you'll be soldering those to the Wemos D1 Mini.
      Again, JWFFM install video has a good demonstration of this: [Youtube: Cutting GREEN and WHITE wires](https://youtu.be/kSWicH8hUFo?t=453)
1. Connecting wires to your Wemos D1 Mini. I found it much easier to soldered these to the bottom of the board:
   1. Connect the **GROUND** wire from the **BMS**, the middle wire out of the BMS 5 pin connector to **GND** on Wemos D1.
   1. Connect the **5v** wire, the other one from BMS to the **5v** on the board.
   1. Connect the **WHITE** wire **RUNNING TO THE MAIN BOARD** to the **TX** pin on the board.
   1. Connect the **GREEN** wire **RUNNING TO THE MAIN BOARD** to the **D1** pin on the board.
   1. Connect the **WHITE** stubby wire running to the **BMS** to the **RX** pin on the board.
   1. Cover the bottom of the Wemos D1 mini with either fish tape or isolating tape so that non of the exposed soldering joints have any chance of contacting anything on the BMS. I also put a bunch of tape on the top of the board, just in case.

Pictures demonstrating soldering points on the board:

<img src= "docs/img/wemos_d1_top.png?raw=true" height="180px">

<img src="docs/img/wemos_d1_bottom.png?raw=true" height="180px">

How it looks like in my setup:
<img src="docs/img/wemos_d1_installed.jpg" height="180px">

# For developers/collaborators:

## Onewheel BMS -> MB communiction

The BMS (Battery Management System) board, located in the battery side of the onewheel, communicates with the main board via [RS485](https://en.wikipedia.org/wiki/RS-485) protocol. Details that I've managed to discover so far:

- The communication is unidirectional. The data flows only from BMS to MB. It's 115200 baud, standard 8N1 framing.
- The RS485 bus signaling voltage seems to be 3.3v. This makes it possible to read the bus voltages directly via ESP8266 and doesn't require 5v -> 3.3v level shifting.
- Both ends of the RS485 bus are terminated properly - 120 ohm resistors across the A and B lines and pullup / pulldown resistors A and B lines correspondingly.

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

TODO(lolwheel) fill in as I go.

The `02` message seems to encode the individual cell voltages, e.g.

```
FF 55 AA 02 0E EB 0E EF 0E EC 0E ED 0E EF 0E ED 0E EF 0E EF 0E ED 0E F0 0E ED 0E F0 0E ED 0E F0 0E F0 00 2B 10 F1
```

First three bytes - preamble, fourth byte - message type. Next 30 bytes are 15 `uint16` representing cell voltages in volts \* 1000, e.g. `0EEB` = 3819 in decimal = 3.819 volts.

The `03` message seems to encode the battery percentage.

```
FF 55 AA 03 48 02 49
```

First three bytes - preamble, fourth byte - message type. Next byte is the current battery percentage being reported by the board. e.g. 0x48 is 72 in decimal = 72% battery. Last two bytes are checksum.

The `06` message encodes the BMS serial number.

The data is 4 bytes long and is big-endian encoded `uint32_t` serial number of the BMS.
Spoofing this number works around the BMS pairing which I've tested by
swapping BMSes between two Pints.
