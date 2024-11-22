#include <SPI.h>
#include <mcp2515.h>

// Inisialisasi MCP2515 dan variabel-variabel data
struct can_frame canMsg;
MCP2515 mcp2515(5);  // ESP32 menggunakan GPIO 5 sebagai CS (Chip Select)
int speedRead = 0;
int tempValue = 0;
int currentValue = 0;
int voltageIn = 0;
int voltaseMaksimal = 84;
int voltaseMinimal = 60;

void setup() {
  // Inisialisasi komunikasi Serial untuk debugging
  Serial.begin(115200);

  // Inisialisasi MCP2515
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    // Cek apakah ID CAN adalah 0x80000926
    if (canMsg.can_id == 0x80000926) {
      Serial.print("Data: ");
      for (int i = 0; i < 4; i++) {
        Serial.print(canMsg.data[i], HEX); // Tampilkan data byte dalam format hex
        Serial.print(" ");
      }
      Serial.println();

      // Mengonversi data dari hexadecimal menjadi desimal
      long decimalValueSpeed = convertHexToDecimalSpeed(canMsg.data);

      // Hitung kecepatan berdasarkan data yang diterima
      speedRead = 32 * (decimalValueSpeed / 10.0) * 0.001885;
      Serial.print("Kecepatan (Km/h) : ");
      Serial.println(abs(speedRead)); // Cetak kecepatan dalam Km/h
    } 
    else if (canMsg.can_id == 0x80001026) {
      // Proses data temperatur
      Serial.print("Data: ");
      for (int i = 0; i < 2; i++) {
        Serial.print(canMsg.data[i], HEX);
        Serial.print(" ");
      }
      long decimalValueTemp = convertHexToDecimalTemp(canMsg.data);
      tempValue = decimalValueTemp / 10;
      Serial.print("Temperature (Celcius) : ");
      Serial.println(abs(tempValue));

      // Proses data arus (Current)
      for (int i = 4; i < 6; i++) {
        Serial.print(canMsg.data[i], HEX);
        Serial.print(" ");
      }
      long decimalValueCurrent = convertHexToDecimalCurrent(canMsg.data);
      currentValue = decimalValueCurrent;
      Serial.print("Current (A) : ");
      Serial.println(abs(currentValue));
    } 
    else if (canMsg.can_id == 0x80001B26) {
      // Proses data voltase
      Serial.print("Data Voltage: ");
      for (int i = 4; i < 6; i++) {
        Serial.print(canMsg.data[i], HEX);
        Serial.print(" ");
      }
      long decimalValueVoltage = convertHexToDecimalVoltage(canMsg.data);
      voltageIn = decimalValueVoltage / 10;
      Serial.print("Voltage (V) : ");
      Serial.println(abs(voltageIn));

      // Perhitungan persentase SOC
      int persentase = (voltageIn - voltaseMinimal) * 100 / (voltaseMaksimal - voltaseMinimal);
      Serial.print("SOC (V) : ");
      Serial.println(abs(persentase));
    }
  }
}

// Fungsi untuk mengirim data ke HMI melalui Serial
void sendToHMI(String prefix, int value) {
  Serial.print(prefix);
  Serial.print(value);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

// Fungsi konversi Hex ke Desimal untuk kecepatan
long convertHexToDecimalSpeed(byte hexData[]) {
  long value = ((long)hexData[0] << 24) | ((long)hexData[1] << 16) |
               ((long)hexData[2] << 8) | (long)hexData[3];
  return value;
}

// Fungsi konversi Hex ke Desimal untuk temperatur
long convertHexToDecimalTemp(byte hexData[]) {
  long value = ((long)hexData[0] << 8) | (long)hexData[1];
  return value;
}

// Fungsi konversi Hex ke Desimal untuk voltase
long convertHexToDecimalVoltage(byte hexData[]) {
  long value = ((long)hexData[4] << 8) | (long)hexData[5];
  return value;
}

// Fungsi konversi Hex ke Desimal untuk arus (Current)
long convertHexToDecimalCurrent(byte hexData[]) {
  String hexString = "";
  for (int i = 4; i < 6; i++) {
    hexString += String(hexData[i], HEX);
  }
  return strtol(hexString.c_str(), NULL, 16);
}
