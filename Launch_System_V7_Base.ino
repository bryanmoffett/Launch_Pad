#include <Wire.h>
#include <SparkFun_Qwiic_Button.h>
#include <SparkFun_Alphanumeric_Display.h>  
#include <Servo.h>
//#include "BlinkM_funcs.h"
#include "SparkFun_Qwiic_Relay.h"
#include <SparkFun_Qwiic_Buzzer_Arduino_Library.h> // This may vary slightly based on the library
#include "Qwiic_LED_Stick.h"
#include <ArduinoBLE.h>

#define RELAY_ADDR 0x18 // Alternate address 0x19
Qwiic_Relay relay(RELAY_ADDR); 

HT16K33 display; // First display, for pad status and launch information
HT16K33 display2; // Second display, for bluetooth connection status
QwiicButton button;

uint32_t next;

//Define LED characteristics
uint8_t brightness = 250;   //The maximum brightness of the pulsing LED. Can be between 0 (min) and 255 (max)
uint16_t cycleTime = 1000;   //The total time for the pulse to take. Set to a bigger number for a slower pulse, or a smaller number for a faster pulse
uint16_t offTime = 200;     //The total time to stay off between pulses. Set to 0 to be pulsing continuously.

boolean statusLaunched = false;   // tracks if launch has occurred
boolean abortLaunch = false;     // tracks an abort event

// set all pins
const int buzzer = 10; // this pin is the constant large buzzer, no longer used
const int pinLaunch = 12;   // this pin lights fuse that fires the rocket motor through the relay
const int pinFire = 5;     // this pin has an LED that tells if the fuse is LIT or not
const int pinServo = 9;   // the pin attached to the launch clamp servo
const int ledPin = 13; // Pin for the onboard LED

// set up servo
int pos = 0;
Servo servo;

// set up Qwiic buzzer
QwiicBuzzer qbuzzer; // Create a buzzer object

// set up LED Stick
LED LEDStick; //Create an object of the LED class

// Create BLE Service and Characteristic UUIDs - you can generate your own
BLEService myService("c34457f2-f9e6-11ee-8020-325096b39f47"); 
BLECharacteristic myCharacteristic("abcd5678-efgh-1234-5678-abcd5678efgh", BLEWrite | BLERead, 16); // Maximum length 16 bytes

