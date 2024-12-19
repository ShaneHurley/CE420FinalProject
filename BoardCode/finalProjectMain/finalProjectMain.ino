#include <WiFi.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include "ArduinoLowPower.h"

// Wi-Fi credentials
//const char* ssid = "KULABS";
//const char* password = "WHX434][{c";

// Wi-Fi credentials
//const char* ssid = "Delts";
//const char* password = "1210Dupont";

// Wi-Fi credentials
//const char* ssid = "HurleyHome";
//const char* password = "collinallisonshane";

// Wi-Fi credentials
const char* ssid = "iPhone";
const char* password = "Shaner04";

IPAddress ip; 

// Define the server URL you want to connect to
const char* server = "weather.shanehurley.com";
const int port = 8000;
const int zipcode = 48504;

const int Value_dry = 580;
const int Value_wet = 280;

struct Averages {
  float avgMaxTemp;
  float avgMinTemp;
  float avgPrecipitation;
};

float zonePercent[9];

WiFiClient client;

void setup() {
  // Start serial communication
  Serial.begin(9600);

  // Connect to Wi-Fi
  connectToWiFi();

  // Connect to the server and fetch JSON data
  int zipCode = getZipCode();
  if (zipCode != -1) {
    Serial.print("ZIP Code: ");
    Serial.println(zipCode);
  } else {
    Serial.println("Failed to retrieve ZIP code");
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

  JSONVar weeklyForecast = jsonData["weekly_forecast"];
  JSONVar historicalData = jsonData["historical_precipitation"];

  // Calculate averages
  Averages historicalAverages = calculateAverages(historicalData);
  Averages forecastAverages = calculateAverages(weeklyForecast);

  Serial.println("Historical Averages:");
  Serial.print("Avg Max Temp: ");
  Serial.println(historicalAverages.avgMaxTemp);
  Serial.print("Avg Min Temp: ");
  Serial.println(historicalAverages.avgMinTemp);
  Serial.print("Avg Precipitation: ");
  Serial.println(historicalAverages.avgPrecipitation);

  Serial.println("\nForecast Averages:");
  Serial.print("Avg Max Temp: ");
  Serial.println(forecastAverages.avgMaxTemp);
  Serial.print("Avg Min Temp: ");
  Serial.println(forecastAverages.avgMinTemp);
  Serial.print("Avg Precipitation: ");
  Serial.println(forecastAverages.avgPrecipitation);
}

void loop() {
  int SensorValue = analogRead(A0);
  Serial.print("Sensor Value: ");
  Serial.println(SensorValue);
  float MoisturePercent = map(SensorValue, Value_dry, Value_wet, 0, 100);
  if (MoisturePercent > 100)  //correct the percentage to 100% if read over 100.
  {
    Serial.print("Moisture Percent: ");
    Serial.println("100 %");
  } else if (MoisturePercent < 0)  //correct the percentage to 0% if read less than 0.
  {
    Serial.print("Moisture Percent: ");
    Serial.println("0 %");
  } else {
    Serial.print("Moisture Percent: ");
    Serial.print(MoisturePercent);
    Serial.println("%");
    delay(250);
  }
  delay(30000);
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

  for (size_t i = 0; i < weatherData.length(); i++) {
    JSONVar day = weatherData[i];
    totalMaxTemp += (float)(double)day["max_temp"];
    totalMinTemp += (float)(double)day["min_temp"];
    totalPrecipitation += (float)(double)day["precipitation"];
    count++;
  }

  Averages result;
  result.avgMaxTemp = totalMaxTemp / count;
  result.avgMinTemp = totalMinTemp / count;
  result.avgPrecipitation = totalPrecipitation / count;
  return result;
}

//float[] zoneCalculator(JSONVar weatherData, Zones current){

//}
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
    Serial.println("Response:");
    Serial.println(response);

    int zipIndex = response.indexOf('"zip"');
    if (zipIndex != -1) {
      int start = response.indexOf("zip") + 6; // Start of ZIP code
      int end = start+5;             // End of ZIP code
      Serial.println(start);
      Serial.println(end);
      String zipCode = response.substring(start, end);     // Extract the ZIP code
      Serial.println("ZIP Code: " + zipCode);
      return zipCode.toInt();                              // Convert to integer
    } else {
      Serial.println("ZIP code not found in the response.");
      return -1;
    }
  } else {
    Serial.println("Connection to ip-api.com failed.");
    return -1;
  }
}



