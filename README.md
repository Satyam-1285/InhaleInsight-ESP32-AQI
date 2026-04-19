# InhaleInsight-ESP32-AQI# InhaleInsight: Smart AQI & Environmental Monitor

An ESP32-based Internet of Things (IoT) system designed to monitor Air Quality Index (AQI), temperature, humidity, and harmful pollutant levels. It features a real-time scrolling LED matrix display and live data logging to the cloud via ThingSpeak. 

## 🚀 Features

* **Real-Time Monitoring:** Tracks PM2.5 particles, temperature, humidity, and gas/smoke levels.
* **EPA Standard AQI Calculation:** Automatically converts raw PM2.5 data into the official EPA Air Quality Index scale.
* **Live Cloud Dashboard:** Streams real-time hardware and sensor vitals to ThingSpeak for remote monitoring.
* **Dynamic Visual Display:** Utilizes an 8-module MAX72xx LED Matrix to scroll through metrics and system statuses using the MD_Parola library.
* **Smart Connectivity:** Integrates `WiFiManager` to allow users to configure Wi-Fi credentials via a captive portal without hardcoding them into the device. Automatically switches to an offline AP mode if the connection fails.
* **Safety Alarms:** Triggers an active buzzer if smoke percentages exceed 80% or if the temperature crosses 35.0°C.

## 🛠️ Hardware Requirements

* ESP32 Microcontroller
* PMS5003 / PMS7003 Particulate Matter Sensor (connected via Serial2)
* DHT11 Temperature & Humidity Sensor
* MQ135 Gas Sensor
* 8x MAX7219 LED Matrix Modules (FC16 Hardware)
* Active Buzzer

## 💻 Software Dependencies

Ensure you have the following libraries installed in your Arduino IDE:
* `MD_Parola` & `MD_MAX72xx` (For LED Matrix animations)
* `PMS Library` (For PM sensor serial communication)
* `DHT sensor library` (By Adafruit)
* `WiFiManager` (For dynamic Wi-Fi provisioning)
* `ThingSpeak` (For cloud data logging)

## ⚙️ Setup & Configuration

1.  **Clone the Repository:** Download the project files to your local machine.
2.  **Hardware Assembly:** Connect the sensors and displays to the ESP32 according to the defined pins in the code (e.g., MQ135 to Pin 34, DHT11 to Pin 4, CS Pin to 5).
3.  **Cloud Setup:** * Create a [ThingSpeak](https://thingspeak.com/) account and set up a new channel with 6 fields (PM1.0, PM2.5, PM10, Gas%, Temp, Humidity).
    * Replace `myChannelNumber` and `myWriteAPIKey` in the code with your own channel credentials.
4.  **Upload:** Flash the `AQI_Meter.ino` code to your ESP32.
5.  **Wi-Fi Provisioning:** On the first boot, the device will host a Wi-Fi network named **"InhaleInsight"**. Connect to it using your smartphone or PC to configure your local Wi-Fi credentials. 

## 👥 Team & Credits

[cite_start]**Designed and developed by:** Radhika, Anurag, Satyam, and Raghvendra. [cite: 24]
[cite_start]**Guided by:** Prof. Deepti Malviya [cite: 23]
*Department of Electronics and Communication Engineering, Sagar Institute of Science and Technology (SISTec).*
