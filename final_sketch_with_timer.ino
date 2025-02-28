#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Eduroam Wi-Fi credentials
#define EAP_USERNAME "23b0430@iitb.ac.in"
#define EAP_IDENTITY "23b0430@iitb.ac.in"
#define EAP_PASSWORD "Your password"
const char *ssid = "eduroam";

// Washing Machine LDR Sensor
#define LDR_SENSOR_PIN 34  // Connect LDR sensor output to GPIO34
#define LED_PIN 2          // ESP32 onboard LED

AsyncWebServer server(80);
String machineStatus = "Unknown";
unsigned long startTime = 0;
bool isMachineOn = false;

void connectToEduroam() {
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
    Serial.print("Connecting to Eduroam...");
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
        counter++;
        if (counter >= 30) {
            Serial.println("\nConnection Failed! Restarting...");
            ESP.restart();
        }
    }
    Serial.println("\nConnected to Eduroam!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    pinMode(LDR_SENSOR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    connectToEduroam();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        unsigned long elapsedTime = (isMachineOn) ? (millis() - startTime) / 60000 : 0;
        int remainingTime = (isMachineOn) ? max(0, 90 - (int)elapsedTime) : 90;
        
        String html = "<html><head><title>Washing Machine Status</title>";
        html += "<meta http-equiv='refresh' content='5'>";
        html += "<style>";
        html += "body { font-family: 'Roboto Mono', monospace; background-color: #1e272e; color: #f0f0f0; text-align: center; padding: 50px; }";
        html += "h1 { color: #66d9ef; }";
        html += "#status { font-size: 40px; padding: 20px; border-radius: 15px; display: inline-block; font-weight: bold; background-color: " + String((machineStatus == "ON") ? "green" : "red") + "; }";
        html += "</style></head><body>";
        html += "<h1>Washing Machine Status</h1>";
        html += "<div id='status'>Washing Machine is " + machineStatus + "</div>";
        html += "<p>Time ON: " + String(elapsedTime) + " min</p>";
        html += "<p>Estimated time left: " + String(remainingTime) + " min</p>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    server.begin();
}

void loop() {
    int ldrValue = analogRead(LDR_SENSOR_PIN);
    Serial.print("LDR Value: ");
    Serial.println(ldrValue);
    
    if (ldrValue > 1300) {
        delay(500);
        int secondRead = analogRead(LDR_SENSOR_PIN);
        if (abs(secondRead - ldrValue) > 1000) {
            if (isMachineOn) {
                isMachineOn = false;
                machineStatus = "OFF";
                digitalWrite(LED_PIN, LOW);
            }
        } else {
            if (!isMachineOn) {
                isMachineOn = true;
                startTime = millis();
            }
            machineStatus = "ON";
            digitalWrite(LED_PIN, HIGH);
        }
    } else {
        if (isMachineOn) {
            isMachineOn = false;
            machineStatus = "OFF";
            digitalWrite(LED_PIN, LOW);
        }
    }

    Serial.println("Washing Machine is " + machineStatus);
    delay(2000);
}
