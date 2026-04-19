#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "PMS.h"
#include <DHT.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <WiFiManager.h> 

// --- ThingSpeak Configuration ---
unsigned long myChannelNumber = 3269425;     
const char * myWriteAPIKey = "KYHRATME8XYZ3IUF"; 

WiFiClient client;
unsigned long lastThingSpeakTime = 0;
const unsigned long thingSpeakInterval = 20000; 

// --- Display Configuration ---
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW 
#define MAX_DEVICES 8  
#define CS_PIN 5       
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// --- Sensor Configurations ---
#define PMS_RX 26
#define PMS_TX 27
PMS pms(Serial2);
PMS::DATA data;

#define MQ135_PIN 34
#define BUZZER_PIN 15
#define SMOKE_PERCENT_THRESHOLD 80 
#define TEMP_THRESHOLD 35.0  

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- Variables ---
char displayMessage[250] = "Waiting for data..."; 
unsigned long lastDataTime = 0;
const unsigned long timeout = 4000; 

byte displayState = 0;
int cycleCount = 0; 

bool isAlarmActive = false;         
unsigned long lastBuzzerTime = 0;   
const int buzzerInterval = 100;     
bool buzzerState = LOW;             

// --- Official EPA AQI Formula ---
int calculateAQI(int pm25) {
  float c = (float)pm25;
  float cLow, cHigh, iLow, iHigh;

  if (c <= 12.0) { cLow = 0.0; cHigh = 12.0; iLow = 0.0; iHigh = 50.0; } 
  else if (c <= 35.4) { cLow = 12.1; cHigh = 35.4; iLow = 51.0; iHigh = 100.0; } 
  else if (c <= 55.4) { cLow = 35.5; cHigh = 55.4; iLow = 101.0; iHigh = 150.0; } 
  else if (c <= 150.4) { cLow = 55.5; cHigh = 150.4; iLow = 151.0; iHigh = 200.0; } 
  else if (c <= 250.4) { cLow = 150.5; cHigh = 250.4; iLow = 201.0; iHigh = 300.0; } 
  else if (c <= 350.4) { cLow = 250.5; cHigh = 350.4; iLow = 301.0; iHigh = 400.0; } 
  else if (c <= 500.4) { cLow = 350.5; cHigh = 500.4; iLow = 401.0; iHigh = 500.0; } 
  else { return 500; }

  float aqi = ((iHigh - iLow) / (cHigh - cLow)) * (c - cLow) + iLow;
  return (int)(aqi + 0.5);
}

// --- Animation Functions ---
void playWelcomePart1() {
  myDisplay.print("Welcome");
  for (int i = 0; i <= 15; i++) { myDisplay.setIntensity(i); delay(125); } 
  for (int i = 15; i >= 0; i--) { myDisplay.setIntensity(i); delay(125); } 
  
  myDisplay.setIntensity(10); 
  myDisplay.print("Init..."); 
  delay(2000);

  myDisplay.displayClear();
  myDisplay.displayText("InhaleInsight Welcomes You", PA_CENTER, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!myDisplay.displayAnimate()) { yield(); }

  myDisplay.displayClear();
  myDisplay.displayText("Department of ECE || Guide : Prof. Deepti Malviya", PA_CENTER, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!myDisplay.displayAnimate()) { yield(); }

  myDisplay.displayClear();
  myDisplay.displayText("Designed by: Radhika | Anurag | Satyam | Raghvendra ", PA_CENTER, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!myDisplay.displayAnimate()) { yield(); }
}

void playWelcomePart2() {
  myDisplay.print("Warming sensor..");
  delay(5000);

  myDisplay.displayClear();
  myDisplay.displayText("Waiting for data....", PA_CENTER, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!myDisplay.displayAnimate()) { yield(); }
}

