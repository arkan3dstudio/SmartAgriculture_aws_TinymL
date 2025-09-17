#include <Arduino.h>
#include <TinyML_Agronomi_inferencing.h>   // ini header utama library kamu
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <MQTTClient.h>
#include "daftarcuaca.h"
#include "rekom_decision_tree.h"
#include <Wire.h>

#define LED_INDICATOR 15  // Gunakan pin yang tersedia untuk LED
#define PIN_ULTRASONIK 23 // 
#define CHECK_INTERVAL_SEC 900  // Waktu tidur dalam detik 15 menit 

#define RXD2 25
#define TXD2 26
#define PKEY 33
#define RST 32

#define SerialMon Serial
#define SerialAT Serial2

const char apn[] = "Internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

const byte rxPin = 16; // RX2 - ke kaki 4 sensor (kabel putih)
const byte txPin = 17; // TX2 - ke kaki 3 sensor (kabel kuning)
// ---- Konfigurasi ----
float ketinggianSensor = 150.0;  // default awal cm, sesuaikan dengan tinggi sensor dari dasar tangki
HardwareSerial mySerial(1);


TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
MQTTClient mqtt(256);
HttpClient http(client, "api.openweathermap.org", 80);

const char* mqtt_broker = "202.10.44.229";
const int mqtt_port = 1884;
const char* mqtt_client_id = "esp32_client03_drtpm";
const char* mqtt_user = "pkmunimed";
const char* mqtt_pass = "pkmunimed123";
const char* mqtt_ctrl_ultrasonic_topic = "esp32_client03/drtpm/ultrasonic";
const char* mqtt_command_topic = "esp32_client03/drtpm/ultrasonic";

String URL = "http://api.openweathermap.org/data/2.5/weather?q=";
// Lokasi target (Tugu Kol)
String ApiKey = "c04aff3631a8224c57fd07bd2982b33b";
String lat = "3.427128";
String lon = "98.853176";
//3.427128, 98.853176

unsigned long lastCheck = 0;
const unsigned long checkInterval = 20000;

int8_t res_cmd(char* ATcommand, char* expected_answer, unsigned int timeout);
void send_at(const char* _command);
void wRespon(long waktu);
void connectMQTT();
void handleMQTTMessage(String &topic, String &payload);
String sendATWithResponse(String command);
String convertUnixTime(const int unixTime);
void ambilCuaca();

void blinkErrorLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_INDICATOR, HIGH);
    delay(200);
    digitalWrite(LED_INDICATOR, LOW);
    delay(200);
  }
}

void hidupkanModem() {
  pinMode(PKEY, OUTPUT);
  pinMode(RST, OUTPUT);

  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);
  digitalWrite(PKEY, HIGH);
  digitalWrite(RST, HIGH);
  delay(1000);
  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);
}

void matikanModem() {
  SerialAT.println("AT+CFUN=0"); // Set SIM7600 ke minimum functionality
  delay(200); // beri waktu
}

CuacaData lastCuacaData;

// ==== Edge Impulse TinyML ====
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
String hasilTinyML = "";

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void print_inference_result(ei_impulse_result_t result) {
    int max_index = 0;
    float max_value = 0;

    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_value) {
            max_value = result.classification[i].value;
            max_index = i;
        }
    }

    switch (max_index) {
case 0:
    hasilTinyML = "Kondisi lahan saat ini kemungkinan kekurangan air, sehingga disarankan untuk segera melakukan pengairan 💧";
    break;
case 1:
    hasilTinyML = "Kadar air pada lahan sudah mencukupi, sehingga tidak diperlukan penambahan pengairan ✅";
    break;
case 2:
    hasilTinyML = "Lahan mungkin dalam kondisi kelebihan air, sehingga lakukan perhentian pengairan sementara waktu 🚫";
    break;

    }
}


