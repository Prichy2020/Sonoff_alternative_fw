/*
This is an alternative firmware for sonoff basic. 
Using the http query we can turn the relay on/off. 
The wifi connection check is set and if the signal is lost, the sonoff resets...
DNS is available. Example // example:  http://sonoff_0.local/getdata?online=1&switchState=0&status1=1&status2=0&value1=10&value2=20&value3=30&value4=40

Toto je alternativni firmware pro sonoff basic.
Pomoci http dotazu můžeme vypnout/zapnout rele.
Je nastavena kontrola wifi připojení a pokud dojde ke ztrátě signálu, tak se sonoff resetuje...


*/
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

#ifndef STASSID
#define STASSID "------------"
#define STAPSK "-------------"
#endif

#define name "sonoff_0"

#define BUTTON          0                                    // (Don't Change for Original Sonoff, Sonoff SV, Sonoff Touch, Sonoff S20 Socket)
#define RELAY           12                                   
#define LED             13     

const char* ssid = STASSID;           // wifi ssid name
const char* password = STAPSK;        // wifi password

int blink_pause = 100;             //basic delay for blink blue led
bool led_status = true;
int lastExecutedMillis;

bool online;
bool switchState;
bool status1;
bool status2;
int value1;
int value2;
int value3;
int value4;

// TCP server at port 80 will respond to HTTP requests
WiFiServer server(80);
// example:  http://sonoff_0.local/getdata?online=1&switchState=0&status1=1&status2=0&value1=10&value2=20&value3=30&value4=40

void handleRequest(WiFiClient client, String request) {
  if (request.indexOf("GET /getdata") != -1) {
    // decoding request
   online = request.indexOf("online=") != -1 ? request.substring(request.indexOf("online=") + 7, request.indexOf("&", request.indexOf("online="))).toInt() : false;
   switchState = request.indexOf("switchState=") != -1 ? request.substring(request.indexOf("switchState=") + 12, request.indexOf("&", request.indexOf("switchState="))).toInt() : false;
   status1 = request.indexOf("status1=") != -1 ? request.substring(request.indexOf("status1=") + 8, request.indexOf("&", request.indexOf("status1="))).toInt() : false;
   status2 = request.indexOf("status2=") != -1 ? request.substring(request.indexOf("status2=") + 8, request.indexOf("&", request.indexOf("status2="))).toInt() : false;
   value1 = request.indexOf("value1=") != -1 ? request.substring(request.indexOf("value1=") + 7, request.indexOf("&", request.indexOf("value1="))).toInt() : 0;
   value2 = request.indexOf("value2=") != -1 ? request.substring(request.indexOf("value2=") + 7, request.indexOf("&", request.indexOf("value2="))).toInt() : 0;
   value3 = request.indexOf("value3=") != -1 ? request.substring(request.indexOf("value3=") + 7, request.indexOf("&", request.indexOf("value3="))).toInt() : 0;
   value4 = request.indexOf("value4=") != -1 ? request.substring(request.indexOf("value4=") + 7, request.indexOf("&", request.indexOf("value4="))).toInt() : 0;

    // Answer
    
    String response = "<html><body>";
    response += "<h1>Data from device:</h1>";
    response += "<p>online = " + String(online) + "</p>";
    response += "<p>switchState = " + String(switchState) + "</p>";
    response += "<p>status1 = " + String(status1) + "</p>";
    response += "<p>status2 = " + String(status2) + "</p>";
    response += "<p>value1 = " + String(value1) + "</p>";
    response += "<p>value2 = " + String(value2) + "</p>";
    response += "<p>value3 = " + String(value3) + "</p>";
    response += "<p>value4 = " + String(value4) + "</p>";
    response += "</body></html>";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println("");
    client.println(response);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println("");
    client.println("Page not found");
  }
}


void setup(void) {
  Serial.begin(115200);
  ESP.wdtEnable(WDTO_8S);      // watchdog setting
  
  WiFi.mode(WIFI_STA);         // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);      // Wait for connection
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "'name'.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(name)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) { delay(1000); }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void loop(void) {
    // Refresh watchdog
  ESP.wdtFeed();

    // Kontrola stavu WiFi připojení
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Restarting...");
    ESP.restart();
  }

  MDNS.update();
  if (millis() - lastExecutedMillis >= blink_pause) {
    
    lastExecutedMillis = millis(); // save the last executed time
     led_status=!led_status;
     if (led_status) {
       Serial.println("milis");
      digitalWrite(LED,  LOW);
     } else {Serial.println("!milis");
       digitalWrite(LED,HIGH); }
  }

  // Check if a client has connected
  WiFiClient client = server.available();
  
if (client) { // client active
    
    // Wait for data from client to become available
unsigned long startMillis = millis();
while (client.connected() && !client.available()) {
  if (millis() - startMillis >= 1000) {
    Serial.println("Timeout - no data received");
    break;
  }
  delay(1);
}

  // Read the HTTP request only if client is connected and data is available
  if (client.connected() && client.available()) {
    Serial.println("pred readstring");
    String request = client.readStringUntil('\r');

    // Handle the request only if there is a valid request
    if (!request.isEmpty()) {
      handleRequest(client, request);
    }
  }

  // Disconnect the client only if response has been sent
  if (!client.connected()) {
    Serial.println("stop klient");
    client.stop();}
    

   if (switchState){ digitalWrite(RELAY, HIGH);} else { digitalWrite(RELAY, LOW);}
}
