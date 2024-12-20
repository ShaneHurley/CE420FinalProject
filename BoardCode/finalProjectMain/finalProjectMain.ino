#include <WiFi.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include "ArduinoLowPower.h"

// Wi-Fi credentials
//const char* ssid = "KULABS";
//const char* password = "WHX434][{c";

// Wi-Fi credentials
const char* ssid = "Delts";
const char* password = "1210Dupont";

const bool demo = true;

// Wi-Fi credentials
//const char* ssid = "HurleyHome";
//const char* password = "collinallisonshane";

// Wi-Fi credentials
//const char* ssid = "iPhone";
//const char* password = "Shaner04";

const int outputPins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

IPAddress ip; 

const char* server = "weather.shanehurley.com";
const int port = 8000;

const int Value_dry = 580;
const int Value_wet = 280;

JSONVar systemTypes = JSONVar();

struct Averages {
  float avgMaxTemp;
  float avgMinTemp;
  float precipitation;
};

const int SprinklerTime = 100000;

float zonePercent[9];


WiFiClient client;
Averages historicalAverages;
JSONVar historicalData;
JSONVar uiJson;
JSONVar weeklyForecast; // Declare weeklyForecast as a global variable
Averages forecastAverages;


void setup() {
  // Start serial communication
  Serial.begin(9600);

  // Connect to Wi-Fi
  connectToWiFi();

  for (int i = 0; i < 13; i++) {
    pinMode(outputPins[i], OUTPUT);
  }
  digitalWrite(outputPins[8], HIGH);

  systemTypes["wifiButton"] = 1;
  systemTypes["analogButton"] = 2;
  systemTypes["demoButton"] = 3;

  // Connect to Wi-Fi
  connectToWiFi();

  // Connect to the server and fetch JSON data
  int zipCode = getZipCode();
  if (zipCode == -1) {
    Serial.println("Failed to retrieve ZIP code");
  } else if (firstDigit(zipCode)!=4) {
    zipCode = 48504;
  }
  String serverData = connectToServer(server, port, zipCode);
  Serial.println("Received JSON data:");


  // Parse the received JSON data
  JSONVar jsonData = JSON.parse(serverData);

  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(jsonData) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }
  Serial.println((jsonData));

  weeklyForecast = jsonData["weekly_forecast"];
  historicalData = jsonData["historical_precipitation"];
  Serial.print(historicalData);

  // Calculate averages
  historicalAverages = calculateAverages(historicalData);
  forecastAverages = calculateAverages(weeklyForecast);

  String userInput = fetchUserInputFromServer();
  uiJson = JSON.parse(userInput);

}

void loop() {
  
  runSystem(forecastAverages, historicalAverages, uiJson);
  if (demo){
  delayWithDark(30000);
  }else{
    delayWithDark(84600000);
  }
  int zipCode = getZipCode();
  if (zipCode == -1) {
    Serial.println("Failed to retrieve ZIP code");
  } else if (firstDigit(zipCode)!=4) {
    zipCode = 48504;
  }
  String serverData = connectToServer(server, port, zipCode);
  Serial.println("Received JSON data:");


  // Parse the received JSON data
  JSONVar jsonData = JSON.parse(serverData);

  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(jsonData) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }

  weeklyForecast = jsonData["weekly_forecast"];
  historicalData = jsonData["historical_precipitation"];

  // Calculate averages
  historicalAverages = calculateAverages(historicalData);
  forecastAverages = calculateAverages(weeklyForecast);

  String userInput = fetchUserInputFromServer();
  uiJson = JSON.parse(userInput);
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  Serial.println(WiFi.status());
  delay(3000);
  Serial.println(WiFi.status());

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.println(WiFi.gatewayIP());
}

String connectToServer(const char* server, int port, int zipcode) {

  if (client.connect(server, port)) {
    Serial.println("Connected to server");

    // Build the URL using the zipcode
    String url = "/" + String(zipcode);

    // Send HTTP GET request
    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: " + String(server));
    client.println("Connection: close");
    client.println();

    // Wait for server response
    while (!client.available()) {
      delay(100);
    }

    // Read response
    String response = "";
    while (client.available()) {
      response += char(client.read());
    }

    client.stop();

    // Extract JSON payload from the HTTP response
    int jsonStart = response.indexOf("\r\n\r\n");
    if (jsonStart != -1) {
      return response.substring(jsonStart + 4);  // Return the JSON part
    } else {
      Serial.println("Failed to parse response");
      return "{}";
    }
  } else {
    Serial.println("Connection to server failed");
    return "{}";  // Return empty JSON if connection fails
  }
}

Averages calculateAverages(JSONVar weatherData) {
  float totalMaxTemp = 0;
  float totalMinTemp = 0;
  float totalPrecipitation = 0;
  int count = 0;

  for (size_t i = 3; i < weatherData.length()-2; i++) {
    JSONVar day = weatherData[i];
    totalMaxTemp += (float)(double)day["max_temp"];
    totalMinTemp += (float)(double)day["min_temp"];
    totalPrecipitation += (float)(double)day["precipitation"];
    count++;
  }

  Averages result;
  result.avgMaxTemp = totalMaxTemp / count;
  result.avgMinTemp = totalMinTemp / count;
  result.precipitation = totalPrecipitation;
  return result;
}

