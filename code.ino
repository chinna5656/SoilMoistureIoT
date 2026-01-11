#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h> // ต้องใช้สำหรับ HTTPS

/* ================= WIFI ================= */
const char* ssid = "YOUR_WIFI_NAME";      // <== แก้ชื่อ WiFi
const char* password = "YOUR_WIFI_PASS";  // <== แก้รหัส WiFi

/* ================= GOOGLE SCRIPT ================= */
// ใส่ URL ที่ได้จาก Deploy Web App (ต้องลงท้ายด้วย /exec)
const char* scriptURL = "https://script.google.com/macros/s/xxxxxxxxx/exec";  //<==แก้ไข URL

/* ================= LCD ================= */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* ================= ADS1115 ================= */
Adafruit_ADS1115 ads;

/* ================= RELAY ================= */
#define RELAY1_PIN D5
#define RELAY2_PIN D6

/* ================= CALIBRATION (ค่าสอบเทียบ) ================= */
// ต้องหาค่าเหล่านี้จากการทดสอบจริง (ค่าแห้งสุด / ค่าแช่น้ำ)
int dry1 = 21000, wet1 = 7800;
int dry2 = 20900, wet2 = 7450;
int dry3 = 10640, wet3 = 7450;
int dry4 = 17500, wet4 = 7973;

/* ================= THRESHOLD (จุดทำงาน) ================= */
// ปรับให้กว้างขึ้นเพื่อป้องกันปั๊มกระชาก
#define PUMP1_ON_THRESHOLD   45  // เปิดปั๊มเมื่อต่ำกว่า 45%
#define PUMP1_OFF_THRESHOLD  70  // ปิดปั๊มเมื่อสูงกว่า 70% (รดจนชุ่ม)

#define PUMP2_ON_THRESHOLD   45
#define PUMP2_OFF_THRESHOLD  70

/* ================= TIME ================= */
#define READ_INTERVAL 2000
#define SEND_INTERVAL 3600000UL   // 1 ชั่วโมง

unsigned long lastReadTime = 0;
unsigned long lastSendTime = 0;

bool pump1Running = false;
bool pump2Running = false;

/* ================= FUNCTION ================= */
int toPercent(int raw, int dry, int wet) {
  // เซนเซอร์ Capacitive ส่วนใหญ่: ค่ามาก=แห้ง, ค่าน้อย=เปียก
  if (raw >= dry) return 0;
  if (raw <= wet) return 100;
  return map(raw, dry, wet, 0, 100);
}

void setup() {
  Serial.begin(115200); // แนะนำ 115200 สำหรับ ESP8266
  
  // ตั้งค่า Relay ให้เป็น High/Low ตาม Active (ส่วนใหญ่ Relay Module จะ Active LOW)
  // ถ้าใช้ Relay Active LOW (Low คือติด) ให้ตั้ง digitalWrite เป็น HIGH ไว้ก่อนเพื่อปิด
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW); // ปรับตาม Hardware จริง (Low/High)
  digitalWrite(RELAY2_PIN, LOW);

  Wire.begin(D2, D1); // SDA, SCL

  if (!ads.begin(0x48)) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  ads.setGain(GAIN_ONE);

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");

  WiFi.mode(WIFI_STA); // บังคับโหมด Station
  WiFi.begin(ssid, password);
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) { // รอแค่ 10 วินาที
    delay(500);
    lcd.print(".");
    timeout++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected");
    Serial.println("WiFi Connected");
  } else {
    lcd.print("WiFi Failed"); // ทำงานต่อแบบ Offline
    Serial.println("WiFi Failed");
  }
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // อ่านค่าเซนเซอร์และคุมปั๊ม
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;

    int raw1 = ads.readADC_SingleEnded(0);
    int raw2 = ads.readADC_SingleEnded(1);
    int raw3 = ads.readADC_SingleEnded(2);
    int raw4 = ads.readADC_SingleEnded(3);

    int s1 = toPercent(raw1, dry1, wet1);
    int s2 = toPercent(raw2, dry2, wet2);
    int s3 = toPercent(raw3, dry3, wet3);
    int s4 = toPercent(raw4, dry4, wet4);

    int avg = (s1 + s2 + s3) / 3;

    /* ===== PUMP CONTROL LOGIC (Hysteresis) ===== */
    // Pump 1 Logic
    if (avg < PUMP1_ON_THRESHOLD && !pump1Running) {
      digitalWrite(RELAY1_PIN, HIGH); // สั่งเปิดปั๊ม
      pump1Running = true;
      Serial.println("Pump 1: ON");
    } else if (avg >= PUMP1_OFF_THRESHOLD && pump1Running) {
      digitalWrite(RELAY1_PIN, LOW);  // สั่งปิดปั๊ม
      pump1Running = false;
      Serial.println("Pump 1: OFF");
    }

    // Pump 2 Logic
    if (s4 < PUMP2_ON_THRESHOLD && !pump2Running) {
      digitalWrite(RELAY2_PIN, HIGH);
      pump2Running = true;
      Serial.println("Pump 2: ON");
    } else if (s4 >= PUMP2_OFF_THRESHOLD && pump2Running) {
      digitalWrite(RELAY2_PIN, LOW);
      pump2Running = false;
      Serial.println("Pump 2: OFF");
    }

    /* ===== LCD ===== */
    lcd.setCursor(0, 0);
    // ใช้ sprintf เพื่อจัด Format ให้ข้อความไม่กระพริบหรือซ้อนกัน
    char line1[17];
    snprintf(line1, 17, "AVG:%3d%% P1:%s", avg, pump1Running ? "ON " : "OFF");
    lcd.print(line1);

    lcd.setCursor(0, 1);
    char line2[17];
    snprintf(line2, 17, "S4 :%3d%% P2:%s", s4, pump2Running ? "ON " : "OFF");
    lcd.print(line2);

    /* ===== SERIAL DEBUG ===== */
    Serial.printf("Raw: %d, %d, %d, %d | ", raw1, raw2, raw3, raw4);
    Serial.printf("Percent: %d, %d, %d, %d | AVG: %d\n", s1, s2, s3, s4, avg);
  }

  /* ===== SEND TO GOOGLE SHEET ===== */
  if (now - lastSendTime >= SEND_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) {
       lastSendTime = now;
       
       WiFiClientSecure client;
       client.setInsecure(); // ข้ามการเช็ค SSL Certificate (สำคัญมากสำหรับ ESP8266)
       
       HTTPClient https;
       
       // สร้าง URL
       String url = String(scriptURL) + 
                    "?s1=" + String(ads.readADC_SingleEnded(0)) + // ส่งค่า Raw หรือ % ก็ได้ตามต้องการ
                    "&s2=" + String(ads.readADC_SingleEnded(1)) +
                    "&s3=" + String(ads.readADC_SingleEnded(2)) +
                    "&avg=" + String((ads.readADC_SingleEnded(0)+ads.readADC_SingleEnded(1)+ads.readADC_SingleEnded(2))/3) +
                    "&s4=" + String(ads.readADC_SingleEnded(3));
       
       Serial.print("Sending data... ");
       
       https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // สำคัญ: Google Script มักจะ Redirect
       
       if (https.begin(client, url)) {
         int httpCode = https.GET();
         Serial.print("Code: ");
         Serial.println(httpCode); // 200 = สำเร็จ, 302 = Redirect, 404 = ผิด URL
         https.end();
       } else {
         Serial.println("Connection Failed");
       }
    }
  }
}