void deteksi_air(){
   if (mySerial.available())
    {
        static uint8_t recv_buf[10] = {0};
        uint16_t len = mySerial.readBytes((uint8_t *)&recv_buf, 4);

        if (len == 4 && recv_buf[0] == 0xff)
        {
            uint8_t sum = recv_buf[0] + recv_buf[1] + recv_buf[2];
            if (sum == recv_buf[3])
            {
                uint16_t distanceMM = (recv_buf[1] << 8) | recv_buf[2]; // hasil sensor dalam mm
                float distanceCM = distanceMM / 10.0; // ubah mm ke cm

                // Hitung ketinggian air
                float tinggiAir = ketinggianSensor - distanceCM;
                if (tinggiAir < 0) tinggiAir = 0; // supaya tidak negatif

                // Cetak hasil
                Serial.print("Jarak permukaan air: ");
                Serial.print(distanceCM, 1); // satu angka desimal
                Serial.println(" cm");

                Serial.print("Ketinggian air: ");
                Serial.print(tinggiAir, 1);
                Serial.println(" cm\n");
            }
        }
    }
}

void setup() {
  SerialMon.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, rxPin, txPin);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);
  delay(250);
  Wire.begin(); 
  hidupkanModem();  // <-- Tambahkan ini sebelum SerialAT.begin
  delay(500);
  SerialAT.begin(115200, SERIAL_8N1, RXD2, TXD2);
 
  pinMode(RST, OUTPUT);
  pinMode(PKEY, OUTPUT);
  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);
  digitalWrite(PKEY, HIGH);
  digitalWrite(RST, HIGH);
  delay(1000);
  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);

  SerialMon.println("Initializing modem...");
  Serial.println("waiting AT");
  byte _retry = 0;
  while (!res_cmd("AT", "OK", 1000)) {
  _retry++;
  Serial.println(_retry);
  if (_retry >= 20) {
    Serial.println("❌ Modem gagal merespons setelah 20 percobaan. Restart ESP...");
    delay(3000);
    ESP.restart(); // Restart ESP32
  }
}

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  delay(1000);

  send_at("AT+CREG?");
  send_at("AT+CSQ");
  send_at("AT+COPS?");
  send_at("AT+CPSI?");
  send_at("AT+CGDCONT=1,\"IP\",\"Internet\"");

  modem.gprsConnect(apn, gprsUser, gprsPass);
  if (!modem.isGprsConnected()) {
    SerialMon.println("[GPRS] Failed to connect");
    return;
  }
  SerialMon.println("[GPRS] Connected");
  delay(2000);

  mqtt.begin(mqtt_broker, mqtt_port, client);
  mqtt.setOptions(60, true, 1000);
  mqtt.onMessage(handleMQTTMessage);
  //connectMQTT();
  mqtt.subscribe(mqtt_ctrl_ultrasonic_topic);
  // Setelah modem setup dan MQTT connect:
  delay(1500);  // Tambahkan delay pendek agar I2C siap
}

void loop() {

  bool sukses = true;
  mqtt.begin(mqtt_broker, mqtt_port, client);
  if (!mqtt.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
    SerialMon.println("❌ MQTT gagal konek");
    blinkErrorLED(5);  // Indikator error
    sukses = false;
  } else {
    mqtt.subscribe(mqtt_ctrl_ultrasonic_topic);
    connectMQTT(); // Publish info modem
  }

  if (!modem.isGprsConnected()) {
    SerialMon.println("❌ GPRS gagal konek");
    blinkErrorLED(5);
    sukses = false;
  }

  if (sukses) {
   mqtt.disconnect();          // Putuskan koneksi MQTT
   delay(1000);
   ambilCuaca();               // Ambil dan proses data cuaca
   delay(1000);
   connectMQTT();
   delay(1000);

   // ✅ LED ON untuk indikasi sukses
   digitalWrite(LED_INDICATOR, HIGH);
   delay(3000);
   digitalWrite(LED_INDICATOR, LOW);
 }

  matikanModem();             // Matikan modem untuk hemat daya
  SerialMon.println("💤 Tidur selama " + String(CHECK_INTERVAL_SEC) + " detik");
  esp_sleep_enable_timer_wakeup(CHECK_INTERVAL_SEC * 1000000ULL);
  esp_deep_sleep_start();     // Masuk deep sleep
}

