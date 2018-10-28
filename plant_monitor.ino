
#include "FS.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define AWS_ENDPOINT "a2kuvr4lb7qxrw.iot.us-east-1.amazonaws.com"
#define POLL_INTERVAL_MS 5000

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

WiFiClientSecure espClient;
PubSubClient client(AWS_ENDPOINT, 8883, callback, espClient); //set  MQTT port number to 8883 as per //standard

bool loadKeys() {
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return false;
  }

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

  // Load certificate file
  File cert = SPIFFS.open("/thing.der", "r");
  if (!cert) {
    Serial.println("Failed to open cert file");
    return false;
  }
  else {
    Serial.println("Success to open cert file");
  }

  delay(1000);

  if (espClient.loadCertificate(cert)) {
    Serial.println("cert loaded");
  } else {
    Serial.println("cert not loaded");
    return false;
  }

  // Load private key file
  File private_key = SPIFFS.open("/thing.private.der", "r");
  if (!private_key) {
    Serial.println("Failed to open private cert file");
    return false;
  }
  else {
    Serial.println("Success to open private cert file");
  }

  delay(1000);

  if (espClient.loadPrivateKey(private_key)) {
    Serial.println("private key loaded");
  } else {
    Serial.println("private key not loaded");
    return false;
  }

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
  return true;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPthing")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void ensureConnected() {
  if (!client.connected()) {
    reconnect();
  }
}

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(true);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  setup_wifi();
  delay(1000);
  if (!loadKeys()) {
    return;
  }
  ensureConnected();
  
  digitalWrite(LED_BUILTIN, HIGH);
}

long lastMsg = 0;
char msg[200];

void loop() {
  ensureConnected();
  client.loop();

  long now = millis();
  if (now - lastMsg > POLL_INTERVAL_MS) {
    lastMsg = now;
    
    int moisture = analogRead(0);
    snprintf (msg, 200, "{\"state\": {\"desired\": {\"moisture\": %ld}}}", moisture);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("$aws/things/parlor-palm/shadow/update", msg);
    
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
  }
}
