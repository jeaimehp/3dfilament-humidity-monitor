/*
 *  Name: 3D Filament Box Humidity Monitor 
 *  By: Je'aime Powell
 *  Date: 5/29/21
 *  Purpose: Sensor to live in the 3D Printer filament box to monitor
 *  filament humidity.
 *  
 *  Built for/Test Equipmemt
 *    - Arduino Nano 33 BLE Sense
 *      - HTS221 Temperature and Humidity Sensor (built-in)
 *      - RGB LED (built-in)
 *        << NOTE!! >> LOW = ON for the LEDs
 *      - Switch Pin D2 -> GND (Switch Closed for Open Box Message)
 *      Note: other built in sensors: Ref: https://store.arduino.cc/usa/nano-33-ble-sense-with-headers
 *        - LSM9DS1 - IMU
 *        - MP34DT05 - Mic
 *        - APDS9960 - Gesture/Light/Proximity
 *        - LPS22HB - Barometric Pressure
 *    - SSD1306 OLED (I2C) 128x32 
 *   
 *   Filament humidity alerts 
 *     Ideal Humidity: 10% - 20% (Excellent), 20%-25% (Good),25% -35% (Fair), >35% (Poor)  
 *      ! PLA Humidity > 45%
 *      ! PETG Humidty > 45% 
 */
/////////////
//LIBRARIES
/////////////

// SSD1306 OLED 128x32
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// HTS221 Temperature and Humidity
#include <Arduino_HTS221.h>

// BLE for bluetooth beacon
#include <ArduinoBLE.h>

// RGB LEDS
#include "mbed.h"

// SSD1306 OLED Global Configuration

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16




// Lid open alert definitions
  // Builtin RGB LEDs pins defined by mbed.h library
  // Ref: https://gilberttanner.com/blog/arduino-nano-33-ble-sense-overview
  // Ref: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

const byte lidswitchPin = 2;
bool humidityAlert = false;


/////////////////////
// BLE Beacon Configuration 
// Ref:  https://rootsaid.com//arduino-ble-example/
//
// Purpose: Allow bluetooth connection to provide Humidity remotely
//
////////////////////

BLEService environmentalSensingService("181A");
BLEUnsignedIntCharacteristic humidityCharacteristic("2A6F", BLERead);
// Ref for Characteristics: https://create.arduino.cc/projecthub/dhorton668/bluetooth-weather-station-40d1e4?ref=part&ref_id=107215&offset=17

float h;


/////////////////////
// FUNCTIONS
/////////////////////


// Easy method to set on board RGB LEDs
// Note: 'K' = Off
void setledstatus(char color){
  switch (color) {
    case 'R':
      digitalWrite(LEDR, LOW);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH);
      break;
    case 'G':
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, LOW);
      digitalWrite(LEDB, HIGH);
      break;
    case 'B':
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, LOW);
      break;
    case 'Y':
      digitalWrite(LEDR, LOW);
      digitalWrite(LEDG, LOW);
      digitalWrite(LEDB, HIGH);
      break;
    case 'K':
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH);
      break;
    default:
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH);
      break;
  }
}

// Provides a status and changes LED based on suggested humidity ranges
// Outputs string with status and also sets humidiyAlert if above 50%
String humidityStatus (float hreading){
 /*   Filament humidity alerts 
 *     Ideal Humidity: 10% - 20% (Excellent), 20%-25% (Good),25% -35% (Fair), >35% (Poor)  
 *      ! PLA Humidity > 45%    
 *      ! PETG Humidty > 45% 
 *      
 *      ASCII Codes for OLED display.write() Ref:https://www.ascii-codes.com/
 *       - Squareroot (check) - 251
 *       - Smile - 1
 *       - Heart - 3
 *       - Double Exclimation - 19
 */
  String state;
  if (hreading <= 20) {
   state = "GREAT";
   setledstatus('K');
   humidityAlert = false;
   Serial.print("Humidity "+ state +" = " );
   Serial.print(hreading);
   Serial.println("%");
   
  } else if (hreading > 20 && hreading <= 25){
   state = "GOOD";
   setledstatus('K');
   humidityAlert = false;
   Serial.print("Humidity "+ state +" = " );
   Serial.print(hreading);
   Serial.println("%"); 
  } else if (hreading > 25 && hreading <= 35){
   state = "FAIR";
   humidityAlert = false;
   setledstatus('Y');
   Serial.print("Humidity "+ state +" = " );
   Serial.print(hreading);
   Serial.println("%"); 
  }
  else {
   state = "POOR";
   
   if (hreading > 50){
     humidityAlert = true;
     setledstatus('R');

   }
   else {
     humidityAlert = false;
     setledstatus('Y');
   }
   Serial.print("Humidity "+ state +" = " );
   Serial.print(hreading);
   Serial.println("%"); 
  }
  
  return state;  
}


