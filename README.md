# IoT Saline Monitoring and Alert System

An IoT-based system that continuously monitors the saline level in an IV bottle and alerts caregivers when it runs low. It reduces the need for constant manual checking, helps prevent air from entering the IV line, and improves patient safety.

**Developed by:** Saline Solutions Pvt. Ltd.

---

## Features

- Real-time saline level monitoring
- Low-level alert when the level drops below a set threshold
- Live web dashboard showing the current status
- Buzzer / visual alert at the bedside *(edit to match your build)*
- WiFi-enabled data transmission

## How It Works

1. A sensor measures the remaining saline level in the bottle.
2. The microcontroller reads the sensor data.
3. The data is sent over WiFi to the web dashboard.
4. The dashboard displays the live level.
5. When the level falls below the threshold, an alert is triggered so staff can replace the bottle in time.

```
[Saline Bottle] -> [Sensor] -> [Microcontroller] -> WiFi -> [Web Dashboard]
                                      |
                                  [Buzzer / LED]
```

## Hardware Required

| Component | Purpose |
|---|---|
| ESP32 / NodeMCU (ESP8266) *(edit)* | Microcontroller with WiFi |
| Load cell + HX711 amplifier *(edit)* | Measures bottle weight / saline level |
| Buzzer | Audible alert |
| LED *(optional)* | Visual alert |
| Power supply / battery | Powers the device |
| Jumper wires, breadboard | Connections |

## Software and Libraries

- Arduino IDE
- ESP32 or ESP8266 board package *(edit)*
- Libraries *(edit to match your code's `#include` lines)*:
  - `WiFi.h` or `ESP8266WiFi.h`
  - `HX711.h`
  - `HTTPClient.h`

## Repository Structure

```
Saline-Solutions/
├── firmware/        # Arduino code for the microcontroller
├── website/         # Web dashboard
├── documentation/   # Reports, circuit diagrams, images
├── README.md
└── .gitignore
```

## Setup and Installation

### 1. Firmware

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add your board package (ESP32 or ESP8266) through **Boards Manager**.
3. Install the required libraries through **Library Manager**.
4. Open the sketch inside `firmware/`.
5. Create a file named `secrets.h` next to the sketch and add your own credentials:

   ```cpp
   #define WIFI_SSID "your_wifi_name"
   #define WIFI_PASSWORD "your_wifi_password"
   // add any API keys or server URLs here
   ```

   > `secrets.h` is listed in `.gitignore` and must never be pushed to GitHub.
6. Select your board and COM port, then click **Upload**.

### 2. Website

1. Open the `website/` folder.
2. Open `index.html` in a browser, or host it using GitHub Pages or any web server.
3. Update the server or API URL in the website code if needed.

## Wiring

| Sensor Pin | Microcontroller Pin |
|---|---|
| HX711 VCC | 3.3V / 5V *(edit)* |
| HX711 GND | GND |
| HX711 DT | GPIO __ *(edit)* |
| HX711 SCK | GPIO __ *(edit)* |
| Buzzer + | GPIO __ *(edit)* |
| Buzzer - | GND |

Add a circuit diagram image in `documentation/` and link it here:

```markdown
![Circuit Diagram](documentation/circuit-diagram.png)
```

## Screenshots

Add photos of your hardware and website here:

```markdown
![Dashboard](documentation/dashboard.png)
![Hardware Setup](documentation/hardware.jpg)
```

## Future Improvements

- SMS / WhatsApp / mobile app notifications
- Monitoring of multiple bottles from one dashboard
- Automatic flow-stop mechanism when the bottle is empty
- Cloud data logging and history

## Disclaimer

This project is a prototype for educational and demonstration purposes. It is not a certified medical device and should not replace professional medical supervision.

## Author

**Gowthami**
GitHub: [@gowthamimuchu-hub](https://github.com/gowthamimuchu-hub)