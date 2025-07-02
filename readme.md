![TinyTAP Banner](assets/tinytab-banner.jpg)

## TinyTAP for Wireshark

[![Project Status](https://img.shields.io/badge/status-in--development-yellow)](...)

TinyTAP is a lightweight embedded tool that monitors ModbusRTU serial communication and streams frame-level raw data in real time – ready for analysis via a custom Wireshark dissector.

It offers a smarter way to observe, filter, and forward serial traffic – ideal for diagnostics, development, and reverse engineering of Modbus-based systems.

> No virtual COM ports. No device drivers. Just clean data on the loopback interface.

A minimal CLI tool receives the raw stream from TinyTAP and forwards it to the loopback interface, where a dedicated Wireshark dissector (included in this project) interprets the data.


## License

This project is licensed under the [BSD 3-Clause License](license.txt).  
See the LICENSE file for full text and conditions.

## Motivation

There are already several approaches for capturing and analyzing ModbusRTU frames, but none of them met my expectations. Since Wireshark is the de facto standard for Ethernet analysis, my first idea was: why not analyze serial data with Wireshark as well?

Unfortunately, reality had other plans. Virtual COM ports, special drivers, and convoluted setups turned the process into a frustrating experience.

**TinyTAP** was born out of a desire to simplify that, with an embedded-friendly approach that delivers raw, usable data directly. No driver headaches, no unnecessary complexity.

Yes, it involves a small embedded board and a CLI tool, but everything is designed to work seamlessly together. Hardware, firmware, dissector, and tooling come from a single source and are tested as a complete system. That’s what makes it simple.

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


## Development Notes

**Toolchain:**
The embedded firmware is developed using [SEGGER Embedded Studio](https://www.segger.com/products/development-tools/embedded-studio/), which is available free of charge for non-commercial use under SEGGER’s [Friendly License](https://www.segger.com/products/development-tools/embedded-studio/license/licensing-conditions/).

**Dissector:**
The Wireshark dissector is implemented in **Lua**, making it easy to extend and portable across platforms. Lua support is built into Wireshark and requires no additional compilation.

**CLI Tool:**
The first version of the CLI tool will be available for **Windows**. Other platforms may follow later.


---

## Project Status

**TinyTAP** is currently under active development. Many core components are already functional, but several features and refinements are still in progress. A first beta release is planned once key elements such as firmware, CLI tooling, and dissector reach a stable and tested state.

If you're curious, feel free to explore the code or follow the progress. Feedback, suggestions, or even contributions are always welcome!
