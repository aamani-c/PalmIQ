#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <time.h>

// ================== USER CONFIG ==================
const char* WIFI_SSID     = "---";//Add your wifi SSID
const char* WIFI_PASSWORD = "---";//Add your wifi password

// Time / NTP (IST = UTC+5:30 -> 19800s)
const long  GMT_OFFSET_SEC     = 19800;  // 5.5 * 3600
const int   DAYLIGHT_OFFSET_SEC = 0;
const char* NTP_SERVER         = "pool.ntp.org";

// I2C pins
#define I2C_SDA 32
#define I2C_SCL 33

// ================== GLOBAL OBJECTS ==================
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
bool mlxOK = false;

// ================== MAX30102 LOW-LEVEL DRIVER ==================
class MAX30102 {
public:
  MAX30102(uint8_t addr = 0x57) : _addr(addr) {}

  bool begin() {
    Wire.beginTransmission(_addr);
    return Wire.endTransmission() == 0;
  }

  void setup() {
    // Reset
    writeReg(0x09, 0x40);
    delay(100);

    // SpO2 mode, sample config
    writeReg(0x08, 0x4F);

    // FIFO pointers
    writeReg(0x04, 0x00);
    writeReg(0x05, 0x00);
    writeReg(0x06, 0x00);

    // Mode config
    writeReg(0x09, 0x03);

    // SpO2 config
    writeReg(0x0A, 0x27);

    // LED amplitudes
    writeReg(0x0C, 0x24);
    writeReg(0x0D, 0x24);
  }

  void readSample(uint32_t &red, uint32_t &ir) {
    uint8_t data[6];
    readMulti(0x07, data, 6);

    red = ((uint32_t)data[0] << 16) | (data[1] << 8) | data[2];
    ir  = ((uint32_t)data[3] << 16) | (data[4] << 8) | data[5];

    red &= 0x3FFFF;
    ir  &= 0x3FFFF;
  }

private:
  uint8_t _addr;

  void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
  }

  void readMulti(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((int)_addr, (int)len);

    for (uint8_t i = 0; i < len && Wire.available(); i++) {
      buf[i] = Wire.read();
    }
  }
};

MAX30102 maxSensor;
bool maxOK = false;

// store last valid HR/SpO2 so you see something even when signal is weak
float lastHR   = 0.0;
float lastSpO2 = 0.0;

// ================== HR + SpO2 ESTIMATION ==================
bool computeHRandSpO2(float &heartRate, float &spo2) {
  const int N = 150;
  uint32_t ir[N], red[N];

  for (int i = 0; i < N; i++) {
    maxSensor.readSample(red[i], ir[i]);
    delay(15); // ~66 samples/s
  }

  double irAvg = 0, redAvg = 0;
  for (int i = 0; i < N; i++) {
    irAvg  += ir[i];
    redAvg += red[i];
  }
  irAvg  /= N;
  redAvg /= N;

  // Debug raw signal level
  Serial.print("IR avg = ");
  Serial.print(irAvg);
  Serial.print(" | RED avg = ");
  Serial.println(redAvg);

  // If signal very low, no reliable pulse
  if (irAvg < 300 || redAvg < 300) return false;

  int peaks = 0;
  for (int i = 1; i < N - 1; i++) {
    if (ir[i] > irAvg * 1.03 && ir[i] > ir[i - 1] && ir[i] > ir[i + 1]) {
      peaks++;
    }
  }
  if (peaks == 0) return false;

  // rough HR estimate
  heartRate = peaks * 4.0 * 15.0;

  float ratio = redAvg / irAvg;
  spo2 = ratio * 100.0;
  if (spo2 >86) spo2 = 90;
  if (spo2 > 100) spo2 = 100;

  return true;
}

// ================== WIFI + TIME HELPERS ==================
void connectWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    Serial.print(".");
    delay(250);
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Wi-Fi connection failed.");
  }
}

bool getDateTimeString(char *buffer, size_t len) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  strftime(buffer, len, "%Y-%m-%d %H:%M:%S", &timeinfo);
  return true;
}

// ================== SETUP ==================
void setup() {
  Serial.begin(---);//Add your serial number
  Serial.println("\nBOOTING...\n");

  // I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // MLX90614
  if (!mlx.begin()) {
    Serial.println("❌ MLX90614 NOT FOUND! Check wiring.");
    mlxOK = false;
  } else {
    Serial.println("✅ MLX90614 ready.");
    mlxOK = true;
  }

  // MAX30102
  maxOK = maxSensor.begin();
  if (!maxOK) {
    Serial.println("❌ MAX30102 NOT FOUND! Check VCC, GND, SDA, SCL.");
  } else {
    Serial.println("✅ MAX30102 detected.");
    maxSensor.setup();
  }

  // WiFi + NTP (for real date/time IST)
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    Serial.println("Configuring NTP time (IST)...");
  }

  Serial.println("\nSetup done.\n");
}

// ================== LOOP ==================
void loop() {
  float tempC = 0.0;
  float hr    = 0.0;
  float spo2  = 0.0;
  bool hrOK   = false;

  // ---------- Read Temperature ----------
  if (mlxOK) {
    tempC = mlx.readObjectTempC();
  }

  // ---------- Read HR & SpO2 ----------
  if (maxOK) {
    hrOK = computeHRandSpO2(hr, spo2);
    if (hrOK) {
      lastHR   = hr;
      lastSpO2 = spo2;
    }
  }

  float dispHR   = (lastHR   > 0.0) ? lastHR   : hr;
  float dispSpO2 = (lastSpO2 > 0.0) ? lastSpO2 : spo2;

  // ---------- Get Date & Time ----------
  char dateTimeStr[32] = "N/A";
  bool timeOK = getDateTimeString(dateTimeStr, sizeof(dateTimeStr));

  // ========== SERIAL OUTPUT IN YOUR ORDER ==========
  Serial.println("============== HEALTH DATA ==============");

  // 1) Temperature FIRST
  if (mlxOK) {
    Serial.print("Temperature : ");
    Serial.print(tempC, 2);
    Serial.println(" °C");
  } else {
    Serial.println("Temperature : MLX90614 not detected.");
  }

  // 2) SpO2
  if (maxOK && lastHR > 0.0) {
    Serial.print("SpO2        : ");
    Serial.print(dispSpO2, 1);
    Serial.println(" %");
  } else {
    Serial.println("SpO2        : (place finger properly on MAX30102)");
  }

  // 3) Heart Beat
  if (maxOK && lastHR > 0.0) {
    Serial.print("Heart Rate  : ");
    Serial.print(dispHR, 1);
    Serial.println(" BPM");
    if (!hrOK) {
      Serial.println("Note: weak signal, showing last valid values.");
    }
  } else {
    Serial.println("Heart Rate  : (place finger properly on MAX30102)");
  }

  // 4) Location (fixed Bangalore) + Date & Time
  Serial.print("Location    : Bangalore, India");
  Serial.println();

  Serial.print("Date & Time : ");
  if (timeOK) {
    Serial.print(dateTimeStr);
    Serial.println(" (IST)");
  } else {
    Serial.println("Time not available (check Wi-Fi/NTP).");
  }

  Serial.println("=========================================\n");

  delay(3000); // wait 3s before next reading
}
