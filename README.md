# PalmIQ
Designed a biometric system that scans unique palm vein patterns for secure, contactless authentication. The solution supports seamless digital payments, provides instant access to health records, logs location and timestamps, and transmits all data to a centralized web server for real-time processing and monitoring.


**📌 Overview**

This project explores a futuristic and secure biometric authentication system that uses palm vein patterns for identity verification, digital payments, and real-time health monitoring. Palm vein recognition is one of the most secure biometric methods because it relies on sub-dermal vein patterns captured using Near-Infrared (NIR) imaging, ensuring high accuracy and resistance to spoofing.
Due to the high cost and limited availability of contactless NIR palm-vein cameras, the current prototype focuses on the health sensing and data-logging components, implemented using affordable and easily available sensors. The system captures real-time health data, attaches GPS location and timestamp, and uploads the result to a web server for monitoring.

This README documents both:
The final proposed concept, and
The current prototype implementation.

**💡 Project Concept (Final Vision)**

A contactless palm scanning system capable of:

Detecting palm vein patterns from the user’s hand.
-> Performing high-security authentication without physical contact.
-> Enabling secure payments via biometric identity.
-> Providing instant access to health records.
-> Attaching location + timestamp for every authentication/event.
-> Uploading data to a central web server for analysis and storage.

This concept combines biometrics + IoT + health monitoring + real-time data systems into a single integrated platform.

**Current Prototype Implementation**

Since contactless palm-vein imaging requires specialized and expensive NIR hardware (Fujitsu PalmSecure, NIR modules, custom IR filters), the prototype uses affordable sensors to demonstrate health monitoring + data transmission functionality using:

Hardware Used:
ESP32 or Raspberry Pi 3B (Controller + WiFi)
MAX30102 Sensor – Heart rate & SpO₂ monitoring (PPG-based)
MLX90614 – Non-contact temperature sensor
Neo-6M GPS Module – Provides real-time latitude & longitude
RTC Module (DS3231/DS1307) – Accurate system timestamps
Breadboard, wires, power supply – Supporting components

Software Used:
Arduino / Python (based on ESP32 or RPi)
HTTP communication
Sensor libraries:
   MAX30102
   MLX90614
   TinyGPS++
   RTC libraries
   JSON data formatting
   Server backend (Node.js etc.)

**System Workflow**
1. Physical Hand Placement (Prototype)
The user places their palm/finger on the health-sensing module.
(Currently physical contact; planned upgrade → contactless NIR scanning.)

2. Health Data Acquisition
   Sensors collect:
     Heart Rate (BPM)
     Blood Oxygen (SpO₂ %)
     Body Temperature (°C)
   
4. Location & Timestamp Logging
    Neo-6M GPS provides latitude & longitude.
    RTC module adds accurate date/time to each reading.

5. Data Packaging & Transmission

Collected data is sent through the ESP32/RPi to a web server via: HTTP

6. Web Dashboard / Server Storage

The server displays:
   Live health readings
   Location map
   Update history
   Diagnostic records

(Frontend + backend implementation depends on user preference.)

**Features**(Prototype)

✔ Real-time heart rate & SpO₂ monitoring
✔ temperature measurement
✔ GPS-based location tagging
✔ Accurate timestamps for every reading
✔ Automatic data upload to web server
✔ IoT integration for logging and analytics
✔ Expandable architecture for future palm-vein scanning

**Use Cases**

Secure healthcare authentication
Attendance + health logs
Fitness monitoring with geo-tracking
Smart payments using biometric
Contactless smart kiosks
Emergency medical record retrieval

**Conclusion**
This project demonstrates the core data-logging and health-sensing modules of a future contactless palm-vein payment and health authentication system. While the full NIR palm-vein scanning is pending specialized hardware, our prototype successfully implements the essential IoT, sensor integration, GPS logging, and server communication components. Future enhancements will enable fully contactless biometric authentication, making the system practical for payments, healthcare, and secure identity systems.