// Checks Pin D2 for GND connect through switch to determine if lid
// is open on the container. If it is is sets the OLED Display to 
// "OPEN BOX" and sets the RGB LED to Red. 
// NOTE: If lid open it remains in this loop until the lid is closed.  
void lidstatus(){
  int switchValue = digitalRead(lidswitchPin);
  delay(500);
  while (switchValue == 0){
    display.clearDisplay();
    display.display();
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
    display.println("----------");
    display.println(" BOX OPEN");
    display.display();
    setledstatus('R');
    switchValue = digitalRead(lidswitchPin);
    Serial.print(" Lid Opened Detected = ");
    Serial.println(switchValue);
    delay(2000); 
  }
  Serial.print(" Lid Closed  = ");
  Serial.println(switchValue);
  setledstatus('K');
}


////////////////
// SETUP
////////////////  
 
 void setup() {
  // Initializing Serial
  Serial.begin(9600);
   delay(3000);

  //Initalize lid switch
  pinMode(lidswitchPin, INPUT_PULLUP);
  
  // Initalize RGB LED pins
  
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(LED_PWR, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  setledstatus('K');
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_PWR, LOW);

  //LED Test
  // !!NOTE!! LOW = ON for the LEDs
  //digitalWrite(LEDR, LOW);
  //delay(1000); 
  //digitalWrite(LEDG, LOW);
  //delay(1000); 
  //digitalWrite(LEDB, LOW);
  //delay(1000); 
  //digitalWrite(LED_PWR, LOW);
  //delay(1000);
  
  Serial.println("In Setup");
  Serial.println("Initializing OLED");
  // Initalizing OLED
   if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.display();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.println(" Filament");
  display.println("Box Sensor");
  display.display();
  delay(2000);
  
  // HTS Sensor Initialization
  //while (!Serial);
  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }
  display.clearDisplay();
  display.display();
  delay(1000);
  display.setTextSize(1);
  display.println("Filament Box Monitor");
  display.println("");
  display.println("HTS initialized");
  display.println("Monitoring...");
  display.display();
  delay(2000);

  //BLE Initialization
  Serial.println("Initializing BLE Beacon ...");
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setLocalName("FilamentBoxMonitor");
  BLE.setAdvertisedService(environmentalSensingService);
  environmentalSensingService.addCharacteristic(humidityCharacteristic);
  BLE.addService(environmentalSensingService);
  //humidityLevelChar.writeValue(0); //inital value

  BLE.setConnectable(true);
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");
  
}


////////////////////
// LOOP
///////////////////

void loop() {
  lidstatus();
  display.clearDisplay();
  float temperature = HTS.readTemperature(FAHRENHEIT);
  float humidity    = HTS.readHumidity();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Humidity");
  
  
  display.setTextSize(2);
  display.print(humidity);
  display.print("% ");
  display.setTextSize(1);
  display.println(humidityStatus(humidity));
  if (humidityAlert){
    display.print("              ALERT");
    display.write(3);
    display.println("");
    
  }
  else {
    display.println("");
  }
  display.display();
  display.setTextSize(1);
  display.print("Temperature = ");
  display.print(temperature);
  display.println("F");
  display.display();

  // BLE Service Beacon
  BLEDevice central = BLE.central();
  bool centralConnected = false;

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    setledstatus('B');
    centralConnected = true;

    while (central.connected()) {
      h = HTS.readHumidity();
      humidityCharacteristic.writeValue((uint16_t) round(h * 100));
      Serial.print("Sent BLE ");
      Serial.println((uint16_t) round(h * 100));
      delay(2000);

  }
}
  if (centralConnected){
    setledstatus('K');
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
    centralConnected = false;
    }
  delay(5000);
    
}
