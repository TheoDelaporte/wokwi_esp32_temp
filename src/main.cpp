#include <Arduino.h>

#include "DHTesp.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

// --- CONFIGURATION ---
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *mqtt_server = MQTT_SERVER;
const char *mqtt_user = MQTT_USER;
const char *mqtt_password = MQTT_PASSWORD;
const int mqtt_port = MQTT_PORT;

const int ledPin = 2; // Broche de la LED virtuelle

WiFiClientSecure espClient;
PubSubClient client(espClient);
DHTesp dht;

// Fonction déclenchée automatiquement à la réception d'un message MQTT
void callback(char *topic, byte *payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(message);

  // Vérification du topic et exécution de l'ordre
  if (String(topic) == "projet/salon/lumiere") {
    if (message == "ON") {
      digitalWrite(ledPin, HIGH);
    } else if (message == "OFF") {
      digitalWrite(ledPin, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);

  dht.setup(15, DHTesp::DHT22); // DHT22 physiquement branché sur le pin 15
  pinMode(ledPin, OUTPUT);      // Configuration de la broche LED en sortie

  // 1. Connexion au Wi-Fi simulé
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connecté !");

  // 2. Configuration MQTT
  espClient.setInsecure(); // Obligatoire sur Wokwi pour ignorer la validation
                           // du certificat SSL

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // On indique au client quelle fonction appeler
                                // lors d'une réception
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connexion MQTT...");
    if (client.connect("ESP32_Wokwi_Client", mqtt_user, mqtt_password)) {
      Serial.println(" Connecté !");

      // On s'abonne au topic de la lumière UNIQUEMENT après une connexion
      // réussie
      client.subscribe("projet/salon/lumiere");
    } else {
      delay(5000); // Pause avant de retenter
    }
  }
}

unsigned long lastPublishTime = 0;

void loop() {
  if (!client.connected())
    reconnect();

  // client.loop() tourne en permanence, la LED s'allumera instantanément !
  client.loop();

  unsigned long now = millis();

  // On ne rentre ici que toutes les 2000 millisecondes (2 secondes)
  if (now - lastPublishTime > 2000) {
    lastPublishTime = now;

    float baseTemp = 22.0;
    float smoothFluctuation = sin(now / 5000.0) * 2.0;
    float noise = random(-5, 6) / 10.0;
    float simulatedTemp = baseTemp + smoothFluctuation + noise;

    String payload = String(simulatedTemp);
    client.publish("projet/salon/temperature", payload.c_str());
  }
}