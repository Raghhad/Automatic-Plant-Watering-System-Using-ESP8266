// Include libraries for ESP8266 WiFi connectivity, I2C communication, and LCD control
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi and IFTTT Credentials
const char* ssid = "YOUR_SSID";     // WiFi network name
const char* password = "YOUR_PASSWORD"; // WiFi network password
const char* host = "maker.ifttt.com"; // IFTTT web service host name
const int httpPort = 80;   // Standard HTTP port

// ThingSpeak Settings
const char* tsHost = "api.thingspeak.com"; // ThingSpeak server address
const String tsKey = "YOUR_THINGSPEAK_API_KEY";   // ThingSpeak API key

// Hardware Pins
const int MOISTURE_SENSOR_PIN = A0;  // Analog pin connected to the soil moisture sensor
const int LED_PIN = D8;              // Digital pin for the LED indicator
const int BUZZER_PIN = D7;           // Digital pin for the buzzer
const int RELAY_PIN = D3;            // Digital pin for controlling a relay (pump)

// LCD Display setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize the LCD to be at I2C address 0x27, 16x2 characters

// Moisture threshold
const int MOISTURE_THRESHOLD = 700;  // Threshold for dry soil condition, adjust as necessary

WiFiClient client;  // Create an instance of the WiFiClient class

void setup() {
    Serial.begin(115200);              // Start serial communication at 115200 baud
    Serial.println("Setup start...");

    // Set pin modes for output devices
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    // Initialize the output devices as OFF
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RELAY_PIN, HIGH);     // Relay normally open

    lcd.init();                        // Initialize the LCD
    lcd.backlight();                   // Turn on the backlight

    Serial.println("Hardware initialized...");
    connectToWiFi();                   // Connect to WiFi
    delay(1000);                       // Wait a second for stability
    lcd.clear();                       // Clear LCD display
}

void loop() {
    Serial.println("Reading moisture sensor...");
    int moistureLevel = analogRead(MOISTURE_SENSOR_PIN); // Read moisture level from sensor
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");
    lcd.print(moistureLevel);          // Display moisture level on LCD
    Serial.print("Moisture level: ");
    Serial.println(moistureLevel);

    int pumpStatus = 0;                // Assume the pump is off initially

    // Check if moisture level is below the threshold
    if (moistureLevel > MOISTURE_THRESHOLD) {
        Serial.println("Moisture below threshold! Activating system...");
        digitalWrite(LED_PIN, HIGH);   // Turn on LED
        digitalWrite(BUZZER_PIN, HIGH);// Turn on buzzer
        digitalWrite(RELAY_PIN, LOW);  // Turn on relay (pump on)
        pumpStatus = 1;                // Update pump status to on
        delay(2000);                   // Keep on for 2 seconds
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        lcd.setCursor(0, 1);
        lcd.print("Status: Watering  ");
        sendToThingSpeak(moistureLevel, pumpStatus); // Send data to ThingSpeak
        triggerIFTTT();                // Trigger IFTTT action
    } else {
        Serial.println("Moisture level OK.");
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(RELAY_PIN, HIGH); // Relay off (pump off)

        lcd.setCursor(0, 1);
        lcd.print("Status: OK         ");
        sendToThingSpeak(moistureLevel, pumpStatus);  // Send current status to ThingSpeak
    }
    delay(60000);  // Delay for a minute before rechecking
}

// Function to connect to WiFi network
void connectToWiFi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    lcd.setCursor(0, 1);
    lcd.print("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("WiFi setup complete.");
}

// Function to send data to ThingSpeak
void sendToThingSpeak(int moistureValue, int pumpStatus) {
    Serial.println("Sending data to ThingSpeak...");

    if (!client.connect(tsHost, httpPort)) {
        Serial.println("Connection to ThingSpeak failed");
        return;
    }

    // Build the URL for the GET request with additional fields
    String url = "/update?api_key=" + tsKey;
    url += "&field1=" + String(moistureValue);  // Field 1 for raw moisture value
    url += "&field2=" + String(moistureValue);  // Field 2 for moisture level 
    url += "&field3=" + String(pumpStatus);     // Field 3 for pump status

    // Make the HTTP GET request
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tsHost + "\r\n" +
                 "Connection: close\r\n\r\n");
    Serial.println("Data sent to ThingSpeak.");
}

// Function to trigger an IFTTT event
void triggerIFTTT() {
    Serial.println("Triggering IFTTT...");
    if (!client.connect(host, httpPort)) {
        Serial.println("Connection to IFTTT failed");
        return;
    }
    String url = "/trigger/Soil_Moisture_Low/json/with/key/YOUR_IFTTT_KEY";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    Serial.println("IFTTT trigger sent.");
}
