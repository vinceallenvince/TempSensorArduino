#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include "floatToString.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "NYCR24"           // cannot be longer than 32 characters!
#define WLAN_PASS       "clubmate"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "http://162.243.120.32"
#define WEBSITE_IP   162,243,120,32
#define WEBPAGE      ""

#define PROJECT_ID   ""  //replace with your project ID
#define PROJECT_WRITE_KEY ""  // replace with your write key

/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/

uint32_t ip;

int sensorPin = 0;

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  // Optional SSID scan
  // listSSIDResults();
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  /* Display the IP address DNS, Gateway, etc. */  
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  
  ip = cc3000.IP2U32(WEBSITE_IP); // ENTER IP
  
  Serial.print(F("connecting to --> ")); cc3000.printIPdotsRev(ip);
  Serial.println(F(""));
}
  
void loop(void)
{
  
  // read temperature sensor
  int temp_reading = analogRead(sensorPin);
  float voltage = temp_reading * 3.3;
  
  voltage /= 1024.0;
  Serial.print(voltage); Serial.println(" volts");
  
  // now print out the temperature
  //float temp_c = (voltage - 0.5) * 100; //converting from 10 mv per degree wit 500 mV offset
                                        //to degrees ((voltage - 500mV) times 100)
  
  // float temperatureC = ((reading * 0.004882) - 0.50) * 100; // some formula that seems to work                                      
  float temp_c = (voltage - 0.33) * 100; // using 3.33v line since 5v is being used by wifi shield
  Serial.print(temp_c); Serial.println(" degrees C");
  
  // now convert to Fahrenheit
  float temp_f = (temp_c * 9.0 / 5.0) + 32.0;
  Serial.print(temp_f); Serial.println(" degrees F");
  
  // connect to the IP address
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 8000);
  
  switch(sendDataToNode(www, temp_f)) {
    case 1:
      Serial.println(F("No response."));
      break;
    case 2:
    
    Serial.println(F("-------------------------------------"));
    
    /* Read data until either the connection is closed, or the idle timeout is reached. */ 
    unsigned long lastRead = millis();
    while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
      while (www.available()) {
        char c = www.read();
        Serial.print(c);
        lastRead = millis();
      }
    }      
    
    www.close();
    Serial.println(F("\n\nDisconnecting..."));
    //cc3000.disconnect();
    //Serial.println(F("Connection closed."));
  }

  delay(60000); // take a reading every minute
}

int sendDataToNode(Adafruit_CC3000_Client www, float temperature) 
{
  if (www.connected()) {
    Serial.println(F("Yay! Connected!!!"));
    
    char buffer[5];
    String str = floatToString(buffer, temperature, 5);
    str.toCharArray(buffer, 5);
    
    www.fastrprint(F("GET "));
    www.fastrprint(F("/?projectId="));
    www.fastrprint(F(PROJECT_ID));
    www.fastrprint(F("&writeKey="));
    www.fastrprint(F(PROJECT_WRITE_KEY));
    www.fastrprint(F("&temperature="));
    www.fastrprint(buffer); // temperature
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
    
    return 2;
  } else {
    return 1;
  }
        
}

/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint8_t valid, rssi, sec, index;
  char ssidname[33]; 

  index = cc3000.startSSIDscan();

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

