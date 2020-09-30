int incomingByte = 0;    // for incoming serial data
int bytesRead = 0;
struct InstrumentData {
  // -100 to 100, -100 means nose down.
  char trimPos;
  // Total number of flap positions
  char flapCnt;
  // Flap position index. [0, flapCnt]
  char flapPos;
  // 0-unkonwn, 1-up, 2-down.
  char landingGearPos;
  bool parkingBrakeOn;
};
InstrumentData instrumentData;
char outputStr[100];

void setup() {
    Serial.begin(9600);    // opens serial port, sets data rate to 9600 bps
}

void loop() {
  // send data only when you receive data:
  if (Serial.available() > 0) {
  
    // read the incoming byte:
    bytesRead = Serial.readBytes((char*)&instrumentData, sizeof(InstrumentData));
  
    // say what you got:
    if (bytesRead == sizeof(InstrumentData)){
      sprintf(outputStr, "Received: %d, %d, %d, %d, %s", instrumentData.trimPos, instrumentData.flapCnt, instrumentData.flapPos, instrumentData.landingGearPos, instrumentData.parkingBrakeOn?"true":"false");
      Serial.println(outputStr);
    } else {
      Serial.println("Wrong bytes received!!!!!!!!");
    }
  }
  
}
