#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 mcp2515(10);
float speedRead = 0;
int tempValue = 0;
int currentValue = 0;
int voltageIn = 0;
int voltaseMaksimal = 84;
int voltaseMinimal = 63;


void setup() {
  Serial.begin(115200);
  
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
  } else if (canMsg.can_id == 0x80001026){
    Serial.print("Data: ");
      for (int i = 0; i < 2; i++) {
        Serial.print(canMsg.data[i], HEX);
        Serial.print(" ");
      }
      long decimalValueTemp = convertHexToDecimalTemp(canMsg.data);
      tempValue = decimalValueTemp / 10;
      Serial.print("Temperature (Celcius) : ");
      Serial.println(abs(tempValue));
       // Kode Current
      for (int i = 4; i < 6; i++) {
        Serial.print(canMsg.data[i], HEX);
        Serial.print(" ");
      }
      long decimalValueCurrent = convertHexToDecimalCurrent(canMsg.data);
      currentValue = decimalValueCurrent;
      Serial.print("Current (A) : ");
      Serial.println(abs(currentValue));
    
    } else if(canMsg.can_id == 0x80001B26){
      Serial.print("Data Voltage: ");
      for (int i = 4; i < 6; i++) {
        Serial.print(canMsg.data[i], HEX);
        Serial.print(" ");
      }
      long decimalValueVoltage = convertHexToDecimalVoltage(canMsg.data);
      voltageIn = decimalValueVoltage / 10;
      Serial.print("Voltage (V) : ");
      Serial.println(abs(voltageIn));
      } 
}

long convertHexToDecimalSpeed(byte hexData[]) {
  // Menggabungkan 4 byte data menjadi satu nilai long menggunakan bit-shifting
  long value = ((long)hexData[0] << 24) | ((long)hexData[1] << 16) |
               ((long)hexData[2] << 8) | (long)hexData[3];
  return value;
}
long convertHexToDecimalTemp(byte hexData[]) {
  String hexString = "";
  for (int i = 0; i < 2; i++) {
    hexString += String(hexData[i], HEX);
  }
  return strtol(hexString.c_str(), NULL, 16);
}
long convertHexToDecimalVoltage(byte hexData[]) {
  // Menggabungkan 2 byte data menjadi satu nilai long menggunakan bit-shifting
  long value = ((long)hexData[4] << 8) | (long)hexData[5];
  return value;
}

long convertHexToDecimalCurrent(byte hexData[]) {
  // Menggabungkan 2 byte data menjadi satu nilai long menggunakan bit-shifting
  long value = ((long)hexData[4] << 8) | (long)hexData[5];
  return value;
}