void publishCuacaData() {
  if (!lastCuacaData.valid) return;
   
  // Lokasi dan waktu
  String desa = "Desa Kelapa I, Kec.Galang, Kab.Deli Serdang";
  mqtt.publish("esp32_client03/drtpm/kota", desa.c_str(), true, 0);

  //mqtt.publish("esp32/data/status/ladang/kopi/kota", lastCuacaData.nama_kota.c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/tanggal_waktu", lastCuacaData.waktu.c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/sunrise", lastCuacaData.waktu_sunrise.c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/sunset", lastCuacaData.waktu_sunset.c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/prediksi_cuaca", lastCuacaData.cuaca_group.c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/deskripsi_cuaca", lastCuacaData.cuaca_deskripsi.c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/suhu_lokasi", (String(lastCuacaData.suhu) + " °C").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/feels_like", (String(lastCuacaData.feels_like) + " °C").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/temp_min", (String(lastCuacaData.temp_min) + " °C").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/temp_max", (String(lastCuacaData.temp_max) + " °C").c_str(), true, 0);
  //mqtt.publish("esp32_client03/drtpm/pressure", (String(lastCuacaData.pressure) + " hPa").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/kelembapan_lokasi", (String(lastCuacaData.humidity) + " %").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/tekanan_udara_lokasi", (String(lastCuacaData.pressure) + " hPa").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/visibility", (String(lastCuacaData.visibility) + " m").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/kec_angin_lokasi", (String(lastCuacaData.wind_speed) + " m/s").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/wind_deg", (String(lastCuacaData.wind_deg) + "°").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/arah_angin", lastCuacaData.arah_angin.c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/gust", (String(lastCuacaData.gust) + " m/s").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/rain1h", (String(lastCuacaData.rain1h) + " mm").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/cloud", (String(lastCuacaData.cloud) + " %").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/sea_level", (String(lastCuacaData.sea_level) + " hPa").c_str(), true, 0);
  mqtt.publish("esp32_client03/drtpm/grnd_level", (String(lastCuacaData.grnd_level) + " hPa").c_str(), true, 0);
 //mqtt.publish("esp32_client03/drtpm/keterangan_lokasi", lastCuacaData.keterangan_lokasi.c_str(), true, 0);

 // check deteksi air dengan sensor ultrasonik 
  if (mySerial.available())
    {
        static uint8_t recv_buf[10] = {0};
        uint16_t len = mySerial.readBytes((uint8_t *)&recv_buf, 4);

        if (len == 4 && recv_buf[0] == 0xff)
        {
            uint8_t sum = recv_buf[0] + recv_buf[1] + recv_buf[2];
            if (sum == recv_buf[3])
            {
                uint16_t distanceMM = (recv_buf[1] << 8) | recv_buf[2]; // hasil sensor dalam mm
                float distanceCM = distanceMM / 10.0; // ubah mm ke cm

                // Hitung ketinggian air
                float tinggiAir = ketinggianSensor - distanceCM;
                if (tinggiAir < 0) tinggiAir = 0; // supaya tidak negatif

                // Cetak hasil
                Serial.print("Jarak permukaan air: ");
                Serial.print(distanceCM, 1); // satu angka desimal
                Serial.println(" cm");

                Serial.print("Ketinggian air: ");
                Serial.print(tinggiAir, 1);
                Serial.println(" cm\n");
                mqtt.publish("esp32_client03/drtpm/tinggi_air", (String(tinggiAir) + " cm").c_str(), true, 0);
            }
        }
    }

 //check sensor lokasi
 
  if (lastCuacaData.valid && lastCuacaData.idCuaca != -1) {
  String saran = klasifikasiRekomendasi(
    lastCuacaData.idCuaca,
    lastCuacaData.suhu,
    lastCuacaData.humidity,
    lastCuacaData.wind_speed
  );
  mqtt.publish("esp32_client03/drtpm/saran_agronomis_lokasi", saran.c_str(), true, 0);
  SerialMon.println("📢 Rekomendasi Agronomis Dipublish: " + saran);
  }


    // ==== Update features untuk TinyML ====
    features[0] = lastCuacaData.suhu;        // suhu °C
    features[1] = lastCuacaData.humidity;    // kelembapan %
    features[2] = lastCuacaData.wind_speed;  // angin m/s
    features[3] = lastCuacaData.cloud;       // awan %

  // ==== Jalankan TinyML ====
  ei_impulse_result_t result = { 0 };
  signal_t features_signal;
  features_signal.total_length = sizeof(features) / sizeof(features[0]);
  features_signal.get_data = &raw_feature_get_data;

  EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
  if (res != EI_IMPULSE_OK) {
     SerialMon.printf("ERR: Failed to run classifier (%d)\n", res);
  } else {
     print_inference_result(result);
     SerialMon.println("=== HASIL TINYML ===");
     SerialMon.println(hasilTinyML);

     // ✅ publish hasil ke MQTT
     mqtt.publish("esp32_client03/drtpm/hasil_tinyml", hasilTinyML.c_str(), true, 0);
  }
}

