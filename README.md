# MMDVMHost

MMDVMHost is the host program for the Multi-Mode Digital Voice Modem (MMDVM). It interfaces with MMDVM-compatible hardware (MMDVM, DVMega, ZUMspot, MMDVM_HS, etc.) on one side and digital voice network gateways on the other. It supports seven digital and analog radio modes for amateur radio repeaters and hotspots.

## Supported Modes

| Mode | Protocol | Network Gateway |
|------|----------|----------------|
| **D-Star** | JARL digital voice standard | D-Star Gateway |
| **DMR** | Digital Mobile Radio (2-slot TDMA) | DMR Gateway (BrandMeister, DMR+, etc.) |
| **System Fusion** (YSF) | Yaesu digital voice/data | DG-Id or YSF Gateway (FCS/YSF networks) |
| **P25** | Project 25 Phase 1 | P25 Gateway |
| **NXDN** | Kenwood/Icom digital standard | NXDN Gateway (NXDN/NXCore talk groups) |
| **POCSAG** | Paging protocol | DAPNET Gateway |
| **FM** | Analog FM with CTCSS/squelch | FM Network Gateway |

The DVMega supports D-Star, DMR, and System Fusion.

## Table of Contents

- [Requirements](#requirements)
- [Building](#building)
- [Installation](#installation)
- [Configuration](#configuration)
  - [General](#general)
  - [Info](#info)
  - [Log](#log)
  - [MQTT](#mqtt)
  - [CW ID](#cw-id)
  - [ID Lookups](#id-lookups)
  - [Modem](#modem)
  - [Transparent Data](#transparent-data)
  - [D-Star](#d-star)
  - [DMR](#dmr)
  - [System Fusion](#system-fusion)
  - [P25](#p25)
  - [NXDN](#nxdn)
  - [POCSAG](#pocsag)
  - [FM](#fm)
  - [Network Sections](#network-sections)
  - [Lock File](#lock-file)
  - [Remote Control](#remote-control)
- [Running](#running)
  - [Foreground](#foreground)
  - [Daemon / systemd](#daemon--systemd)
  - [Docker](#docker)
- [Architecture](#architecture)
- [Compile-Time Mode Selection](#compile-time-mode-selection)
- [Remote Control Commands](#remote-control-commands)
- [MQTT Deep Dive](#mqtt-deep-dive)
  - [Connection and Topic Namespace](#connection-and-topic-namespace)
  - [Published Topics](#published-topics)
  - [Subscribed Topics](#subscribed-topics)
  - [JSON Payload Reference](#json-payload-reference)
  - [MQTT Integration Examples](#mqtt-integration-examples)
- [Helper Scripts](#helper-scripts)
- [Hardware Support](#hardware-support)
- [License](#license)

## Requirements

- **OS**: Linux (32/64-bit), macOS, or Windows (Visual Studio 2022, x86/x64)
- **Compiler**: C++17-capable compiler (GCC, Clang, or MSVC)
- **Libraries**:
  - `libpthread` - POSIX threads (Linux/macOS)
  - `libutil` - login utilities (Linux only)
  - `libmosquitto` - Eclipse Mosquitto MQTT client library
  - `nlohmann/json` - JSON for Modern C++ (header-only)
- **Hardware**: MMDVM-compatible modem connected via UART, I2C, or UDP

## Building

### Linux

Install dependencies, then build:

```bash
# Debian/Ubuntu/Raspberry Pi OS
sudo apt-get install build-essential libmosquitto-dev nlohmann-json3-dev

git clone https://github.com/g4klx/MMDVMHost.git
cd MMDVMHost
make
```

### macOS

Install dependencies via Homebrew, then build:

```bash
brew install mosquitto nlohmann-json

git clone https://github.com/g4klx/MMDVMHost.git
cd MMDVMHost
make
```

The Makefile auto-detects macOS via `uname -s` and adjusts include/library paths for Homebrew (both `/usr/local` and `/opt/homebrew` for Apple Silicon).

### Windows (Visual Studio 2022)

Open `MMDVMHost.sln` in Visual Studio 2022. Two external libraries are required: Eclipse Mosquitto and nlohmann/json.

**Option A: vcpkg (recommended)**

vcpkg handles both dependencies and integrates directly with MSBuild:

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install mosquitto:x64-windows nlohmann-json:x64-windows
```

After `vcpkg integrate install`, Visual Studio will automatically find both libraries. Build the solution normally (Debug or Release, x64).

**Option B: Manual installation**

1. **Mosquitto**: Download and run the official installer from [mosquitto.org/download](https://mosquitto.org/download/). The installer places headers and link libraries in `C:\Program Files\mosquitto\devel\` — this is the path the `.vcxproj` already references. **Note**: Mosquitto is *not* available as a NuGet package. The official installer is the only manual option.

2. **nlohmann/json**: Install via NuGet in Visual Studio:
   - Right-click the project in Solution Explorer → **Manage NuGet Packages**
   - Search for `nlohmann.json` and install it
   - This provides `#include <nlohmann/json.hpp>` automatically

   Alternatively, download the single header `json.hpp` from [the GitHub releases](https://github.com/nlohmann/json/releases) and place it at `C:\Program Files\nlohmann\json.hpp` (this path is already in the `.vcxproj` include directories).

Build the solution (Build → Build Solution, or Ctrl+Shift+B).

### Build Notes

The build auto-detects the git revision and embeds it in the binary via `GitVersion.h`.

## Installation

### Manual

```bash
sudo make install
```

This installs the `MMDVMHost` binary to `/usr/local/bin/`.

### systemd Service

```bash
sudo make install-service
```

This will:
1. Install the binary to `/usr/local/bin/`
2. Create a system user `mmdvm` in the `dialout` group
3. Copy `MMDVMHost.ini` to `/etc/MMDVMHost.ini` (with daemon mode and log path configured)
4. Create `/var/log/mmdvm/` for log output
5. Install and enable the `mmdvmhost.service` systemd unit

To remove:

```bash
sudo make uninstall-service
```

## Configuration

All configuration is in a single INI file (`MMDVMHost.ini`). Each section is described below.

### General

```ini
[General]
Callsign=G9BF          # Your amateur radio callsign (required)
Id=123456               # Your DMR/CCS7 ID (required)
Timeout=180             # TX timeout in seconds
Duplex=1                # 1=full duplex (repeater), 0=simplex (hotspot)
RFModeHang=10           # Seconds to stay in RF-activated mode after traffic ends
NetModeHang=3           # Seconds to stay in network-activated mode after traffic ends
Daemon=0                # 1=run as background daemon, 0=foreground
```

`RFModeHang` and `NetModeHang` control how long the repeater remains in a particular mode after the last transmission ends, preventing premature mode switches. `ModeHang` sets both RF and Net hang to the same value.

### Info

```ini
[Info]
RXFrequency=435000000   # Receive frequency in Hz
TXFrequency=435000000   # Transmit frequency in Hz
Power=1                 # Transmit power in watts
Latitude=0.0            # Repeater latitude (for DMR direct connect)
Longitude=0.0           # Repeater longitude
Height=0                # Antenna height in metres
Location=Nowhere        # Text description of location
Description=Multi-Mode Repeater
URL=www.google.co.uk    # Info URL
```

Latitude, Longitude, Height, Location, Description, and URL are only needed for direct connections to DMR masters (not via a gateway).

### Log

```ini
[Log]
MQTTLevel=1             # MQTT log level (0=none, 1=debug ... 6=fatal)
DisplayLevel=1          # Console log level (0=none, 1=debug ... 6=fatal)
```

Log levels:
- **0** - No logging
- **1** - Debug
- **2** - Message
- **3** - Info
- **4** - Warning
- **5** - Error
- **6** - Fatal

Logs are published to MQTT and/or displayed on the console depending on the configured levels.

### MQTT

```ini
[MQTT]
Host=127.0.0.1          # MQTT broker host
Port=1883               # MQTT broker port
Auth=0                  # 1=use authentication, 0=anonymous
Username=mmdvm          # MQTT username (when Auth=1)
Password=mmdvm          # MQTT password (when Auth=1)
Keepalive=60            # MQTT keepalive interval in seconds
Name=mmdvm              # MQTT client name (used as topic prefix)
```

MQTT is used for structured JSON logging and remote control command reception. All log messages and mode state changes are published as JSON to MQTT topics under the configured `Name` prefix.

### CW ID

```ini
[CW Id]
Enable=1                # 1=enable CW ID transmission
Time=10                 # CW ID interval in minutes
# Callsign=            # Override callsign for CW ID (defaults to [General] Callsign)
```

Periodically transmits the station callsign in Morse code (CW) for regulatory identification.

### ID Lookups

```ini
[DMR Id Lookup]
File=DMRIds.dat         # Path to DMR ID database file
Time=24                 # Reload interval in hours

[NXDN Id Lookup]
File=NXDN.csv           # Path to NXDN ID database file
Time=24                 # Reload interval in hours
```

These databases map numeric radio IDs to callsigns and user names. Used for display and logging purposes. The DMR ID file is also used by P25 mode. See [Helper Scripts](#helper-scripts) for automatic updating.

### Modem

```ini
[Modem]
Protocol=null           # Connection protocol: "null", "uart", "udp", or "i2c" (Linux only)
# UART settings
UARTPort=/dev/ttyACM0   # Serial port device
UARTSpeed=115200        # Baud rate
# I2C settings
I2CPort=/dev/i2c        # I2C device
I2CAddress=0x22         # I2C slave address
# UDP settings
ModemAddress=192.168.2.100
ModemPort=3334
LocalAddress=192.168.2.1
LocalPort=3335
```

**Signal parameters:**

| Parameter | Description | Default |
|-----------|-------------|---------|
| `TXInvert` | Invert TX signal polarity | 1 |
| `RXInvert` | Invert RX signal polarity | 0 |
| `PTTInvert` | Invert PTT signal | 0 |
| `TXDelay` | TX key-up delay in ms | 100 |
| `RXOffset` | RX frequency offset in Hz | 0 |
| `TXOffset` | TX frequency offset in Hz | 0 |
| `DMRDelay` | DMR-specific TX delay | 0 |
| `RXLevel` | RX signal level (0-100) | 50 |
| `TXLevel` | TX signal level (0-100) | 50 |
| `RXDCOffset` | RX DC offset correction | 0 |
| `TXDCOffset` | TX DC offset correction | 0 |
| `RFLevel` | Overall RF output level (0-100) | 100 |

**Per-mode TX levels** (optional overrides of `TXLevel`):

```ini
# CWIdTXLevel=50
# D-StarTXLevel=50
# DMRTXLevel=50
# YSFTXLevel=50
# P25TXLevel=50
# NXDNTXLevel=50
# POCSAGTXLevel=50
# FMTXLevel=50
```

**Diagnostics:**

```ini
RSSIMappingFile=RSSI.dat  # RSSI calibration data file
UseCOSAsLockout=0         # Use Carrier Operated Switch as lockout
Trace=0                   # 1=enable modem data tracing
Debug=0                   # 1=enable modem debug output
```

### Transparent Data

```ini
[Transparent Data]
Enable=0                   # 1=enable transparent data passthrough
RemoteAddress=127.0.0.1    # Remote UDP address
RemotePort=40094           # Remote UDP port
LocalPort=40095            # Local UDP port
# SendFrameType=0          # Include frame type byte in transparent data
```

Allows raw data passthrough between the modem and an external application via UDP.

### D-Star

```ini
[D-Star]
Enable=0                # 1=enable D-Star mode
Module=C                # D-Star module letter (A-D, typically C for 70cm)
SelfOnly=0              # 1=only accept transmissions from own callsign
AckReply=1              # 1=send acknowledgement replies
AckTime=750             # Ack response delay in ms
AckMessage=0            # Ack type: 0=BER, 1=RSSI, 2=S-Meter
ErrorReply=1            # 1=send error replies for invalid headers
RemoteGateway=0         # 1=gateway is on a remote machine
# ModeHang=10           # Override mode hang time for D-Star
WhiteList=              # Comma-separated allowed callsigns (empty=all)
```

### DMR

```ini
[DMR]
Enable=1                # 1=enable DMR mode
Beacons=0               # 0=off, 1=network beacons, 2=timed beacons
BeaconInterval=60       # Beacon interval in seconds
BeaconDuration=3        # Beacon duration in seconds
ColorCode=1             # DMR color code (0-15)
SelfOnly=0              # 1=only accept own ID
# MinSrcId=10000        # Minimum allowed source DMR ID (default 10000)
EmbeddedLCOnly=0        # 1=only accept embedded link control
DumpTAData=1            # 1=display talker alias data
# Prefixes=234,235      # Allowed DMR ID prefixes
# Slot1TGWhiteList=     # Comma-separated allowed TGs on slot 1
# Slot2TGWhiteList=     # Comma-separated allowed TGs on slot 2
CallHang=3              # DMR call hang time in seconds
TXHang=4                # DMR TX hang time in seconds
# ModeHang=10           # Override mode hang time for DMR
# OVCM=0               # Over-the-air Voice Channel Mode: 0=off, 1=RX, 2=TX, 3=both, 4=force off
# MQTTVoice=0           # Publish AMBE voice frames (base64) to MQTT after FEC
# MQTTData=0            # Publish data payloads (base64) to MQTT after FEC
```

DMR operates in 2-slot TDMA mode. In duplex mode, both slots are active simultaneously. Access control supports blacklists, whitelists, prefix filtering, and per-slot talkgroup whitelists via `CDMRAccessControl`.

### System Fusion

```ini
[System Fusion]
Enable=0                # 1=enable Yaesu System Fusion mode
LowDeviation=0          # 1=use low deviation mode
SelfOnly=0              # 1=only accept own callsign
TXHang=4                # TX hang time in seconds
RemoteGateway=0         # 1=gateway is on a remote machine
# ModeHang=10           # Override mode hang time
```

### P25

```ini
[P25]
Enable=0                # 1=enable P25 Phase 1 mode
NAC=293                 # Network Access Code (hex, 0x000-0xFFF)
SelfOnly=0              # 1=only accept own ID
OverrideUIDCheck=0      # 1=bypass UID validation
RemoteGateway=0         # 1=gateway is on a remote machine
TXHang=5                # TX hang time in seconds
# ModeHang=10           # Override mode hang time
```

### NXDN

```ini
[NXDN]
Enable=0                # 1=enable NXDN mode
RAN=1                   # Radio Access Number (1-64)
SelfOnly=0              # 1=only accept own ID
RemoteGateway=0         # 1=gateway is on a remote machine
TXHang=5                # TX hang time in seconds
# ModeHang=10           # Override mode hang time
```

NXDN supports both Icom and Kenwood network protocols, selectable in the network section.

### POCSAG

```ini
[POCSAG]
Enable=1                # 1=enable POCSAG paging mode
Frequency=439987500     # POCSAG transmit frequency in Hz
```

POCSAG is a receive-from-network/transmit-only mode used for paging. Messages are received from the DAPNET gateway and transmitted over the air.

### FM

```ini
[FM]
Enable=1                # 1=enable analog FM mode
# Callsign=            # Override callsign for FM CW ID
CallsignSpeed=20        # CW ID speed in WPM
CallsignFrequency=1000  # CW ID tone frequency in Hz
CallsignTime=10         # CW ID interval in minutes
CallsignHoldoff=0       # Delay before first CW ID in minutes
CallsignHighLevel=50    # CW ID high audio level
CallsignLowLevel=20     # CW ID low audio level
CallsignAtStart=1       # 1=send CW ID at start of transmission
CallsignAtEnd=1         # 1=send CW ID at end of transmission
CallsignAtLatch=0       # 1=send CW ID when latch activates
RFAck=K                 # RF acknowledgement type (K, N, etc.)
ExtAck=N                # External acknowledgement type
AckSpeed=20             # Ack CW speed in WPM
AckFrequency=1750       # Ack tone frequency in Hz
AckMinTime=4            # Minimum time before ack in seconds
AckDelay=1000           # Ack delay in ms
AckLevel=50             # Ack audio level
# Timeout=180           # FM-specific timeout (overrides General timeout)
TimeoutLevel=80         # Timeout tone level
CTCSSFrequency=88.4     # CTCSS tone frequency in Hz
CTCSSThreshold=30       # CTCSS detection threshold
# CTCSSHighThreshold=30 # CTCSS high detection threshold (hysteresis)
# CTCSSLowThreshold=20  # CTCSS low detection threshold (hysteresis)
CTCSSLevel=20           # CTCSS encode level
KerchunkTime=0          # Minimum TX time to activate in seconds (0=disabled)
HangTime=7              # Repeater hang time in seconds
AccessMode=1            # Access mode (see below)
LinkMode=0              # 1=minimal logic control (link mode)
COSInvert=0             # 1=invert Carrier Operated Switch
NoiseSquelch=0          # 1=enable noise squelch
SquelchThreshold=30     # Noise squelch threshold
# SquelchHighThreshold=30
# SquelchLowThreshold=20
RFAudioBoost=1          # RF audio gain multiplier
MaxDevLevel=90          # Maximum deviation level
ExtAudioBoost=1         # External (network) audio gain multiplier
# ModeHang=10           # Override mode hang time
```

**FM Access Modes:**

| Value | Mode |
|-------|------|
| 0 | Carrier access with COS |
| 1 | CTCSS only access without COS |
| 2 | CTCSS only access with COS |
| 3 | CTCSS to start, then carrier access with COS |

FM audio processing includes configurable IIR pre-emphasis and de-emphasis filters plus a 3-stage bandpass filter chain.

### Network Sections

Each mode has a corresponding network section for gateway connectivity. All use UDP:

```ini
[D-Star Network]
Enable=0
LocalAddress=127.0.0.1
LocalPort=20011
GatewayAddress=127.0.0.1
GatewayPort=20010
# ModeHang=3            # Override network mode hang time
Debug=0

[DMR Network]
Enable=0
LocalAddress=127.0.0.1
LocalPort=62032
GatewayAddress=127.0.0.1
GatewayPort=62031
Jitter=360              # Jitter buffer size in ms
Slot1=1                 # 1=enable slot 1 networking
Slot2=1                 # 1=enable slot 2 networking
# ModeHang=3
Debug=0

[System Fusion Network]
Enable=0
LocalAddress=127.0.0.1
LocalPort=3200
GatewayAddress=127.0.0.1
GatewayPort=4200
# ModeHang=3
Debug=0

[P25 Network]
Enable=0
LocalAddress=127.0.0.1
LocalPort=32010
GatewayAddress=127.0.0.1
GatewayPort=42020
# ModeHang=3
Debug=0

[NXDN Network]
Enable=0
Protocol=Icom           # "Icom" or "Kenwood"
LocalAddress=127.0.0.1
LocalPort=14021
GatewayAddress=127.0.0.1
GatewayPort=14020
# ModeHang=3
Debug=0

[POCSAG Network]
Enable=0
LocalAddress=127.0.0.1
LocalPort=3800
GatewayAddress=127.0.0.1
GatewayPort=4800
# ModeHang=3
Debug=0

[FM Network]
Enable=1
LocalAddress=127.0.0.1
LocalPort=3810
GatewayAddress=127.0.0.1
GatewayPort=4810
PreEmphasis=1           # 1=apply pre-emphasis to network audio
DeEmphasis=1            # 1=apply de-emphasis to received RF audio
TXAudioGain=1.0         # Network TX audio gain
RXAudioGain=1.0         # Network RX audio gain
# ModeHang=3
Debug=0
```

All network communication uses `CUDPSocket` for IPv4/IPv6 UDP transport.

### Lock File

```ini
[Lock File]
Enable=0                # 1=write a lock file indicating active mode
File=/tmp/MMDVM_Active.lck
```

When enabled, writes the current active mode to a file for external monitoring.

### Remote Control

```ini
[Remote Control]
Enable=0                # 1=enable MQTT-based remote control
```

See [Remote Control Commands](#remote-control-commands) for the command list.

## Running

### Foreground

```bash
MMDVMHost /path/to/MMDVMHost.ini
```

If no config file is specified, it defaults to `MMDVMHost.ini` in the current directory.

### Daemon / systemd

With `Daemon=1` in the config file, MMDVMHost forks to background.

Using systemd:

```bash
sudo systemctl start mmdvmhost
sudo systemctl stop mmdvmhost
sudo systemctl status mmdvmhost
sudo journalctl -u mmdvmhost -f    # Follow logs
```

### Docker

A `docker-compose.yml` is provided in the `linux/` directory:

```bash
cd linux
docker-compose up -d
```

Mount your configuration, RSSI data, and DMR ID files as volumes. Pass through the modem serial device.

## Architecture

```
                    ┌─────────────────────────────────────────────────┐
                    │                  MMDVMHost                       │
                    │                                                  │
  RF/Modem side     │   ┌──────────┐    ┌──────────────────────┐      │    Network side
                    │   │          │    │   Mode Controllers    │      │
  ┌──────────┐     │   │          │    │                       │      │    ┌──────────────┐
  │  MMDVM   │◄───►│   │  CModem  │◄──►│  CDStarControl       │◄────►│───►│ DStarNetwork │
  │ Hardware  │     │   │          │    │  CDMRControl         │      │    │ DMRNetwork   │
  │ (DVMega,  │     │   │          │    │  CYSFControl         │      │    │ YSFNetwork   │
  │  ZUMspot, │     │   │          │    │  CP25Control         │      │    │ P25Network   │
  │  etc.)    │     │   │          │    │  CNXDNControl        │      │    │ NXDNNetwork  │
  └──────────┘     │   │          │    │  CPOCSAGControl      │      │    │ POCSAGNetwork│
                    │   │          │    │  CFMControl          │      │    │ FMNetwork    │
                    │   └──────────┘    └──────────────────────┘      │    └──────────────┘
                    │                                                  │         │
                    │   ┌──────────┐    ┌──────────────────────┐      │         │
                    │   │   CConf  │    │  Support Services     │      │         ▼
                    │   │  (INI    │    │                       │      │    ┌──────────────┐
                    │   │  parser) │    │  CDMRLookup           │      │    │   Gateways   │
                    │   └──────────┘    │  CNXDNLookup          │      │    │ (D-Star GW,  │
                    │                   │  CUserDB              │      │    │  DMR GW,     │
                    │   ┌──────────┐    │  CRemoteControl       │      │    │  YSF GW,     │
                    │   │   Log    │    │  CMQTTConnection      │      │    │  P25 GW,     │
                    │   │  + MQTT  │    └──────────────────────┘      │    │  etc.)        │
                    │   └──────────┘                                   │    └──────────────┘
                    └─────────────────────────────────────────────────┘
```

### Core Components

- **`CMMDVMHost`** - Main application class. Owns the modem, all mode controllers, and all network connections. Runs the main event loop, dispatching data between the modem and networks, managing mode switching, and handling CW ID timing.

- **`CModem`** - Abstraction over the physical modem hardware. Communicates via `IModemPort` (UART, I2C, UDP, or null). Uses ring buffers (`CRingBuffer`) for async data flow. Manages modem configuration, frequency setting, and per-mode TX/RX data queues.

- **`CConf`** - INI file parser. Reads all configuration into typed accessors.

- **Mode Controllers** (`CDStarControl`, `CDMRControl`, `CYSFControl`, `CP25Control`, `CNXDNControl`, `CPOCSAGControl`, `CFMControl`) - Each handles the protocol-specific logic for one mode: RF data processing, network data processing, state machines, error correction, and display updates.

- **Network Classes** (`CDStarNetwork`, `CDMRNetwork`, `CYSFNetwork`, `CP25Network`, `INXDNNetwork`, `CPOCSAGNetwork`, `CFMNetwork`) - UDP-based network interfaces to their respective gateways. NXDN uses an abstract interface (`INXDNNetwork`) with Icom and Kenwood implementations.

- **`CMQTTConnection`** - Mosquitto-based MQTT client for publishing JSON log data and receiving remote control commands.

- **`CRemoteControl`** - Parses MQTT-received commands into `REMOTE_COMMAND` enum values for mode switching, enable/disable, paging, and status queries.

### Data Flow

1. **RF to Network**: Modem receives RF data -> mode controller decodes/processes -> network class transmits to gateway
2. **Network to RF**: Network class receives from gateway -> mode controller encodes/processes -> modem transmits over RF
3. **Mode switching**: Main loop monitors activity timers and hang timers to switch between active modes. Only one digital mode is active at a time (except in duplex DMR which uses both timeslots).

### Key Support Classes

| Class | Purpose |
|-------|---------|
| `CRingBuffer<T>` | Thread-safe circular buffer for async data transfer |
| `CTimer` | Millisecond-resolution countdown timer |
| `CStopWatch` | Elapsed time measurement |
| `CThread` | Platform-abstracted threading |
| `CUDPSocket` | IPv4/IPv6 UDP networking |
| `CLog` | Leveled logging to console and MQTT |
| `CUserDB` | CSV-based radio ID to callsign lookup database |
| `CDMRLookup` / `CNXDNLookup` | Mode-specific ID lookup with periodic reload |
| `CRSSIInterpolator` | RSSI ADC-to-dBm mapping from calibration data |
| `CDMRAccessControl` | DMR blacklist/whitelist/talkgroup filtering |
| `CGolay24128` | Golay (24,12,8) error correction |
| `CRS129` / `CRS634717` | Reed-Solomon error correction codecs |
| `CBPTC19696` | Block Product Turbo Code for DMR |
| `CIIRDirectForm1Filter` | IIR audio filter (pre/de-emphasis, bandpass) |

## Compile-Time Mode Selection

Individual modes can be compiled out by commenting the relevant `#define` in `Defines.h`:

```cpp
#define USE_DSTAR
#define USE_DMR
#define USE_YSF
#define USE_P25
#define USE_NXDN
#define USE_POCSAG
#define USE_FM
```

This reduces binary size and resource usage for single-mode deployments.

## Remote Control Commands

When remote control is enabled, commands are received via MQTT. Available commands:

| Command | Description |
|---------|-------------|
| `mode idle` | Switch to idle mode |
| `mode lockout` | Lock out all modes |
| `mode dstar` | Force D-Star mode |
| `mode dmr` | Force DMR mode |
| `mode ysf` | Force System Fusion mode |
| `mode p25` | Force P25 mode |
| `mode nxdn` | Force NXDN mode |
| `mode fm` | Force FM mode |
| `enable <mode>` | Enable a mode |
| `disable <mode>` | Disable a mode |
| `page <RIC> <message>` | Send POCSAG alphanumeric page |
| `page_bcd <RIC> <digits>` | Send POCSAG BCD numeric page |
| `page_a1 <RIC> <message>` | Send POCSAG alert type 1 |
| `page_a2 <RIC> <message>` | Send POCSAG alert type 2 |
| `cw` | Trigger CW ID |
| `reload` | Reload configuration |
| `connection_status` | Query network connection status |
| `config_hosts` | Query configured network hosts |

## MQTT Deep Dive

MMDVMHost uses MQTT (via the Mosquitto library) as its primary integration bus. All structured telemetry, log output, remote control, and display data flow through MQTT topics. This enables dashboards, external automation, logging pipelines, and remote management without modifying MMDVMHost itself.

### Connection and Topic Namespace

On startup, MMDVMHost connects to the MQTT broker as client `MMDVMHost.<pid>` (e.g. `MMDVMHost.12345`).

All topics are namespaced under the `Name` configured in `[MQTT]`. For a topic `T` and `Name=mmdvm`, the actual MQTT topic is `mmdvm/T`. If a topic string already contains `/`, it is used as-is (no prefix added).

**Configuration reference:**

```ini
[MQTT]
Host=127.0.0.1          # MQTT broker host
Port=1883               # MQTT broker port
Auth=0                  # 1=use username/password authentication
Username=mmdvm          # Broker username (when Auth=1)
Password=mmdvm          # Broker password (when Auth=1)
Keepalive=60            # MQTT keepalive interval in seconds (minimum 5)
Name=mmdvm              # Topic prefix and client identifier
```

**QoS**: All messages are published and subscriptions are made at QoS 2 (exactly once) by default.

**Reconnection**: The Mosquitto client library handles automatic reconnection. On disconnect, the `m_connected` flag is cleared and publishes are silently dropped until the connection is restored.

### Published Topics

MMDVMHost publishes to four topic categories:

| Topic | Payload Type | Description |
|-------|-------------|-------------|
| `{name}/log` | Plain text | Timestamped log messages (filtered by `MQTTLevel`) |
| `{name}/json` | JSON | Structured telemetry from all mode controllers |
| `{name}/response` | Plain text | Responses to remote control commands (`OK` or `KO`) |
| `{name}/display-out` | Binary | Raw serial data from the modem's display port |

#### Log Messages (`{name}/log`)

Published when a log event meets or exceeds the configured `MQTTLevel`. Format:

```
L: YYYY-MM-DD HH:MM:SS.mmm <message text>
```

Where `L` is the level character:

| Character | Level | Value |
|-----------|-------|-------|
| `D` | Debug | 1 |
| `M` | Message | 2 |
| `I` | Info | 3 |
| `W` | Warning | 4 |
| `E` | Error | 5 |
| `F` | Fatal | 6 |

**Example:**
```
M: 2025-03-15 14:22:01.345 DMR Slot 1, received RF voice header from VK2ABC to TG 505
```

Timestamps are in UTC.

#### Structured JSON (`{name}/json`)

All JSON payloads are wrapped in a top-level key identifying the subsystem:

```json
{"<TopLevel>": { ...payload... }}
```

The `<TopLevel>` key identifies the source. See the [JSON Payload Reference](#json-payload-reference) for all top-level keys and their schemas.

#### Command Responses (`{name}/response`)

After each remote control command, a response is published:

- `OK` - Command recognized and accepted
- `KO` - Command invalid or rejected
- For `status` and `hosts` commands, the response contains the requested data as a formatted string

#### Display Data (`{name}/display-out`)

Raw binary data received from the modem's serial/display interface, forwarded as-is for external display processing.

### Subscribed Topics

MMDVMHost subscribes to two topics for inbound data:

| Topic | Payload Type | Description |
|-------|-------------|-------------|
| `{name}/command` | Plain text | Remote control commands (only when `[Remote Control] Enable=1`) |
| `{name}/display-in` | Binary | Raw data to send to the modem's serial/display port |

#### Command Input (`{name}/command`)

Text commands parsed by `CRemoteControl`. See [Remote Control Commands](#remote-control-commands) for the full command list. Command syntax:

```
<verb> [<arg1> [<arg2> ...]]
```

**Examples:**
```
mode idle
mode dmr
enable dstar
disable fm
page 1234567 Hello world
page_bcd 1234567 5551234
page_a1 1234567
cw VK2ABC
reload
status
hosts
```

**Mode command with timeout:** `mode dmr 120` (force DMR for 120 seconds)
**Mode command fixed:** `mode dmr fixed` (force DMR until explicitly changed)

#### Display Input (`{name}/display-in`)

Binary data forwarded to the modem's serial interface for driving external displays (e.g. Nextion, OLED). Messages up to 5000 bytes are accepted; longer messages are split into 252-byte chunks sent at timed intervals.

### JSON Payload Reference

All JSON payloads include a `timestamp` field in ISO-like format. Each top-level key represents a different subsystem.

#### `MMDVM` - Host Mode Changes

Published when MMDVMHost switches modes or emits host-level messages.

**Mode change:**
```json
{
  "MMDVM": {
    "timestamp": "2025-03-15T14:22:01.345",
    "mode": "DMR"
  }
}
```

Possible `mode` values: `"Idle"`, `"D-Star"`, `"DMR"`, `"YSF"`, `"P25"`, `"NXDN"`, `"POCSAG"`, `"FM"`, `"Lockout"`, `"Error"`

**Host message:**
```json
{
  "MMDVM": {
    "timestamp": "2025-03-15T14:22:01.345",
    "message": "MMDVMHost is running"
  }
}
```

#### `RSSI` - Signal Strength

Published periodically during active transmissions when RSSI data is available. Emitted by all digital modes and FM.

```json
{
  "RSSI": {
    "timestamp": "2025-03-15T14:22:01.345",
    "mode": "DMR",
    "value": -65
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `mode` | string | `"DMR"`, `"D-Star"`, `"YSF"`, `"P25"`, `"NXDN"`, or `"FM"` |
| `value` | int | Average RSSI in dBm over the sampling window |
| `slot` | int | DMR slot number (1 or 2) - **DMR only** |

#### `BER` - Bit Error Rate

Published periodically during active digital transmissions.

```json
{
  "BER": {
    "timestamp": "2025-03-15T14:22:01.345",
    "mode": "DMR",
    "value": 1.23
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `mode` | string | `"DMR"`, `"D-Star"`, `"YSF"`, `"P25"`, or `"NXDN"` |
| `value` | float | BER as a percentage (0.0 - 100.0) |
| `slot` | int | DMR slot number (1 or 2) - **DMR only** |

#### `Text` - Slow Data Text

Published when text data is decoded from slow data streams (D-Star slow data, DMR talker alias).

```json
{
  "Text": {
    "timestamp": "2025-03-15T14:22:01.345",
    "mode": "DMR",
    "value": "VK2ABC Spencer"
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `mode` | string | `"DMR"` or `"D-Star"` |
| `value` | string | Decoded text content |
| `slot` | int | DMR slot number - **DMR only** |

#### `DMR` - DMR Activity

Published for voice, data, and CSBK events on DMR slots.

**Voice start (RF):**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "rf",
    "action": "start",
    "slot": 1,
    "src_id": 5051234,
    "dst_id": 505,
    "group": "yes",
    "src_info": "VK2ABC Spencer"
  }
}
```

**Voice end (RF, with RSSI):**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "slot": 1,
    "duration": 14.3,
    "ber": 0.85,
    "rssi": {
      "min": -72,
      "max": -58,
      "ave": -65
    }
  }
}
```

**Voice end (RF, without RSSI):**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "slot": 1,
    "duration": 14.3,
    "ber": 0.85
  }
}
```

**Transmission lost:**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "lost",
    "slot": 1,
    "duration": 5.2,
    "ber": 3.45
  }
}
```

**Network voice start:**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "network",
    "action": "start",
    "slot": 2,
    "src_id": 3101234,
    "dst_id": 91,
    "group": "yes",
    "src_info": "DL1ABC Hans"
  }
}
```

**Network voice end:**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "slot": 2,
    "duration": 8.7,
    "loss": 0.5,
    "ber": 1.2
  }
}
```

**Data transmission start:**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "rf",
    "action": "start",
    "slot": 1,
    "src_id": 5051234,
    "dst_id": 505,
    "group": "yes",
    "src_info": "VK2ABC",
    "frames": 4
  }
}
```

**User/TG rejected:**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "rf",
    "action": "rejected",
    "slot": 1,
    "src_id": 9999999,
    "dst_id": 505,
    "group": "yes",
    "src_info": ""
  }
}
```

**CSBK event:**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "rf",
    "action": "csbk",
    "slot": 1,
    "src_id": 5051234,
    "dst_id": 5055678,
    "group": "no",
    "src_info": "VK2ABC",
    "csbk_desc": "Call Alert"
  }
}
```

CSBK descriptions: `"Unit to Unit Voice Service Request"`, `"Unit to Unit Voice Answer Response"`, `"Negative Acknowledgment Response"`, `"Preamble"`, `"Call Alert"`, `"Call Alert Ack"`, `"Radio Check"`, `"Call Emergency"`

**DMR field reference:**

| Field | Type | Present | Description |
|-------|------|---------|-------------|
| `timestamp` | string | Always | UTC timestamp |
| `action` | string | Always | `"start"`, `"end"`, `"lost"`, `"rejected"`, `"csbk"` |
| `slot` | int | Always | 1 or 2 |
| `source` | string | Start/reject/csbk | `"rf"` or `"network"` |
| `src_id` | int | Start/reject/csbk | Source DMR ID |
| `dst_id` | int | Start/reject/csbk | Destination ID (talkgroup or private) |
| `group` | string | Start/reject/csbk | `"yes"` = group call, `"no"` = private call |
| `src_info` | string | Start/reject/csbk | Looked-up callsign + name |
| `duration` | float | End/lost | Transmission duration in seconds |
| `ber` | float | End/lost (RF) | Bit error rate percentage |
| `loss` | float | End (network) | Frame loss percentage |
| `rssi` | object | End/lost (RF, when available) | `{min, max, ave}` in dBm |
| `frames` | int | Data start | Number of data blocks |
| `csbk_desc` | string | CSBK events | Human-readable CSBK description |

**AMBE voice frame (when `MQTTVoice=1`):**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:01.345",
    "type": "voice",
    "slot": 1,
    "source": "rf",
    "sequence": 0,
    "data": "B5dV/X3fdf9w..."
  }
}
```

Published for every voice frame after FEC correction. The `data` field contains the full 33-byte DMR frame (including AMBE payload) as base64. `sequence` is 0 for sync frames, 1-5 for voice frames. `source` is `"rf"` or `"network"`.

**Data payload (when `MQTTData=1`):**
```json
{
  "DMR": {
    "timestamp": "2025-03-15T14:22:01.345",
    "type": "data",
    "slot": 1,
    "source": "rf",
    "rate": "1/2",
    "data": "AQIDBAU..."
  }
}
```

Published for every data block after FEC decoding. The `data` field is the decoded payload as base64: 12 bytes for rate 1/2 (BPTC), 18 bytes for rate 3/4 (trellis), or 24 bytes for rate 1/1 (unprotected). `rate` is `"1/2"`, `"3/4"`, or `"1/1"`.

#### `D-Star` - D-Star Activity

**RF voice start:**
```json
{
  "D-Star": {
    "timestamp": "2025-03-15T14:22:01.345",
    "src_callsign": "VK2ABC",
    "src_ext": "VK2A",
    "dst_callsign": "CQCQCQ",
    "source": "rf",
    "action": "start",
    "reflector": ""
  }
}
```

**Network voice start:**
```json
{
  "D-Star": {
    "timestamp": "2025-03-15T14:22:01.345",
    "src_callsign": "DL1ABC",
    "src_ext": "DL1A",
    "dst_callsign": "VK2ABC",
    "source": "network",
    "action": "start",
    "reflector": "REF001 C"
  }
}
```

**RF voice end (with RSSI):**
```json
{
  "D-Star": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "duration": 12.5,
    "ber": 0.42,
    "rssi": {
      "min": -68,
      "max": -55,
      "ave": -62
    }
  }
}
```

**Network voice end:**
```json
{
  "D-Star": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "duration": 8.3,
    "loss": 1.2
  }
}
```

**D-Star field reference:**

| Field | Type | Description |
|-------|------|-------------|
| `src_callsign` | string | 8-char source callsign (MY1) |
| `src_ext` | string | 4-char source extension (MY2) |
| `dst_callsign` | string | 8-char destination callsign (YOUR) |
| `source` | string | `"rf"` or `"network"` |
| `action` | string | `"start"`, `"end"`, `"lost"` |
| `reflector` | string | 8-char reflector callsign (network only, empty if none) |
| `duration` | float | Seconds |
| `ber` | float | BER percentage (RF) |
| `loss` | float | Frame loss percentage (network) |
| `rssi` | object | `{min, max, ave}` in dBm (RF, when available) |

#### `YSF` - System Fusion Activity

**RF voice start:**
```json
{
  "YSF": {
    "timestamp": "2025-03-15T14:22:01.345",
    "src_callsign": "VK2ABC",
    "source": "rf",
    "action": "start",
    "dg-id": 0,
    "mode": "V/D Mode 2",
    "reflector": ""
  }
}
```

**Network voice start:**
```json
{
  "YSF": {
    "timestamp": "2025-03-15T14:22:01.345",
    "src_callsign": "DL1ABC",
    "source": "network",
    "action": "start",
    "dg-id": 10,
    "reflector": "DE Germany"
  }
}
```

**RF voice end:**
```json
{
  "YSF": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "duration": 10.2,
    "ber": 1.5,
    "rssi": {
      "min": -70,
      "max": -60,
      "ave": -65
    }
  }
}
```

**Network voice end:**
```json
{
  "YSF": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "duration": 7.8,
    "loss": 2
  }
}
```

**YSF field reference:**

| Field | Type | Description |
|-------|------|-------------|
| `src_callsign` | string | 10-char source callsign (trimmed) |
| `source` | string | `"rf"` or `"network"` |
| `action` | string | `"start"`, `"end"`, `"lost"` |
| `dg-id` | int | DG-ID number |
| `mode` | string | YSF mode (RF start only, e.g. `"V/D Mode 2"`) |
| `reflector` | string | Reflector name (network only) |
| `duration` | float | Seconds |
| `ber` | float | BER percentage (RF) |
| `loss` | int | Lost frame count (network) |
| `rssi` | object | `{min, max, ave}` in dBm |

#### `P25` - P25 Activity

**RF voice start:**
```json
{
  "P25": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "rf",
    "action": "start",
    "src_id": 5051234,
    "dst_id": 10200,
    "group": "yes",
    "src_info": "VK2ABC Spencer"
  }
}
```

**RF voice end:**
```json
{
  "P25": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "duration": 9.1,
    "ber": 0.6,
    "rssi": {
      "min": -71,
      "max": -59,
      "ave": -64
    }
  }
}
```

**Network voice end:**
```json
{
  "P25": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "duration": 6.5,
    "loss": 0.8
  }
}
```

P25 uses the same field schema as DMR (minus `slot` and `csbk_desc`).

#### `NXDN` - NXDN Activity

**RF voice start:**
```json
{
  "NXDN": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "rf",
    "action": "start",
    "src_id": 12345,
    "dst_id": 100,
    "group": "yes",
    "src_info": "VK2ABC"
  }
}
```

**Network voice end with frame count:**
```json
{
  "NXDN": {
    "timestamp": "2025-03-15T14:22:15.678",
    "source": "network",
    "action": "end",
    "src_id": 54321,
    "dst_id": 200,
    "group": "yes",
    "src_info": "DL1ABC",
    "frames": 48
  }
}
```

**RF voice end:**
```json
{
  "NXDN": {
    "timestamp": "2025-03-15T14:22:15.678",
    "action": "end",
    "duration": 5.4,
    "ber": 2.1,
    "rssi": {
      "min": -75,
      "max": -62,
      "ave": -68
    }
  }
}
```

NXDN uses 16-bit IDs (0-65535) instead of 24-bit DMR IDs.

#### `POCSAG` - POCSAG Paging

**Page transmitted:**
```json
{
  "POCSAG": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "network",
    "ric": 1234567,
    "functional": "Alphanumeric",
    "message": "Hello World"
  }
}
```

**Alert page (no message body):**
```json
{
  "POCSAG": {
    "timestamp": "2025-03-15T14:22:01.345",
    "source": "network",
    "ric": 1234567,
    "functional": "Alert1"
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `source` | string | Always `"network"` (POCSAG is receive-from-network only) |
| `ric` | int | Receiver Identity Code (pager address) |
| `functional` | string | `"Alphanumeric"`, `"Numeric"`, `"Alert1"`, `"Alert2"` |
| `message` | string | Page content (absent for alert-only pages) |

#### `FM` - FM Analog Activity

**State change:**
```json
{
  "FM": {
    "timestamp": "2025-03-15T14:22:01.345",
    "state": "listening"
  }
}
```

Possible `state` values: `"listening"`, `"kerchunk"`, `"to_rf_audio"`, `"rf_audio"`, `"relaying_rf_audio"`, `"timeout"`, `"to_network_audio"`, `"network_audio"`

FM also publishes `RSSI` events (see the `RSSI` section above) but does not publish `BER` events since FM is an analog mode.

### MQTT Integration Examples

#### Subscribe to All JSON Telemetry

```bash
mosquitto_sub -h 127.0.0.1 -t "mmdvm/json" -v
```

#### Subscribe to Log Messages

```bash
mosquitto_sub -h 127.0.0.1 -t "mmdvm/log" -v
```

#### Send a Remote Control Command

```bash
# Switch to idle
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "mode idle"

# Force DMR mode for 60 seconds
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "mode dmr 60"

# Send a POCSAG page
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "page 1234567 Meeting at 3pm"

# Query connection status
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "status"

# Trigger CW ID
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "cw VK2ABC"

# Reload configuration
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "reload"

# Enable/disable modes
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "enable dstar"
mosquitto_pub -h 127.0.0.1 -t "mmdvm/command" -m "disable fm"
```

#### Monitor Command Responses

```bash
mosquitto_sub -h 127.0.0.1 -t "mmdvm/response" -v
```

#### Python: Parse JSON Telemetry

```python
import json
import paho.mqtt.client as mqtt

def on_message(client, userdata, msg):
    if msg.topic.endswith("/json"):
        data = json.loads(msg.payload)
        for key, value in data.items():
            if key == "DMR":
                action = value.get("action")
                if action == "start":
                    print(f"DMR Slot {value['slot']}: {value['src_info']} -> "
                          f"{'TG ' if value['group'] == 'yes' else ''}{value['dst_id']}")
                elif action == "end":
                    print(f"DMR Slot {value['slot']}: ended, "
                          f"{value.get('duration', 0):.1f}s, BER: {value.get('ber', 0):.1f}%")
            elif key == "RSSI":
                print(f"{value['mode']} RSSI: {value['value']} dBm")

client = mqtt.Client()
client.on_message = on_message
client.connect("127.0.0.1", 1883)
client.subscribe("mmdvm/json")
client.loop_forever()
```

#### Node-RED / Home Assistant

Subscribe to `mmdvm/json` and use a JSON node to parse. Key paths for common automations:

- **Active mode**: `MMDVM.mode`
- **Who's talking (DMR)**: `DMR.src_info`, `DMR.src_id`
- **Signal quality**: `RSSI.value`, `BER.value`
- **Paging activity**: `POCSAG.ric`, `POCSAG.message`

#### Filtering by Mode with Mosquitto

```bash
# Watch only DMR events
mosquitto_sub -h 127.0.0.1 -t "mmdvm/json" | grep '"DMR"'

# Watch RSSI across all modes
mosquitto_sub -h 127.0.0.1 -t "mmdvm/json" | grep '"RSSI"'
```

#### Multiple Instances

Run multiple MMDVMHost instances with different `Name` values to keep topics separate:

```ini
# Instance 1
[MQTT]
Name=repeater-70cm

# Instance 2
[MQTT]
Name=repeater-2m
```

Subscribe to all: `mosquitto_sub -t "+/json" -v`

## Helper Scripts

Located in `linux/`:

| Script | Description |
|--------|-------------|
| `DMRIDUpdate.sh` | Updates `DMRIds.dat` from a DMR ID database (for cron) |
| `DMRIDUpdateBM.sh` | Updates `DMRIds.dat` from BrandMeister (for cron) |
| `tg_shutdown.sh` | Automated shutdown on receipt of a specific talkgroup |

Example cron entry for daily DMR ID updates:

```bash
0 3 * * * /path/to/DMRIDUpdate.sh
```

## Hardware Support

MMDVMHost auto-detects the connected hardware type:

| Hardware | Enum Value | Notes |
|----------|-----------|-------|
| MMDVM | `MMDVM` | Full-size repeater board |
| DVMega | `DVMEGA` | Raspberry Pi hat (D-Star, DMR, YSF) |
| ZUMspot | `MMDVM_ZUMSPOT` | Hotspot board |
| MMDVM_HS_HAT | `MMDVM_HS_HAT` | Single-band hotspot hat |
| MMDVM_HS_DUAL_HAT | `MMDVM_HS_DUAL_HAT` | Dual-band hotspot hat |
| Nano Hotspot | `NANO_HOTSPOT` | Compact hotspot |
| Nano DV | `NANO_DV` | Compact digital voice board |
| D2RG MMDVM_HS | `D2RG_MMDVM_HS` | D2RG variant |
| MMDVM_HS | `MMDVM_HS` | Generic hotspot |
| OpenGD77 HS | `OPENGD77_HS` | OpenGD77 firmware hotspot |
| SkyBridge | `SKYBRIDGE` | SkyBridge board |

### Modem Connection Types

| Protocol | Interface | Use Case |
|----------|-----------|----------|
| `uart` | Serial/USB (e.g. `/dev/ttyACM0`) | Most common - direct USB connection |
| `i2c` | I2C bus (Linux only) | Raspberry Pi GPIO hat |
| `udp` | UDP over IP | Network-attached or remote modem |
| `null` | No hardware | Testing/development |

## License

This software is licensed under the **GNU General Public License v2** (GPL-2.0).

Copyright (C) 2015-2025 by Jonathan Naylor G4KLX and contributors.

This software is primarily intended for amateur and educational use.
