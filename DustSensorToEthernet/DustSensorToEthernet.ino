#include <GP2YDustSensor.h>
#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>
#include <SDConfig.h>

int measurePin = A0;
int ledPin = 2;

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, ledPin, measurePin);

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN
const int SD_SELECT_PIN = 4;
const int ETHERNET_SELECT_PIN = 10;

IPAddress ip, gateway, subnet, serverIP;
int serverPort;
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

EthernetClient client;

//SoftwareSerial gpsSerial(2, 3); //rx, tx 

boolean readConfiguration() {
  /*
   * Length of the longest line expected in the config file.
   * The larger this number, the more memory is used
   * to read the file.
   * You probably won't need to change this number.
   */
  int maxLineLength = 127;
  SDConfig cfg;
  char fileName[] = "setup.cfg";
  const char* CONFIG_HMAC = "HMAC";  
  const char* CONFIG_IP = "IP";
  const char* CONFIG_GATEWAY = "GATEWAY";
  const char* CONFIG_SUBNET = "SUBNET";
  const char* CONFIG_SERVER_IP = "SERVER_IP";
  const char* CONFIG_SERVER_PORT = "SERVER_PORT";

  // Setup the SD card 
  Serial.println("Calling SD.begin()...");
  
  if (!SD.begin(SD_SELECT_PIN)) {
    Serial.println("SD.begin() failed. Check: ");
    Serial.println("  card insertion,");
    Serial.println("  SD shield I/O pins and chip select,");
    Serial.println("  card formatting.");
    return;
  }
  
  Serial.println("...succeeded.");
  // Read our configuration from the SD card file.
  
  // Open the configuration file.
  if (!cfg.begin(fileName, maxLineLength)) {
    Serial.print("Failed to open configuration file: ");
    Serial.println(fileName);
    return false;
  }
  
  // Read each setting from the file.
  while (cfg.readNextSetting()) {
    if (cfg.nameIs(CONFIG_HMAC)) {
      String hmacStr = cfg.getValue();
      Serial.print("HMAC: ");
      Serial.println(hmacStr);      
    } else if (cfg.nameIs(CONFIG_IP)) {
      ip = cfg.getIPAddress();
      Serial.print("IP: ");
      Serial.println(ip);
    } else if (cfg.nameIs(CONFIG_GATEWAY)) {
      gateway = cfg.getIPAddress();
      Serial.print("GATEWAY: ");
      Serial.println(gateway);
    } else if (cfg.nameIs(CONFIG_SUBNET)) {
      subnet = cfg.getIPAddress();
      Serial.print("SUBNET: ");
      Serial.println(subnet);
    } else if (cfg.nameIs(CONFIG_SERVER_IP)) {
      serverIP = cfg.getIPAddress();
      Serial.print("SERVER IP: ");
      Serial.println(serverIP);
    } else if (cfg.nameIs(CONFIG_SERVER_PORT)) {
      serverPort = cfg.getIntValue();
      Serial.print("SERVER PORT: ");
      Serial.println(serverPort);
    } else {
      Serial.print("Unknown name in config: ");
      Serial.println(cfg.getName());
    }
  }
  cfg.end();
  return true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dustSensor.begin();

  if (readConfiguration() == false) {
    return;
  }

  //Init Ethernet
  Ethernet.init(ETHERNET_SELECT_PIN);
  Ethernet.begin(mac, ip, gateway, subnet);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
}

void printToSerial() {
  Serial.print("Dust density: ");
  Serial.print(dustSensor.getDustDensity());
  Serial.print(" ug/m3; Running average: ");
  Serial.print(dustSensor.getRunningAverage());
  Serial.println(" ug/m3");  
}

void loop() {
  while (!client.connected()) {
    if (client.connect(serverIP, serverPort)) {
      Serial.println("connected");
      break;
    } else {
      Serial.println("connection failed");
      printToSerial();
      delay(1000);
    }
  }

  if (client.connected()) {
    client.write("Dust density: " + dustSensor.getDustDensity() + " ug/m3; Running average: " + dustSensor.getRunningAverage() + " ug/m3");    
  }
  else {
    client.stop();
    printToSerial();
  }
  delay(1000);
}
