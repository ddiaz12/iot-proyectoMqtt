#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char* ssid = "INFINITUME7D8";
const char* password = "kasumikasumi";

const char* mqttServer = "k8da39e9.ala.us-east-1.emqxsl.com";
const int mqttPort = 8883;
const char* mqttUser = "server";
const char* mqttPassword = "password";
const char* mqttTopic = "monitores/esp32";  

const char* ca_cert = "-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure espClient;
PubSubClient client(espClient);

const int BUZZER_PIN = 5;
const int GAS_PIN = 26;
const int LED_PIN = 14;

#define DHTPIN 18
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

bool buzzerActive = false;
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

void callback(char* topic, byte* payload, unsigned int length) {
  // Manejo de mensajes MQTT recibidos
  Serial.println("Mensaje MQTT recibido:");
  Serial.print("Tema: ");
  Serial.println(topic);
  Serial.print("Contenido: ");

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Mensaje: ");
  Serial.println(message);

    // Procesa el mensaje JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message);

    // Verifica si hay una acción específica en el mensaje
  if (doc.containsKey("action")) {
    String action = doc["action"];

    // Realiza acciones basadas en la acción recibida
    if (action == "buzzer/on") {
      // Enciende el zumbador
      tone(BUZZER_PIN, 440);  // Puedes ajustar la frecuencia según tus necesidades
      Serial.println("Zumbador encendido");
    } else if (action == "buzzer/off") {
      // Apaga el zumbador
      noTone(BUZZER_PIN);
      Serial.println("Zumbador apagado");
    }
}

}

void setup() {
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  WiFi.begin(ssid, password);

  Serial.println("Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado a la red WiFi con dirección IP: ");
  Serial.println(WiFi.localIP());

  // Configuración del cliente MQTT con TLS/SSL
  espClient.setCACert(ca_cert);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("El cliente %s se conecta al servidor MQTT\n", client_id.c_str());

    if (client.connect(client_id.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Conexión exitosa al servidor MQTT");
    } else {
      Serial.print("Error de conexión, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 2 segundos");
      delay(2000);
    }
  }

  // Publicación y suscripción
  client.publish(mqttTopic, "¡Hola, soy ESP32 con TLS/SSL ^^!");
  client.subscribe(mqttTopic);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  // Control de tiempo para la solicitud GET (si es necesario)
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      // Aquí iría el código para la solicitud GET si es necesario
      Serial.println("Haciendo solicitud GET...");
    } else {
      Serial.println("WiFi Desconectado");
    }
    lastTime = millis();
  }

  // Lógica del sensor de gas
  int gasState = digitalRead(GAS_PIN);
  bool isGasDetected = (gasState == LOW);
  postSmoke(isGasDetected);

   // Activa el zumbador si se detecta gas
  if (isGasDetected) {
    tone(BUZZER_PIN, 440);  // Puedes ajustar la frecuencia según tus necesidades
    Serial.println("Zumbador activado debido a la detección de gas");
  } else {
    noTone(BUZZER_PIN);
  }

  // Verifica el intervalo de envío para los datos del sensor DHT11
  unsigned long tiempoActual = millis();
  if (tiempoActual - lastTime >= timerDelay) {
    // Lee datos del sensor DHT11
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();

    // Publica los datos en el tema MQTT
    enviarDatosDHT(temperatura, humedad);

    // Actualiza el tiempo anterior
    lastTime = tiempoActual;

    // Mensajes de depuración
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.print(" °C, Humedad: ");
    Serial.print(humedad);
    Serial.println(" %");
  }

  delay(1000);
}


void postSmoke(bool smokeDetected) {
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["smokeDetected"] = smokeDetected;

  String jsonStr;
  serializeJson(jsonDoc, jsonStr);

  client.publish(mqttTopic, jsonStr.c_str());
  Serial.println("Enviando estado del sensor de humo...");
}

void enviarDatosDHT(float temperatura, float humedad) {
  // Crea un objeto JSON con los datos del sensor DHT11
String payload = "{";
payload += "\"temperature\":";
payload += String(temperatura);
payload += ",";
payload += "\"humidity\":";
payload += String(humedad);
payload += "}";
client.publish(mqttTopic, payload.c_str());
Serial.println("Enviando datos del sensor DHT11...");
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Conectado al servidor MQTT");
    } else {
      Serial.print("Falló, rc=");
      Serial.print(client.state());
      Serial.println(" Reintentando en 5 segundos");
      delay(5000);
    }
  }
}
