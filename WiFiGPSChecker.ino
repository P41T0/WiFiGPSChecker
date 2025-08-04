#include <TinyGPS++.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <config.h>
#define RX2 16
#define TX2 17

#define gpsb 9600
const int maxXarxes = 50;
String ssidUniques[maxXarxes];
bool connected;
TinyGPSPlus gps;

WiFiClient espClient;
PubSubClient client(espClient);
int compteUnic = 0;
int numWifis;
float timer;
float timerGPS;
float latitud;
float longitud;
bool gpsavailable;
HardwareSerial gps_serie(2);


void OnMqttReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Missatge rebut en el topic: ");
  Serial.println(topic);


  String json;
  for (int i = 0; i < length; i++) {
    json += (char)payload[i];
    Serial.print((char)payload[i]);
  }

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("Error en deserialitzar el JSON: ");
    Serial.println(error.c_str());
    return;
  }
}

void InitMqtt() {
  client.setServer(serverIp, port);
  client.setCallback(OnMqttReceived);
  Serial.print("connected to server");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //timer = millis();
  int tryDelay = 500;
  int numberOfTries = 25;
  Serial.print("entro aqui");
  numWifis = 0;
  timer = millis();
  timerGPS = millis();
  longitud = 0;
  latitud = 0;
  gpsavailable = false;
  gps_serie.begin(gpsb, SERIAL_8N1, RX2, TX2);
  Serial.println("gps iniciat");
  WiFi.begin(SSID_connect, pwd);

  while (true) {
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL: Serial.println("[WiFi] SSID not found"); break;
      case WL_CONNECT_FAILED:
        Serial.print("[WiFi] Error, No se ha connectat al WiFi! Motiu: ");
        return;
        break;
      case WL_CONNECTION_LOST: Serial.println("[WiFi] Connexió perduda"); break;
      case WL_SCAN_COMPLETED: Serial.println("[WiFi] Escaneig completat"); break;
      case WL_DISCONNECTED: Serial.println("[WiFi] WiFi desconnectat"); break;
      case WL_CONNECTED:
        connected = true;
        Serial.println("[WiFi] WiFi connectat!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
        InitMqtt();

        return;
        break;
      default:
        Serial.print("[WiFi] Estat del WiFi: ");
        Serial.println(WiFi.status());
        break;
    }
    delay(tryDelay);

    if (numberOfTries <= 0) {
      Serial.print("[WiFi] No s’ha pogut connectar al WiFi!");
      WiFi.disconnect();
      return;
    } else {
      numberOfTries--;
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe("sensorica");
      Serial.print("connectat");
    } else {
      Serial.print("estat: ");
      Serial.println(client.state());
      delay(2500);
    }
  }
}

void loop() {
  if (connected == true) {
    if (!client.connected()) {
      reconnect();
    }
    if (millis() - timerGPS > 2500) {
      timerGPS = millis();
      while (gps_serie.available() > 0) {
        // get the byte data from the GPS
        gps.encode(gps_serie.read());
      }
      if (gps.location.isUpdated()) {
        // Mostra les coordenades si s'han actualitzat

        longitud = gps.location.lng();
        latitud = gps.location.lat();
        gpsavailable = true;
      } else {
        // Indica que encara no hi ha fix GPS
        gpsavailable = false;
      }
    }


    if (millis() - timer > 5000) {
      timer = millis();
      numWifis = WiFi.scanNetworks();
      if (numWifis > 0) {
        for (int i = 0; i < compteUnic; i++) {
          ssidUniques[compteUnic] = "";
        }
        compteUnic = 0;
        // put your main code here, to run repeatedly:
        DynamicJsonDocument jsonDoc(512);
        jsonDoc["latitud"] = latitud;
        jsonDoc["longitud"] = longitud;
        jsonDoc["gpsAvailable"] = gpsavailable;
        for (int i = 0; i < numWifis; i++) {
          String nomWifi = WiFi.SSID(i);
          bool xarxaDuplicada = false;
          for (int j = 0; j < compteUnic; j++) {
            if (ssidUniques[j] == nomWifi) {
              xarxaDuplicada = true;
              break;
            }
          }
          if (xarxaDuplicada == false) {
            ssidUniques[compteUnic] = nomWifi;
            compteUnic++;
            if (compteUnic >= maxXarxes) {
              break;
            }
          }
        }
        for (int i = 0; i < compteUnic; i++) {
          jsonDoc["SSID"] = ssidUniques[i];
          char jsonserialitzat[512];
          serializeJson(jsonDoc, jsonserialitzat);
          client.publish("sensorica", jsonserialitzat);
        }
      }
    }
  }
}
