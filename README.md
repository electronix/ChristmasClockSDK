# Christmas Clock

Firmware for a Christmas countdown clock built around a Raspberry Pi Pico (RP2040) on a custom PCB. Four seven-segment digits made of 142 addressable RGB LEDs show a countdown or, when no countdown is running, the time of day. C++17 with the Pico SDK and CMake.

## What it does

- **Countdown display**: shows `MM:SS`, counting down once per second. It starts paused with a 5 minute countdown and can go negative. The colour escalates from green through orange to red as the end approaches (warning at one third, final phase at one sixth of the countdown).
- **Time of day**: while no countdown is running and nobody has touched the clock for 3 seconds, the display shows `HH:MM` in cyan with a blinking colon. A red LED runs around the 60 LEDs on the outer edge of the display as a seconds dial, one step per second, starting at the bottom left. The time is set from a PC over USB (see below); there is no battery-backed RTC, so it has to be set again after every power-up.
- **Touch controls** (MPR121 capacitive controller): a touch slider sets the brightness (for the countdown and the clock alike) and six pads control the countdown: -1 min, +1 min, reset, start/pause, +1 s, -1 s.
- **IR remote control**: receives NEC-protocol remotes (three physical remotes are mapped: Terratec, DVB-T and Samsung) for on/off, brightness, and a menu (`ENTER`) in which the countdown length is typed in as digits. A transmitter is included too, and a loopback self-test can be enabled with `IR_LOOPBACK_TEST` in `app/src/Main.cpp`.
- **Effects**: after 10 idle seconds without a time set, a "Matrix rain" screensaver runs. Touching TP1 and TP6 together toggles a Snake game-style effect. A 4-character marquee window for hex text (`BEEF`, `CAFE`, ...) is available in the display driver.
- **USB serial console**: text output (a `[heartbeat]` line every 5 seconds) and simple commands over the USB CDC serial port.

### Setting the time over USB

The clock asks for the time with `REQ TIME` whenever a terminal connects, and every 2 seconds while its time is unset. Commands (one per line): `TIME YYYY-MM-DD HH:MM:SS` (local time), `GET TIME`, `HELP`.

The easy way is `tools/settime.html`, a single-file Web Serial page for Chrome or Edge: open it, press "Mit Uhr verbinden" (connect) once and pick the clock's serial port. From then on the page reconnects by itself while it stays open and answers every time request from the clock with the PC's local time. If the browser refuses Web Serial on a local `file://` page, serve the folder instead, e.g. `python -m http.server` in `tools/`, and open `http://localhost:8000/settime.html`.

### DCF77

DCF77 radio time reception does **not** work with the on-board receiver: its analog front end amplifies noise to full swing and no signal is ever visible. The driver was removed again (it remains in the git history); the comment at the top of `app/src/Main.cpp` explains why. An external DCF77 module would be the only realistic way to get it working.

## Code layout

| Path | Contents |
|---|---|
| `app/` | main loop (`Main.cpp`), state machine and UI (`ChristmasClock`), wall clock (`WallClock`), serial commands (`SerialCommands`) |
| `drivers/led/` | WS2812 driver over PIO/DMA, seven-segment rendering (`SevenSeg`), Matrix and Snake effects |
| `drivers/ir/` | NEC receive/transmit over PIO, error correction, remote-control mapping (`NECEventMapper`) |
| `drivers/touch/` | MPR121 touch slider and pads over I2C |
| `drivers/power/` | placeholder for power management, not implemented yet |
| `tools/` | `settime.html`, the time-setting web page |

The hardware design files (schematic, PCB layout, Gerber, enclosure) are not part of this repository, see `hardware/README.txt`. `CLAUDE.md` holds detailed developer notes.

## Gitpod

When starting in gitpod, the environment is automatically prepared. All required packages are installed and the raspberry pi pico sdk is cloned from the git master branch.

## SDK Setup

Install the following packages to cross compile for the RP2040:

```bash
sudo apt install cmake build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

## Prepare build files (Pico SDK from git)

```bash
mkdir build
cd build
cmake -DPICO_SDK_FETCH_FROM_GIT=ON ..
```

## Prepare build files (local Pico SDK)

If the SDK is located in ~/pico-sdk the script start-env can be used:

```bash
. start-env
```

If the file github-id contains the private key used for your github account the script will start a seperate ssh-agent with only that key.

Otherwise use:

```bash
mkdir build
cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
```

## Build

```bash
make -j$(nproc)
```

## Deploy

- Press BOOTSEL button on the Pico board and reset the board
- Copy the `build/app/ChristmasClock.uf2` file to the Pico board