int getZipCode() {
  if (client.connect("ip-api.com", 80)) {
    Serial.println("Connected to IP Geolocation API via HTTP");
    
    // Send the HTTP request
    client.println("GET /json HTTP/1.1");
    client.println("Host: ip-api.com");
    client.println("Connection: close");
    client.println();

    // Wait for the response
    String response = "";
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 5000) { // 5-second timeout
      while (client.available()) {
        char c = client.read();
        response += c;
      }
    }

    client.stop(); // Close the connection

    // Parse the response for the ZIP code

    int zipIndex = response.indexOf('"zip"');
    if (zipIndex != -1) {
      int start = response.indexOf("zip") + 6;
      int end = start+5;
      String zipCode = response.substring(start, end);
      return zipCode.toInt();
    } else {
      Serial.println("ZIP code not found in the response.");
      return -1;
    }
  } else {
    Serial.println("Connection to ip-api.com failed.");
    return -1;
  }
}

String fetchUserInputFromServer() {
  const char* host = "shanehurley.com";
  int port = 8001;

  if (client.connect(host, port)) {
    Serial.println("Connected to user input server");
    
    // Send HTTP GET request
    String url = "/"; // Adjust this path if needed
    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Connection: close");
    client.println();

    // Wait for server response
    while (!client.available()) {
      delay(100);
    }

    // Read response
    String response = "";
    while (client.available()) {
      response += char(client.read());
    }

    client.stop();

    // Extract JSON payload from the HTTP response
    int jsonStart = response.indexOf("\r\n\r\n");
    if (jsonStart != -1) {
      return response.substring(jsonStart + 4); // Return the JSON part
    } else {
      Serial.println("Failed to parse response from user input server");
      return "{}";
    }
  } else {
    Serial.println("Connection to user input server failed");
    return "{}"; // Return empty JSON if connection fails
  }
}

int firstDigit(int n) { 
    while (n >= 10)  
        n /= 10; 
    return n; 
} 

float* zoneCalculator(Averages forecastAverages, Averages historicalAverages, JSONVar uiJson) {
  static float zonePercent[9]; // Use static to ensure the array persists outside the function

  if (demo) Serial.println("Starting zoneCalculator...");

  // Retrieve master switch and temperature
  bool masterSwitch = (bool)(uiJson["master"]);
  float avgTemp = forecastAverages.avgMinTemp;

  if (demo) {
    Serial.print("Master Switch: ");
    Serial.println(masterSwitch ? "ON" : "OFF");
    Serial.print("Average Temperature: ");
    Serial.println(avgTemp);
    Serial.print("systemContainer: ");
    Serial.println(uiJson["systemContainer"]);
  }

  // Check if the master switch is ON and temperature is above freezing
  if (masterSwitch && avgTemp >= 32 || ((String)uiJson["systemContainer"]).equals("demoButton")) {
    if (((String)uiJson["systemContainer"]).equals("demoButton") && demo) {
      Serial.println("BRRRRRR. ITS COLD BUT FOR TESTS ITS FINE");
    }

    if (demo) {
      Serial.print("Historical Precipitation: ");
      Serial.println(historicalAverages.precipitation);
    }

    if (historicalAverages.precipitation <= 10.5) {
      if (demo) Serial.println("Historical precipitation is low. Processing zones...");
      for (int i = 0; i < 9; i++) {
        int zoneStatus = (int)(uiJson["zones"][i]); // Check if zone is enabled
        if (demo) {
          Serial.print("Zone ");
          Serial.print(i);
          Serial.print(" status: ");
          Serial.println(zoneStatus == 1 ? "ENABLED" : "DISABLED");
        }

        if (zoneStatus == 1) {
          int sensorValue = analogRead(i); // Read sensor value for the zone
          if (demo) {
            Serial.print("Zone ");
            Serial.print(i);
            Serial.print(" sensor value: ");
            Serial.println(sensorValue);
          }

          // Map sensor value to moisture percentage
          float moisturePercent = map(sensorValue, Value_dry, Value_wet, 0, 100);
          if (demo) {
            Serial.print("Zone ");
            Serial.print(i);
            Serial.print(" moisture percentage: ");
            Serial.println(moisturePercent);
          }

          if ((forecastAverages.precipitation < 0.75) || (moisturePercent < 40)) {
            if (demo) {
              Serial.print("Forecast precipitation is low. Checking moisture for Zone ");
              Serial.println(i);
            }

            if (moisturePercent < 75) {
              // Calculate adjusted moisture percentage
              zonePercent[i] = (1 - (moisturePercent/100)) * (double)uiJson["sensor"];
              if (demo) {
                Serial.print("Zone ");
                Serial.print(i);
                Serial.print(" adjusted moisture: ");
                Serial.println(zonePercent[i]);
                
              } else if (demo) {
                Serial.print("Zone ");
                Serial.print(i);
                Serial.println(" moisture is too high. Skipping adjustment.");
              }
            } else if (demo) {
              zonePercent[i] = 0;
              Serial.print("Zone ");
              Serial.print(i);
              Serial.println(" moisture is too high. Skipping adjustment.");
            }
          } else if (demo) {

            Serial.println("Forecast precipitation is sufficient. Skipping all zones.");
          }
        }
      }
    } else if (demo) {
      for (int i = 0; i < 9; i++) {
      zonePercent[i] = 0;
    }
      Serial.println("Historical precipitation is sufficient. Skipping zones.");
    }
  } else if (demo) {
    for (int i = 0; i < 9; i++) {
      zonePercent[i] = 0;
    }
    Serial.println("Master switch is OFF or temperature is too low. Skipping zone calculations.");
  }

  if (demo) Serial.println("Zone calculation completed.");
  return zonePercent; // Return the array
}