void connectMQTT() {
  SerialMon.print("[MQTT] Connecting to broker...");
  while (!mqtt.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
    SerialMon.print(".");
    delay(1000);
  }
  SerialMon.println(" connected!");

  String info = modem.getModemInfo();
  mqtt.publish(mqtt_command_topic, info.c_str());
  SerialMon.println(info);
  // ✅ Publish ulang data terakhir jika tersedia
  publishCuacaData();
}

void send_at(const char* _command) {
  SerialAT.println(_command);
  unsigned long timeout = millis() + 3000;
  while (millis() < timeout) {
    while (SerialAT.available()) {
      SerialMon.write(SerialAT.read());
    }
  }
}

int8_t res_cmd(char* ATcommand, char* expected_answer, unsigned int timeout) {
  uint8_t x = 0, answer = 0;
  char response[100];
  unsigned long prevMillis;
  memset(response, '\0', 100);
  delay(100);

  while (SerialAT.available() > 0) SerialAT.read();
  SerialAT.println(ATcommand);
  prevMillis = millis();

  do {
    if (SerialAT.available() != 0) {
      response[x] = SerialAT.read();
      x++;
      if (strstr(response, expected_answer) != NULL) {
        answer = 1;
      }
    }
  } while ((answer == 0) && ((millis() - prevMillis) < timeout));
  return answer;
}

void wRespon(long waktu) {
  unsigned long cur_time = millis();
  unsigned long old_time = cur_time;
  while (cur_time - old_time < waktu) {
    cur_time = millis();
    while (SerialAT.available() > 0) {
      SerialMon.print(SerialAT.readString());
    }
  }
}

String sendATWithResponse(String command) {
  SerialAT.println(command);
  String response = "";
  unsigned long timeout = millis() + 2000;
  while (millis() < timeout) {
    while (SerialAT.available()) {
      response += SerialAT.readStringUntil('\n');
    }
  }
  SerialMon.println("[AT Response] " + response);
  return response;
}

void handleMQTTMessage(String &topic, String &payload) {
  SerialMon.print("[MQTT] Topic: ");
  SerialMon.println(topic);
  SerialMon.print("[MQTT] Payload: ");
  SerialMon.println(payload);

  if (topic == mqtt_ctrl_ultrasonic_topic) {
    // Kontrol pin ultrasonik berdasarkan payload
    if (payload == "1") {
      digitalWrite(PIN_ULTRASONIK, HIGH);
    } else {
      digitalWrite(PIN_ULTRASONIK, LOW);
    }
    // Kirim feedback ke topik perintah (optional, bisa diisi dengan pesan status)
    mqtt.publish(mqtt_command_topic, "OK");
  }
}


