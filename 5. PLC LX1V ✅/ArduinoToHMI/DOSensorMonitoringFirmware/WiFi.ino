#if !IS_ARDUINO_BOARD && ENABLE_FIREBASE
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;  // Offset for WIB (UTC+7)
const int daylightOffset_sec = 0;

const char* loggingTimes[] = {
  "06:00",
  "12.00",
  "18.00"
};

const int numLoggingTimes = sizeof(loggingTimes) / sizeof(loggingTimes[0]);

String lastLoggedTime = "";
String currentDate = "";

void wifiTaskInit() {
  auth.apiKey = "AIzaSyAGROtnNcRC-c0a2j05yHlQZQRDb2WUjiE";
  auth.databaseURL = "https://testing-project-8edf2-default-rtdb.firebaseio.com/";
  auth.projectID = "testing-project-8edf2";
  auth.user.email = "admin@gmail.com";
  auth.user.password = "admin123";

  // firebase.connectToWiFi("silenceAndSleep", "00000000");
  firebase.connectToWiFi("TP-Link_E14A", "Cctv4321");
  firebase.init(&auth);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void wifiTaskLoop() {
  static uint32_t timeCheckTimer;
  static uint32_t firebaseRealtimeTimer;
  static uint32_t firebaseHistoryTimer;

  uint32_t epoch = getTime();
  epoch += gmtOffset_sec;
  DateTimeNTP currentTime(epoch);
  String timestampPrint = currentTime.timestamp();
  timestampPrint.replace("T", " ");

  int currentHour = currentTime.hour();
  int currentMinute = currentTime.minute();
  String today = timestampPrint.substring(0, 10);
  String currentTimeStr = String(currentHour) + ":" + String(currentMinute);

  // Cek jika hari sudah berganti
  if (currentDate != today) {
    currentDate = today;
    lastLoggedTime = "";  // Reset log harian
  }

  if (millis() - firebaseRealtimeTimer >= 5 * 1000 && firebase.isConnect()) {
    firebaseRealtimeTimer = millis();
    firebase.initData(5);
    firebase.addData(writeHoldingRegister[ADC_REGISTER], "data/do_adc");
    firebase.addData(writeHoldingRegister[VOLT_REGISTER], "data/do_voltage");
    firebase.addData(writeHoldingRegister[DO_REGISTER], "data/do_sensor_value");
    firebase.addData(writeHoldingRegister[OUT_FREQUENCY_REGISTER], "data/output_frequency");
    firebase.addData(writeHoldingRegister[PWM_OUT_REGISTER], "data/pwm_output");
    firebase.sendDataFloat();
  }

  if (millis() - firebaseHistoryTimer >= 15 * 1000 && firebase.isConnect()) {
    firebaseHistoryTimer = millis();
    firebase.initData(5);
    firebase.addData(writeHoldingRegister[ADC_REGISTER], String("hourly_history/" + String(timestampPrint) + "/do_adc").c_str());
    firebase.addData(writeHoldingRegister[VOLT_REGISTER], String("hourly_history/" + String(timestampPrint) + "/do_voltage").c_str());
    firebase.addData(writeHoldingRegister[DO_REGISTER], String("hourly_history/" + String(timestampPrint) + "/do_sensor_value").c_str());
    firebase.addData(writeHoldingRegister[OUT_FREQUENCY_REGISTER], String("hourly_history/" + String(timestampPrint) + "/output_frequency").c_str());
    firebase.addData(writeHoldingRegister[PWM_OUT_REGISTER], String("hourly_history/" + String(timestampPrint) + "/pwm_output").c_str());
    firebase.sendDataFloat();
  }

  if (millis() - timeCheckTimer >= 1000) {
    timeCheckTimer = millis();

    // Format waktu saat ini sebagai string "HH:MM"
    char currentTimeFormatted[6];
    snprintf(currentTimeFormatted, sizeof(currentTimeFormatted), "%02d:%02d", currentHour, currentMinute);

    // Debug: Cetak waktu saat ini
    Serial.print("| timestampPrint: ");
    Serial.print(timestampPrint);
    Serial.print("| currentTime: ");
    Serial.print(currentTimeFormatted);

    // Cek apakah waktu saat ini ada dalam daftar loggingTimes
    bool shouldLog = false;
    for (int i = 0; i < numLoggingTimes; i++) {
      if (strcmp(currentTimeFormatted, loggingTimes[i]) == 0 && currentTimeStr != lastLoggedTime) {
        shouldLog = true;
        break;
      }
    }

    Serial.print("| shouldLog: ");
    Serial.print(shouldLog);

    // Jika perlu log dan terhubung ke Firebase
    if (shouldLog && firebase.isConnect()) {
      lastLoggedTime = currentTimeStr;

      // Format timePart: HH:MM:00
      String timePart = currentTimeStr + ":00";  // Tambahkan ":00" untuk detik
      // Format path: history/YYYY-MM-DD/HH:MM:00
      String path = "history/" + currentDate + "/" + timePart;

      // Kirim data
      firebase.initData(5);
      firebase.addData(writeHoldingRegister[ADC_REGISTER], (path + "/do_adc").c_str());
      firebase.addData(writeHoldingRegister[VOLT_REGISTER], (path + "/do_voltage").c_str());
      firebase.addData(writeHoldingRegister[DO_REGISTER], (path + "/do_sensor_value").c_str());
      firebase.addData(writeHoldingRegister[OUT_FREQUENCY_REGISTER], (path + "/output_frequency").c_str());
      firebase.addData(writeHoldingRegister[PWM_OUT_REGISTER], (path + "/pwm_output").c_str());
      firebase.sendDataFloat();

      Serial.println("Logged at: " + timestampPrint);
    }

    Serial.println();
  }
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return (0);
  }
  time(&now);
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return now;
}
#endif