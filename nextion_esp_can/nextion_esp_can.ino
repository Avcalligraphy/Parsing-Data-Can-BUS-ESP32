#include <can.h>
#include <mcp2515.h>
#include <CanHacker.h>
#include <CanHackerLineReader.h>
#include <lib.h>
#include <SPI.h>

const int SPI_CS_PIN = 5;  // Pin Chip Select
const int INT_PIN = 21;    // Pin Interrupt

CanHackerLineReader *lineReader = NULL;
CanHacker *canHacker = NULL;

void handleError(const CanHacker::ERROR error);

void setup() {
    Serial.begin(115200);  // Untuk debugging
    SPI.begin();           // Inisialisasi SPI
    
    // Stream untuk interface dan debugging
    Stream *interfaceStream = &Serial;
    Stream *debugStream = &Serial;  // Gunakan Serial yang sama untuk debug
    
    canHacker = new CanHacker(interfaceStream, debugStream, SPI_CS_PIN);
    canHacker->setClock(MCP_8MHZ);  
//    canHacker->enableLoopback();  // Hapus ini untuk menonaktifkan mode loopback
    lineReader = new CanHackerLineReader(canHacker);
    
    pinMode(INT_PIN, INPUT);  // Konfigurasi pin interrupt
}

void loop() {
    CanHacker::ERROR error;

    if (digitalRead(INT_PIN) == LOW) {
        error = canHacker->processInterrupt();
        handleError(error);
    }
}

void serialEvent() {
    CanHacker::ERROR error = lineReader->process();
    handleError(error);
}

void handleError(const CanHacker::ERROR error) {
    if (error != CanHacker::ERROR_OK) {
        Serial.print("Error (code ");
        Serial.print((int)error);
        Serial.println(")");
        while (1) {
            delay(2000);
        }
    }
}