String convertUnixTime(const int unixTime) {
  time_t rawTime = unixTime;
  struct tm * timeinfo;
  timeinfo = gmtime(&rawTime);  // UTC
  timeinfo->tm_hour += 7;       // Konversi ke WIB

  // Penyesuaian tanggal jika jam melebihi 24
  if (timeinfo->tm_hour >= 24) {
    timeinfo->tm_hour -= 24;
    timeinfo->tm_mday += 1;
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d %b %Y %H:%M:%S", timeinfo);
  return String(buffer);
}

String convertUnixToHour(const int unixTime) {
  time_t rawTime = unixTime;
  struct tm * timeinfo;
  timeinfo = gmtime(&rawTime); // UTC
  timeinfo->tm_hour += 7;      // Konversi ke WIB

  // Atur penyesuaian jika jam melebihi 24
  if (timeinfo->tm_hour >= 24) {
    timeinfo->tm_hour -= 24;
    timeinfo->tm_mday += 1;
  }

  char buffer[10];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);  // hanya waktu
  return String(buffer);
}

String interpretArahAngin(int deg) {
  if (deg >= 337.5 || deg < 22.5) return "Utara";
  else if (deg >= 22.5 && deg < 67.5) return "Timur Laut";
  else if (deg >= 67.5 && deg < 112.5) return "Timur";
  else if (deg >= 112.5 && deg < 157.5) return "Tenggara";
  else if (deg >= 157.5 && deg < 202.5) return "Selatan";
  else if (deg >= 202.5 && deg < 247.5) return "Barat Daya";
  else if (deg >= 247.5 && deg < 292.5) return "Barat";
  else return "Barat Laut";
}

