// Definisikan pin serial ESP32 untuk komunikasi dengan Nextion
#define RX_PIN 16
#define TX_PIN 17

// Inisialisasi komunikasi serial
HardwareSerial nextionSerial(1);

int rpm = 0;
float ampere = 0.0;
float voltage = 0.0;
int soc = 0;
int temperature = 0;

void setup() {
  // Mulai serial monitor untuk debugging
  Serial.begin(115200);
  
  // Mulai komunikasi serial dengan Nextion
  nextionSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Tunggu sedikit untuk stabilisasi
  delay(100);
  Serial.println("Nextion display initialized");
}

void loop() {
  // Simulasi pembacaan data sensor
  rpm = random(1000, 5000);        // Simulasi data RPM
  ampere = random(10, 50) / 10.0;  // Simulasi data ampere
  voltage = random(110, 130) / 10.0; // Simulasi data voltage
  soc = random(50, 100);           // Simulasi data SOC
  temperature = random(20, 40);    // Simulasi data temperature
  
  // Kirim data ke Nextion
  sendToNextion(rpm, ampere, voltage, soc, temperature);
  
  // Delay untuk pembacaan berikutnya
  delay(1000);
}

void sendToNextion(int rpm, float ampere, float voltage, int soc, int temperature) {
  // Format data untuk dikirim ke Nextion
  String cmdRpm = "speed.val=" + String(rpm);
  String cmdAmpere = "ampere.val=" + String(ampere);
  String cmdVoltage = "voltage.val=" + String(voltage);
  String cmdSoc = "value.val=" + String(soc);
  String cmdTemperature = "temperature.val=" + String(temperature);
  
  // Kirim data ke Nextion satu per satu
  sendCommandToNextion(cmdRpm);
  sendCommandToNextion(cmdAmpere);
  sendCommandToNextion(cmdVoltage);
  sendCommandToNextion(cmdSoc);
  sendCommandToNextion(cmdTemperature);
}

void sendCommandToNextion(String cmd) {
  // Kirim perintah ke Nextion dan diakhiri dengan tiga karakter 0xFF
  nextionSerial.print(cmd);
  nextionSerial.write(0xFF);
  nextionSerial.write(0xFF);
  nextionSerial.write(0xFF);
}