void runSystem(Averages forecastAverages, Averages historicalAverages, JSONVar uiJson) {
  Serial.println("Running System");
  float* zonePercent = zoneCalculator(forecastAverages, historicalAverages, uiJson);
  for (int i = 0; i <= 9; i++){
    Serial.print(zonePercent[i]);Serial.print("=Zone");Serial.println(i);
    if (zonePercent[i]!=0){
      int zoneTime = (int) (zonePercent[i] * SprinklerTime);
      Serial.print("zoneTime: ");Serial.println(zoneTime);
      if (((String)uiJson["systemContainer"]).equals("demoButton")){
        zoneTime = (int)(zoneTime/9);
        Serial.print("zoneTime: ");Serial.println(zoneTime);
      }

      if (zoneTime>100){
        Serial.println("turnOnSprinkler ");
        turnOnSprinkler((String)uiJson["systemContainer"],i);
        delayWithLight(zoneTime,i);
        Serial.println("turnOffSprinkler ");
        turnOffSprinkler((String)uiJson["systemContainer"],i);
      }
    }
  }
}

void turnOnSprinkler(String systemContainer,int zone){
  switch ((int)systemTypes[systemContainer]) {
    case 1: 
      {
        // Create the URL dynamically based on the zone
        const String url = String("https://api.rach.io/") + zone + "/public/device/on";
        
        // Initialize WiFiClient
        WiFiClient client;
        
        // Connect to the server (ensure it's HTTPS or adjust for HTTP)
        if (!client.connect("api.rach.io", 443)) {  // Using port 443 for HTTPS
          Serial.println("Connection failed!");
          return;
        }

        // Construct the HTTP request headers
        String httpRequest = "PUT " + url + " HTTP/1.1\r\n";
        httpRequest += "Host: api.rach.io\r\n";
        httpRequest += "Accept: application/json\r\n";
        httpRequest += "Content-Type: application/json\r\n";
        httpRequest += "Connection: close\r\n\r\n";  // End of headers
        
        // Send the HTTP request
        client.print(httpRequest);

        // Read the server response
        String response = "";
        while (client.available()) {
          response += (char)client.read();
        }

        // Print the response for debugging
        Serial.println("Response:");
        Serial.println(response);
        
        // Close the connection
        client.stop();
      }
      break;

    default:
      digitalWrite(outputPins[zone], HIGH);
      break;
  }
}

void turnOffSprinkler(String systemContainer,int zone){
  switch ((int)systemTypes[systemContainer]) {
    case 1: 
      {
        // Create the URL dynamically based on the zone
        const String url = String("https://api.rach.io/") + zone + "/public/device/off";
        
        // Initialize WiFiClient
        WiFiClient client;
        
        // Connect to the server (ensure it's HTTPS or adjust for HTTP)
        if (!client.connect("api.rach.io", 443)) {  // Using port 443 for HTTPS
          Serial.println("Connection failed!");
          return;
        }

        // Construct the HTTP request headers
        String httpRequest = "PUT " + url + " HTTP/1.1\r\n";
        httpRequest += "Host: api.rach.io\r\n";
        httpRequest += "Accept: application/json\r\n";
        httpRequest += "Content-Type: application/json\r\n";
        httpRequest += "Connection: close\r\n\r\n";  // End of headers
        
        // Send the HTTP request
        client.print(httpRequest);

        // Read the server response
        String response = "";
        while (client.available()) {
          response += (char)client.read();
        }

        // Print the response for debugging
        Serial.println("Response:");
        Serial.println(response);
        
        // Close the connection
        client.stop();
      }
      break;

    default:
      digitalWrite(outputPins[zone], LOW);
      break;
  }
}

void delayWithLight(int time,int pin){
  for (int i =0; i <=time;i++){
    digitalWrite(pin, HIGH);
    delay(1);
  }
}

void delayWithDark(int time){
  for (int i =0; i <=time;i++){
    for (int pin = 0; pin < 13; pin++) {
      digitalWrite(pin, LOW);
    }
    delay(1);
  }
}