// --- Wi-Fi Manager Callback (Triggers when AP opens) ---
void configModeCallback(WiFiManager *myWiFiManager) {
  String ipStr = "IP: " + WiFi.softAPIP().toString();

  for (int i = 0; i < 5; i++) {
    myDisplay.displayClear();
    myDisplay.displayText("Connect with InhaleInsight to configure new wifi credentials", PA_CENTER, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    while (!myDisplay.displayAnimate()) { yield(); }

    myDisplay.displayClear();
    myDisplay.displayText(ipStr.c_str(), PA_CENTER, 40, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    while (!myDisplay.displayAnimate()) { yield(); }
  }
  
  myDisplay.displayClear();
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.print("Waiting...");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 

  myDisplay.begin();
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.setIntensity(10);

  playWelcomePart1();
  
  // --- UPDATED WIFI LOGIC ---
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  
  // Requirement: 120s timeout and retry 4-5 times
  wifiManager.setConnectTimeout(50); 
  wifiManager.setConfigPortalTimeout(120); 

  int wifiAttempts = 0;
  bool connected = false;

  while (!connected && wifiAttempts < 1) {
    myDisplay.print("Conn WiFi");
    if (wifiManager.autoConnect("InhaleInsight")) {
      connected = true;
    } else {
      wifiAttempts++;
    }
  }

  if (connected) {
    myDisplay.print("WiFi Conn.");
    delay(2000);
    ThingSpeak.begin(client);
    myDisplay.print("Cloud...");
    delay(2000);
    myDisplay.print("Cloud OK");
    delay(2000);
  } else {
    myDisplay.print("WiFi Fail");
    delay(2000);
    myDisplay.print("Offline..");
    delay(2000);
    // Requirement: Automatically switch to AP mode if connection fails
    WiFi.softAP("InhaleInsight_Offline");
  }

  playWelcomePart2();
  
  myDisplay.displayClear();
  myDisplay.displayText(displayMessage, PA_CENTER, 0, 5000, PA_PRINT, PA_PRINT);
  lastDataTime = millis(); 
}

void loop() {
  if (isAlarmActive) {
    if (millis() - lastBuzzerTime >= buzzerInterval) {
      lastBuzzerTime = millis();            
      buzzerState = !buzzerState;           
      digitalWrite(BUZZER_PIN, buzzerState); 
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = LOW;
  }

  if (pms.read(data)) {
    lastDataTime = millis(); 
  }

  if (myDisplay.displayAnimate()) { 
    int rawGas = analogRead(MQ135_PIN);
    int gasPercent = map(rawGas, 193, 3500, 0, 100);
    gasPercent = constrain(gasPercent, 0, 100);

    float tempOffset = 4.0; 
    float temp = dht.readTemperature() - tempOffset;
    float hum = dht.readHumidity();

    if (gasPercent > SMOKE_PERCENT_THRESHOLD || temp > TEMP_THRESHOLD) isAlarmActive = true; 
    else isAlarmActive = false; 

    int currentAQI = calculateAQI(data.PM_AE_UG_2_5);

    switch(displayState) {
      case 0: 
        myDisplay.setTextAlignment(PA_CENTER);
        myDisplay.setTextEffect(PA_PRINT, PA_PRINT);
        myDisplay.setPause(5000);
        if (millis() - lastDataTime > timeout) strcpy(displayMessage, "PM2.5:Fail");
        else snprintf(displayMessage, sizeof(displayMessage), "PM2.5: %d", data.PM_AE_UG_2_5);
        break;
        
      case 1: 
        myDisplay.setTextEffect(PA_PRINT, PA_PRINT);
        myDisplay.setPause(5000);
        if (isnan(temp)) strcpy(displayMessage, "Temp: Fail");
        else snprintf(displayMessage, sizeof(displayMessage), "Temp: %.0f C", temp);
        break;
        
      case 2: 
        myDisplay.setTextEffect(PA_PRINT, PA_PRINT);
        myDisplay.setPause(5000);
        if (isnan(hum)) strcpy(displayMessage, "Hum: Fail");
        else snprintf(displayMessage, sizeof(displayMessage), "Hum: %.0f %%", hum);
        break;
        
      case 3: 
        myDisplay.setTextEffect(PA_PRINT, PA_PRINT);
        myDisplay.setPause(5000);
        if (rawGas < 10) strcpy(displayMessage, "Pollutants: Fail");
        else snprintf(displayMessage, sizeof(displayMessage), "Pollutants: %d %%", gasPercent);
        break;
        
      case 4: 
        myDisplay.setTextEffect(PA_PRINT, PA_PRINT);
        myDisplay.setPause(5000);
        if (millis() - lastDataTime > timeout) strcpy(displayMessage, "AQI: Fail");
        else snprintf(displayMessage, sizeof(displayMessage), "AQI: %d", currentAQI);
        break;
        
      case 5: 
        myDisplay.setTextAlignment(PA_LEFT); 
        myDisplay.setTextEffect(PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        myDisplay.setSpeed(40);              
        myDisplay.setPause(0); 
        if (millis() - lastDataTime > timeout) strcpy(displayMessage, "Error");
        else {
          if (currentAQI <= 50) strcpy(displayMessage, "Air Quality is Good");
          else if (currentAQI <= 100) strcpy(displayMessage, "Air Quality is Moderate");
          else if (currentAQI <= 150) strcpy(displayMessage, "Air Quality is Unhealthy for sensitive");
          else if (currentAQI <= 200) strcpy(displayMessage, "Air Quality is Unhealthy");
          else if (currentAQI <= 300) strcpy(displayMessage, "Air Quality is Poor");
          else if (currentAQI <= 500) strcpy(displayMessage, "Air Quality is Hazardous");
          else strcpy(displayMessage, "Air Quality is Severe");
        }
        break;
    }

    displayState++;
    
    if (displayState > 5) {
      displayState = 0; 
      cycleCount++; 
      
      if (cycleCount >= 5) {
        cycleCount = 0; 

        // HARD RESET: Completely clear the text buffer and alignment
        displayMessage[0] = '\0'; // This "removes" the old Air Quality message
        myDisplay.displayClear();  // Clears the hardware modules

        myDisplay.setTextAlignment(PA_CENTER);
        
        // Persistent Display Requirement: Animation only plays if connected, 
        // or continues showing data otherwise.
      
          playWelcomePart1(); 
          playWelcomePart2(); 
        
        
        displayState = 0; 
        myDisplay.displayReset();

        

        myDisplay.displayClear();myDisplay.displayText(displayMessage, PA_CENTER, 0, 5000, PA_PRINT, PA_PRINT);
        lastDataTime = millis();


        
      }
    }

    myDisplay.displayReset();

    if (millis() - lastThingSpeakTime > thingSpeakInterval) {
      if(WiFi.status() == WL_CONNECTED){
        ThingSpeak.setField(1, data.PM_AE_UG_1_0);
        ThingSpeak.setField(2, data.PM_AE_UG_2_5);
        ThingSpeak.setField(3, data.PM_AE_UG_10_0);
        ThingSpeak.setField(4, gasPercent); 
        ThingSpeak.setField(5, temp);
        ThingSpeak.setField(6, hum);
        ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      }
      lastThingSpeakTime = millis(); 
    } 
  }
}
