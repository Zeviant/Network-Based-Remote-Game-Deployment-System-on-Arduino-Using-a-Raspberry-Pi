# Network-Based Remote Game Deployment System
Using an Arduino Uno with a Raspberry Pi to simulate a simple networked game-console ecosystem

## Overview
This project demonstrates how to use a Raspberry Pi's networking and storage capabilities to support an Arduino Uno as a lightweight game console. The Raspberry Pi acts as a central server/storage for game binaries and assets, and the Arduino runs game firmware uploaded or delivered via the Pi. The repository includes Arduino sketches, Raspberry Pi scripts/tools, presentation slides, and the project report.

## How it works
- The Raspberry Pi stores multiple game builds (binaries) and exposes them over the network (for example, via a Flask server).
- A human operator selects a game and initiates a deployment to the Arduino.
- The Arduino receives code/data through a USB/serial connection
- The Arduino runs the uploaded firmware/game and outputs to the configured interface (display, LEDs, buttons, serial monitor, etc.). Input is handled via attached controls wired to the Arduino.

Note: Implementation details (exact scripts, uploader mechanism, and serial protocol) are located in the `Raspberry Pi/` and `Arduino/` folders. Consult those folders for the exact scripts and sketches.

## Repository layout
- Arduino/ — Arduino sketches and game source codes.
- Raspberry Pi/ — Server script, game binaries, and thumbnails.
- Videos/ — Demonstration videos for the project.
- GROUP 7 - SLIDES.pdf — Presentation slides describing the project, architecture and results.
- report_doc.pdf — Project report with detailed design, system evaluation, and testing outcomes.
- README.md — (this file) overview and setup instructions.

## Hardware required
Minimum hardware used in this project:
- Raspberry Pi
- Arduino Uno
- USB A-to-B cable (to connect Arduino to Raspberry Pi)
- Joystick, Button, LED, Buzzer, OLED modules
  
## Software requirements
- Raspberry Pi OS
- Python 3
- Arduino IDE (for compiling/uploading sketches)
- Network connectivity (Ethernet or Wi-Fi) for the Pi to host files or provide remote management