void ambilCuaca() {
  // Sensor Lokasi 
       
  // Bangun endpoint berdasarkan lat & lon
  String endpoint = "http://api.openweathermap.org/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&appid=" + ApiKey + "&units=metric";

  http.get(endpoint);
  int statusCode = http.responseStatusCode();
  String response = http.responseBody();
  http.stop();

  SerialMon.print("[HTTP] Status: ");
  SerialMon.println(statusCode);

  if (statusCode == 200) {
    SerialMon.println("[HTTP] Response: ");
    SerialMon.println(response);

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      SerialMon.print("[JSON] Deserialization failed: ");
      SerialMon.println(error.c_str());
      return;
    }
    // Data utama dari JSON
    float suhu = doc["main"]["temp"];
    float feels_like = doc["main"]["feels_like"];
    float temp_min = doc["main"]["temp_min"];
    float temp_max = doc["main"]["temp_max"];
    int pressure = doc["main"]["pressure"];
    int humidity = doc["main"]["humidity"];
    int visibility = doc["visibility"];
    float wind_speed = doc["wind"]["speed"];
    int wind_deg = doc["wind"]["deg"];
    float gust = doc["wind"]["gust"].isNull() ? 0.0 : doc["wind"]["gust"].as<float>();
    float rain1h = doc["rain"]["1h"].isNull() ? 0.0 : doc["rain"]["1h"].as<float>();
    int cloud = doc["clouds"]["all"];
    int code = doc["weather"][0]["id"];
    String main = doc["weather"][0]["main"];
    String deskripsi_en = doc["weather"][0]["description"];
    int timestamp = doc["dt"];
    int sea_level = doc["main"]["sea_level"];
    int grnd_level = doc["main"]["grnd_level"];
    int sunrise_ts = doc["sys"]["sunrise"];
    int sunset_ts = doc["sys"]["sunset"];
    String waktu = convertUnixTime(timestamp);
    String nama_kota = doc["name"];
    String negara = doc["sys"]["country"];
    String waktu_sunrise = convertUnixToHour(sunrise_ts);
    String waktu_sunset = convertUnixToHour(sunset_ts);
    String arah_angin = interpretArahAngin(wind_deg);

    // Ambil data cuaca dari daftarCuaca[]
    String cuaca_group = "-";
    String cuaca_deskripsi = "-";
    for (int i = 0; i < sizeof(daftarCuaca) / sizeof(daftarCuaca[0]); i++) {
      if (daftarCuaca[i].code == code) {
        cuaca_group = daftarCuaca[i].group;
        cuaca_deskripsi = daftarCuaca[i].deskripsi;
        break;
      }
    }

    // Cetak ke Serial
    SerialMon.println("============= INFO CUACA =============");
    SerialMon.print("Semua Data Cuaca Sudah Di Terima "); 
    SerialMon.println("======================================");

    // Simpan ke struct
    lastCuacaData.waktu = waktu;
    lastCuacaData.nama_kota = nama_kota;
    lastCuacaData.negara = negara;
    lastCuacaData.cuaca_group = cuaca_group;
    lastCuacaData.cuaca_deskripsi = cuaca_deskripsi;
    lastCuacaData.code = code;
    lastCuacaData.suhu = suhu;
    lastCuacaData.feels_like = feels_like;
    lastCuacaData.temp_min = temp_min;
    lastCuacaData.temp_max = temp_max;
    lastCuacaData.pressure = pressure;
    lastCuacaData.humidity = humidity;
    lastCuacaData.visibility = visibility;
    lastCuacaData.wind_speed = wind_speed;
    lastCuacaData.wind_deg = wind_deg;
    lastCuacaData.gust = gust;
    lastCuacaData.rain1h = rain1h;
    lastCuacaData.cloud = cloud;
    lastCuacaData.sea_level = sea_level;
    lastCuacaData.grnd_level = grnd_level;
    lastCuacaData.waktu_sunrise = waktu_sunrise;
    lastCuacaData.waktu_sunset = waktu_sunset;
    lastCuacaData.arah_angin = arah_angin;
    lastCuacaData.keterangan_lokasi = (sea_level - grnd_level > 100) ? "Dataran Tinggi" : "Dataran Rendah";
    lastCuacaData.valid = true;
    
    // ======================== REKOMENDASI AGRONOMIS OTOMATIS ========================
  lastCuacaData.idCuaca = -1;
  for (int i = 0; i < sizeof(daftarCuaca) / sizeof(daftarCuaca[0]); i++) {
  if (daftarCuaca[i].code == code) {
    lastCuacaData.idCuaca = daftarCuaca[i].id;
    break;
  }
}

if (mySerial.available())
    {
        static uint8_t recv_buf[10] = {0};
        uint16_t len = mySerial.readBytes((uint8_t *)&recv_buf, 4);

        if (len == 4 && recv_buf[0] == 0xff)
        {
            uint8_t sum = recv_buf[0] + recv_buf[1] + recv_buf[2];
            if (sum == recv_buf[3])
            {
                uint16_t distanceMM = (recv_buf[1] << 8) | recv_buf[2]; // hasil sensor dalam mm
                float distanceCM = distanceMM / 10.0; // ubah mm ke cm

                // Hitung ketinggian air
                float tinggiAir = ketinggianSensor - distanceCM;
                if (tinggiAir < 0) tinggiAir = 0; // supaya tidak negatif

                // Cetak hasil
                Serial.print("Jarak permukaan air: ");
                Serial.print(distanceCM, 1); // satu angka desimal
                Serial.println(" cm");

                Serial.print("Ketinggian air: ");
                Serial.print(tinggiAir, 1);
                Serial.println(" cm\n");
                lastCuacaData.ketinggian_air_deteksi = tinggiAir; // Simpan ketinggian air terdeteksi
                SerialMon.println("Ketinggian air terdeteksi: " + String(tinggiAir, 1) + " cm");
            }
        }
    }

  } else {
    SerialMon.println("[HTTP] Gagal mengambil data cuaca");
  }
}