void setup() {
  // put your setup code here, to run once:

  //************INIT*************
  
  //Initialise the Pyro & buzzer pins)
  pinMode(pinLaunch, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(pinFire, OUTPUT);

  //Make sure that the output are turned off
  digitalWrite(pinLaunch, LOW);
  digitalWrite(buzzer, LOW);
  digitalWrite(pinFire, LOW);  

  Serial.begin(9600);
  Wire.begin(); //Join I2C bus 
  Wire.setClock(400000); // sound effects require changing configuration quickly
  Serial.println("Buzzer and Launch Pin Reset");  

  if(qbuzzer.begin() == false) {
  Serial.println("Qwiic Buzzer not detected. Please check connections.");
  while(1); // Loop forever if the buzzer is not detected
  }
  Serial.println("Qwiic Buzzer ready!");
  
  // Attach servo
  servo.attach(pinServo);
  Serial.println("Launch Servo Attached");  

  //check if display will acknowledge
  if (display.begin() == false) {
    Serial.println("Display 1 did not acknowledge! Freezing.");
    while(1);
  }
   //check if display will acknowledge
  if (display2.begin(0x71) == false) {
    Serial.println("Display 2 did not acknowledge! Freezing.");
    while(1);
  } 
  //check if button will acknowledge over I2C
  if (button.begin() == false) {
    Serial.println("Button did not acknowledge! Freezing.");
    while (1);
  }

  // ready the relay
  if(!relay.begin())
    Serial.println("Check connections to Qwiic Relay.");
  else
    Serial.println("Qwiic Relay is ready."); 
  float version = relay.singleRelayVersion();
  Serial.print("Firmware Version: ");
  Serial.println(version);    
  Serial.println("Button acknowledged.");

  //Start up communication with the LED Stick
  if (LEDStick.begin() == false){
    Serial.println("Qwiic LED Stick failed to begin. Please check wiring and try again!");
    while(1);
  }
  Serial.println("Qwiic LED Stick ready!");
  Serial.println("Both displays ready");
  Serial.println(""); 
  button.LEDoff();  //start with the LED off

  // Begin BLE initialization
  if (!BLE.begin()) {
    Serial.println("Failed to start BLE!");
    while (1); // Loop forever if initialization fails
   pinMode(ledPin, OUTPUT);  // Set the LED pin as output
  }

  // Set the device name
  BLE.setLocalName("Launch_Pad");

  // Add service and characteristic
  BLE.setAdvertisedService(myService);
  myService.addCharacteristic(myCharacteristic);
  BLE.addService(myService);

  // Start advertising
  BLE.advertise();

  // System is set up, configure Ready mode
  Serial.println("Launch_System_V7 Arduino Code on Redboard Artemis - BM - 04/5/24");  
  // Set the
  display.print("RDY");
  display2.print("OFFL");  
  LEDStick.setLEDBrightness(1);
  LEDStick.setLEDColor(0, 255, 0); // set LEDstick to Green
  Serial.println("Startup Complete, Launch Pad in Ready Mode.");
}

void loop() {

  fireCheck(12);

  // Check for connections
  BLEDevice central = BLE.central();  

  if (central) {
      Serial.print("Connected to central: ");
      Serial.println(central.address());

          // Flash the LED a few times
      for (int i = 0; i < 5; i++) {
        digitalWrite(ledPin, HIGH);
        delay(200); 
        digitalWrite(ledPin, LOW);
        delay(200);
        }
      // Keep LED on while connected
      digitalWrite(ledPin, HIGH);  
      connectBeep();
      display2.print("ONLN");  

      while (central.connected()) {
        // Inside the BLE loop, where you check for received data:
        if (myCharacteristic.written()) {
            String receivedData = ""; 
            for (int i = 0; i < myCharacteristic.valueLength(); i++) {
                receivedData += (char)myCharacteristic.value()[i];
            }
            Serial.println("Received data: " + receivedData);

            if (receivedData == "Launch") {
                goLaunch();
            }  else if (receivedData == "Abort") {
                handleAbort();
            }
        }
      }

      Serial.print("Disconnected from central: ");
      Serial.println(central.address());  
      display2.print("OFLN");  
      disconnectBeep();
  } else {
    // Turn the LED off if not connected
    digitalWrite(ledPin, LOW); 
    display2.print("OFLN");        
  }
  
  if (button.isPressed() == true && !statusLaunched) {
    button.LEDconfig(brightness, cycleTime, offTime);
    goLaunch();
  }   
    while (button.isPressed() == true)
      delay(10);  //wait for user to stop pressing
    button.LEDoff();
  delay(20); //let's not hammer too hard on the I2C bus  
}

void goLaunch()
{
    Serial.println("LAUNCH INITIATED!");
    display.print("GOGO");
    LEDStick.setLEDBrightness(5);  
    LEDStick.setLEDColor(255, 255, 255); //set lights to white until countdown begins
    delay(5000);
    longBeep();
    display.print("STRT");    
    delay(1000);
    
    LEDStick.setLEDColor(255, 128, 0); //set lights to orange for countdown

    Serial.println("Countdown commencing.");
    
    countdownSequence();  // abort is handled within countdownSequence

    // Proceed with actions only if no abort was triggered
    if (!abortLaunch) {  // This check ensures we only proceed if abort hasn't been triggered
        servo.write(180); // Assuming this is safe to do regardless of abort state
        
        // All actions below should only occur if we're clear to launch
        delay(1000);  // Wait for any mechanical actions to complete
        LEDStick.setLEDColor(255, 0, 0); //set lights to RED for firing
        display.print("FIRE");  
        digitalWrite(pinLaunch, HIGH); // Fire the rocket motor
        relay.turnRelayOn();  // Activate relay for launch
        Serial.println("The launch pyro has been fired");
        fireBeep();
        delay(1000);
        statusLaunched = true; 
        digitalWrite(pinLaunch, LOW); // Reset launch pin
        relay.turnRelayOff();  // Ensure relay is off after launch
        LEDStick.setLEDBrightness(1);
        LEDStick.setLEDColor(0, 255, 0); //set lights to green since firing is done        
        Serial.println("The launch pyro has been reset");
        display.print("DONE");
        doneBeep(); 
    } else {
        Serial.println("Launch sequence aborted, skipping relay activation.");
        abortLaunch = false;
        Serial.println("abortLaunch variable reset to false.");
    }
}

void shortBeep()
{
  //tone(buzzer, 659, 100);
  Serial.println("Short Beep");
  //delay(300);
  //noTone(buzzer);
  qbuzzer.configureBuzzer(2730, 300, SFE_QWIIC_BUZZER_VOLUME_MAX);
  qbuzzer.on();
  delay(280);  
  //qbuzzer.off();    
}

void connectBeep()
{
  Serial.println("Bluetooth Connection Beep");
  qbuzzer.configureBuzzer(1300, 100, SFE_QWIIC_BUZZER_VOLUME_MAX);
  qbuzzer.on();
  delay(100); 
  qbuzzer.configureBuzzer(1300, 100, SFE_QWIIC_BUZZER_VOLUME_MAX);
  qbuzzer.on();
  delay(100);  
  qbuzzer.configureBuzzer(1300, 100, SFE_QWIIC_BUZZER_VOLUME_MAX);
  qbuzzer.on();
  delay(100);       
}

void disconnectBeep()
{
  Serial.println("Bluetooth Disonnection Beep");
  qbuzzer.configureBuzzer(2730, 300, SFE_QWIIC_BUZZER_VOLUME_MAX);
  qbuzzer.on();
  delay(100); 
  qbuzzer.configureBuzzer(1300, 100, SFE_QWIIC_BUZZER_VOLUME_MAX);
  qbuzzer.on();    
}

void longBeep()
{
  Serial.println("Long Beep");
  
  //Serial.println("Volume: MAX (4)");
  //qbuzzer.configureBuzzer(2730, 2000, SFE_QWIIC_BUZZER_VOLUME_MAX); // frequency: 2.73KHz, duration: 100ms, volume: MAX
  //qbuzzer.on();

  qbuzzer.playSoundEffect(1, 4);  
  qbuzzer.playSoundEffect(1, 4);  
  qbuzzer.playSoundEffect(1, 4);  
  qbuzzer.playSoundEffect(1, 4);    
  //tone(buzzer, 523, 2000);
  //delay(2000);  
  //noTone(buzzer);
  //qbuzzer.on();
  delay(2000);  
  //qbuzzer.off();    
}

void fireBeep() {
  //unsigned long startTime = millis();
  //bool toggle = false; // Toggle between the two tones
  Serial.println("Fire Beep");
 // while (millis() - startTime < 2000) { // Continue for 2000 milliseconds
 //   if (toggle) {
 //     tone(buzzer, 880); // First tone frequency
 //   } else {
 //     tone(buzzer, 790); // Second tone frequency, slightly lower for a noticeable trill
 //   }
 //   delay(50); // Short delay between toggles to create the trill effect
 //   toggle = !toggle; // Switch tones
 // }
 // noTone(buzzer); // Stop the tone

  qbuzzer.playSoundEffect(3, 4);  
  qbuzzer.playSoundEffect(3, 4);  
  qbuzzer.playSoundEffect(3, 4);  
  qbuzzer.playSoundEffect(3, 4);      
  qbuzzer.playSoundEffect(3, 4);    
}

void doneBeep()
{
  //tone(buzzer, 880, 3000);
  //delay(3000);  
  //noTone(buzzer);
  Serial.println("Done Beep");

  qbuzzer.playSoundEffect(4, 4);  
}

void fireCheck(int pin)   // this function checks if the motor fuse is active or not, for safety
{
  int val = 0;     // variable to store the read value
  // read the input pin to check if the fuse is lit
    val = digitalRead(pin);
    if (val == 0)
    {
      //fuse is not lit
      //longBeep();
      digitalWrite(pinFire, LOW);
      //Serial.println("Fuse is not lit");
      //Serial.println(val);
    }
    else
    {
      //fuse is lit
      digitalWrite(pinFire, HIGH);
      display.print("FIRE");  
      Serial.println("CAUTION: Fuse IS LIT");
      Serial.println(val);  
    }
}

void countdownSequence() { // this function manages the countdown
    for (int i = 10; i > 0; i--) {
        if (abortLaunch) {
        Serial.println("Launch abort detected within countdown sequence.");
        display.print("ABRT");
        // Additional abort handling logic here
        return; // Exit the countdown and handle abort
        }
        LEDStick.setLEDColor((i-1), 0, 0, 0);
        display.print(String(i));
        Serial.println(String(i));
        if (i > 3) {
        shortBeep();  
        delay(700);           
        }
        else {
        shortBeep();
        shortBeep();        
        delay(400);  
        }
        // Check for abort signal (second button press)
        if (checkForAbort()) {
            Serial.println("About to execute handleAbort.");
            handleAbort();
            return; // Exit the countdown immediately
        }

    }

}

bool checkForAbort() {
    if (button.isPressed()) {
        delay(50); // Short delay for debouncing, adjust this value based on your needs
        // Re-check the button state after the delay
        if (button.isPressed()) {
          Serial.println("Button pressed while checking for abort.");    
          // The button is still pressed, confirming the abort action
          abortLaunch = true;
        Serial.println("Abort Launch variable set to true.");    
            return true;
        }
    }
    return false; // No valid abort detected
}

void handleAbort() {
    Serial.println("Entering handleAbort.");  
    display.print("ABRT");
    Serial.println("Display set to ABRT, handling an abort.");    
    // Additional logic to safely stop the launch
    // For example, reset servos, turn off relays, etc.
    Serial.println("Launch aborted!");
    // Ensure the system is in a safe state before finishing
    shortBeep();
    longBeep();
    shortBeep();
    longBeep();
    shortBeep();    
    doneBeep();    
    //abortLaunch = false;
    Serial.println("Abort Launch variable left at true.");    
    Serial.println("Exiting handleAbort.");

}
