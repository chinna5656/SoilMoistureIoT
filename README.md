# 🌱 SmartSoilIrrigation

ระบบควบคุมการรดน้ำอัตโนมัติและบันทึกข้อมูลความชื้นในดิน  
พัฒนาโดยใช้ **ESP8266 + ADS1115** พร้อมบันทึกข้อมูลขึ้น **Google Sheets** ทุกชั่วโมง  
เหมาะสำหรับงาน **Smart Farm / IoT / Data Logging**

---

## 📌 Features
- อ่านค่าความชื้นดิน **4 จุด (S1–S4)** ผ่าน ADS1115
- คำนวณค่าเฉลี่ยความชื้น (AVG จาก S1–S3)
- ควบคุมปั๊มน้ำ **2 ชุด** ด้วยระบบ hysteresis
- แสดงผลแบบเรียลไทม์ผ่าน **LCD 16x2**
- บันทึกข้อมูลลง **Google Sheets อัตโนมัติทุก 1 ชั่วโมง**
- Timestamp จาก Google Server (แม่นยำ)
- รองรับการต่อยอดเป็น Smart Farm / Data Analysis

---

## 🧠 System Architecture

Soil Sensors (4)
│
▼
ADS1115 (I2C)
│
▼
ESP8266 (NodeMCU)
├── LCD Display
├── Relay Control (Pump 1 & 2)
└── WiFi → Google Apps Script → Google Sheets

---

## 🔧 Hardware Requirements
- ESP8266 (NodeMCU / Wemos D1 Mini)
- ADS1115 ADC (16-bit)
- Soil Moisture Sensor x4
- Relay Module x2
- Water Pump x2
- LCD 16x2 (I2C)
- Power Supply (ตามสเปกปั๊ม)

---

## 📦 Software Requirements
- Arduino IDE
- ESP8266 Board Package
- Libraries:
  - `Adafruit_ADS1X15`
  - `LiquidCrystal_I2C`
  - `ESP8266WiFi`
  - `ESP8266HTTPClient`

---

## ⚙️ Configuration

### 🔹 WiFi
```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
