## TinyTAP for Wireshark

TinyTAP is a lightweight embedded tool that monitors Modbus RTU serial communication and streams frame-level raw data in real time – ready for analysis via Wireshark.

It offers a smarter way to observe and filter serial traffic – ideal for diagnostics, development, and reverse engineering of Modbus-based systems.

> No virtual COM ports. No device drivers. Just clean data on the loopback interface.

A minimal CLI tool receives the raw stream from TinyTAP and forwards it to the loopback interface.


## License

This project is licensed under the [BSD 3-Clause License](license.txt).
See the LICENSE file for full text and conditions.

## Motivation

There are already several approaches for capturing and analyzing Modbus RTU frames, but none of them met my expectations. Since Wireshark is the de facto standard for Ethernet analysis, my first idea was: why not analyze serial data with Wireshark as well?

Unfortunately, reality had other plans. Virtual COM ports, special drivers, and convoluted setups turned the process into a frustrating experience.

**TinyTAP** was born out of a desire to simplify that, with an embedded-friendly approach that delivers raw, usable data directly. No driver headaches, no unnecessary complexity.

Yes, it involves a small embedded board and a CLI tool, but everything is designed to work seamlessly together. Hardware, firmware and tooling come from a single source and are tested as a complete system. That’s what makes it simple.

The required hardware is affordable:
- **FRDM-MCXN947** development board from NXP (~30 €)
- **RS485 8 Click Board™** from MIKROE (~20 €)

Whether you're debugging, reverse engineering, or developing, TinyTAP offers a transparent and streamlined solution that integrates smoothly into existing Wireshark workflows.


## Why "Tiny"?

The name "Tiny" doesn't mean limited or minimal, it stands for **just the functionality I actually needed**.

It all started with a project called **TinyCTS/AL**: a combination of a Cooperative Task Scheduler (CTS) and an Abstraction Layer (AL) to unify access to peripherals like GPIO, UART, or CAN across different CPUs. You can find more about it [here](https://www.emb4fun.de/projects/tctsal/index.html).

Later, I built **TinyONE**, which extended the foundation with a lightweight TCP/IP stack (lwIP) and a web server (MicroHTTP), including TLS support (via MbedTLS) and login functionality. TinyONE became the base for all my Ethernet- and web-based applications.

**TinyTAP** continues this philosophy: focused, embedded-friendly, and built with only the features that matter, no bloat, just purpose.


---

## Project Structure

- `tinytap/` - The internal name is "frdmn947-mbrtu-tap"
  - `bootloader/` – Embedded bootloader source code
  - `firmware/` – Main embedded firmware
  - `webpage/` – Embedded web interface (HTML/CSS/JS)
  - `tools/tinytap/` – Cross-platform CLI tool (Linux/macOS/Windows)
    - `src/` – Source code (.c)
    - `include/` – Header files (.h)
    - `build.sh` – Build script
    - `CMakeLists.txt` – Build configuration
    - `readme.md` – How to build the CLI tool
    - `license.txt` – BSD license for this component
  - `tools/tne/` – Tiny Network Explorer
  - `license.txt` – BSD 3-Clause License for the overall project
  - `readme.md` – Project overview and motivation


---


## Development Notes

**Toolchain:**
The embedded firmware is developed using [SEGGER Embedded Studio](https://www.segger.com/products/development-tools/embedded-studio/), which is available free of charge for non-commercial use under SEGGER’s [Friendly License](https://www.segger.com/products/development-tools/embedded-studio/license/licensing-conditions/).

**CLI Tool:**
The CLI tool is available for **Windows**, **macOS**, and **Linux**, built with CMake for cross-platform support.
A simple Bash script (`build.sh`) handles platform-specific builds and optimizations.


---

## Project Status

**TinyTAP** is currently under active development. Many core components are already functional, but several features and refinements are still in progress.

If you're curious, feel free to explore the code or follow the progress.


---
---

## Some notes about Mbed TLS
Mbed TLS is used in .\source\common\library\mbedtls and was copied from the following project:
https://github.com/ARMmbed/mbedtls

## Notes about NXP Software Components

This project uses software components from the MCUXpresso SDK for the FRDM-MCXN947 board by NXP,
including the `els_pkc` module which provides cryptographic functionality.

These components are subject to the LA_OPT_NXP_Software_License and may only be used in connection
with NXP hardware. Redistribution or modification of these components outside this context is not
permitted.

The original directory structure and license file from the MCUXpresso SDK have been preserved.
For more details, refer to: https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html