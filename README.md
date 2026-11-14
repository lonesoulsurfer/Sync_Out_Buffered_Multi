# Sync Out: buffered multiple for modular synths

Sync Out takes one sync (clock) input and makes six sync outputs. You can set each output to a different clock division or groove. It uses an Arduino Nano and shows the tempo (BPM) on an OLED screen.

![Sync Out](https://github.com/user-attachments/assets/db6cac9d-b04d-4177-9726-467bc478f5b3)

## Build guide and video

- Step-by-step build guide on Instructables: [Buffered Multiple for Modular Synths - Powered by Arduino](https://www.instructables.com/Buffered-Multiple-for-Modular-Synths-Powered-by-Ar/)
- Video on YouTube: [Sync Out in action](https://youtu.be/81T7WBMwU_8)

## Features

- 1 sync input and 6 sync outputs
- The OLED screen shows the current BPM
- You can set each output to one of these modes:
  - Normal sync (1:1)
  - Clock division: 1/2, 1/4 or 1/8
  - Swing (GS): delays the off-beats by a fixed amount
  - Shuffle (GF): moves the off-beats toward a triplet feel
  - Humanize (GH): adds random timing changes
- Each groove mode has two levels: 50 % and 75 %. Thus there are six groove feels.

### The groove modes

| Mode | What it does |
|------|--------------|
| Swing (GS) | Delays each off-beat by the same amount, up to 66 % of the beat. At 50 % to 75 %, it gives a classic swing feel. |
| Shuffle (GF) | Moves the off-beats toward a 2-against-3 (triplet) rhythm. At 75 %, it is near a dotted-eighth feel. |
| Humanize (GH) | Adds random timing changes around the off-beats, up to 40 % of the beat. It gives a loose, organic feel. |

## What you need

### Hardware

- All parts are in the parts list ([PDF](Parts%20List.pdf) or [Excel](Parts%20List.xlsx)).
- The Gerber files for the PCB and the front panel are in [Gerber Files](Gerber%20Files/). You can send them to a PCB maker.

### Software

- Arduino IDE
- These libraries. Install them with the **Library Manager**:
  - **Adafruit GFX Library**
  - **Adafruit SSD1306**

## Upload the firmware

1. Download this repository. Click **Code → Download ZIP**, then extract the ZIP file.
2. In the Arduino IDE, install the libraries in [Software](#software).
3. Open `Sync_Out_Sketch/Sync_Out_Sketch.ino`.
4. Connect the Arduino Nano to your computer with a USB cable.
5. In the **Tools** menu, set these values:
   - **Board:** Arduino Nano
   - **Processor:** ATmega328P
   - **Port:** the COM port of your Nano
6. Click **Upload**.
7. Wait for the message **Done uploading**.

> **Note:** If the upload fails, set **Processor** to **ATmega328P (Old Bootloader)**, then try again. Many clone Nano boards need this setting.

## Connections

| Nano pin | Function |
|----------|----------|
| A0 | Sync input |
| D2 to D7 | Sync outputs 1 to 6 |
| D8 | Sync indicator LED |
| D9 | Select button (selects the output to change) |
| D10 | Divide button |
| D11 | Multiply button |

## Files in this repository

| Folder or file | Contents |
|----------------|----------|
| `Sync_Out_Sketch/` | Firmware |
| `Eagle Files/` | Eagle files for the PCB |
| `Front Panel Design/` | KiCad, SVG and DXF files for the front panel |
| `Gerber Files/` | Gerber files for the PCB and the front panel, ready to order |
| `Parts List.pdf`, `Parts List.xlsx` | Parts list |

## Support

Do you like this project? You can support my work on [GitHub Sponsors](https://github.com/sponsors/lonesoulsurfer).

## Licence

- **Firmware** (`Sync_Out_Sketch/`): [MIT License](LICENSE). You can use, change and sell it.
- **Hardware** (PCB, front panel and Gerber files): [CERN Open Hardware Licence v2, Strongly Reciprocal](LICENSE-HARDWARE). You can make, change and sell boards. If you share or sell a changed design, you must also share the changed design files under the same licence.

Copyright (c) 2025 Marcus (lonesoulsurfer).
