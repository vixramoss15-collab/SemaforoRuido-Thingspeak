#include <ESP8266WiFi.h>
#include "ThingSpeak.h"

const char* ssid = "Personal-801";          
const char* password = "jhNwmj2tYw";   

unsigned long myChannelNumber = 3525715;       
const char* myWriteAPIKey = "EBB94WPYJTBH4II6"; 

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 15000;     

#define MICROFONO A0
#define LED_VERDE D5
#define LED_AMARILLO D6
#define LED_ROJO D7

#define UMBRAL_VERDE 10
#define UMBRAL_ROJO 32

int nivelActual = 0;
bool modoManualAlerta = false;
unsigned long ultimaLecturaComando = 0;

WiFiClient client;

void apagarLeds() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  
  apagarLeds();
  digitalWrite(LED_VERDE, HIGH);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado.");
  ThingSpeak.begin(client);
}

void loop() {
  // 1. Medir sonido rápidamente
  long suma = 0;
  for (int i = 0; i < 4; i++) {
    int minimo = 1023, maximo = 0;
    for (int j = 0; j < 50; j++) {
      int val = analogRead(MICROFONO);
      if (val < minimo) minimo = val;
      if (val > maximo) maximo = val;
      delay(1);
    }
    suma += (maximo - minimo);
  }
  nivelActual = suma / 4;
  Serial.print("Nivel de ruido: ");
  Serial.println(nivelActual);

  // 2. Consultar comando manual a ThingSpeak cada 5 segundos
  if (millis() - ultimaLecturaComando > 5000) {
    long comando = ThingSpeak.readLongField(myChannelNumber, 2);
    if (ThingSpeak.getLastReadStatus() == 200) {
      Serial.print("Comando leído del Field 2: ");
      Serial.println(comando);
      
      if (comando > 0) {
        modoManualAlerta = true;
        Serial.println(">>> MODO MANUAL: LED ROJO FORZADO");
      } else {
        modoManualAlerta = false;
        Serial.println(">>> MODO AUTOMÁTICO ACTIVADO");
      }
    }
    ultimaLecturaComando = millis();
  }

  // 3. Actuar sobre los LEDs según el modo
  apagarLeds();
  if (modoManualAlerta) {
    digitalWrite(LED_ROJO, HIGH);
    Serial.println("ESTADO: ROJO FIJO (FORZADO)");
  } else {
    if (nivelActual <= UMBRAL_VERDE) {
      digitalWrite(LED_VERDE, HIGH);
      Serial.println("ESTADO: VERDE (Silencio)");
    } else if (nivelActual < UMBRAL_ROJO) {
      digitalWrite(LED_AMARILLO, HIGH);
      Serial.println("ESTADO: AMARILLO (Moderado)");
    } else {
      digitalWrite(LED_ROJO, HIGH);
      Serial.println("ESTADO: ROJO (Alto)");
    }
  }

  // 4. Enviar datos al Field 1 cada 15 segundos
  if (millis() - lastConnectionTime > postingInterval) {
    int x = ThingSpeak.writeField(myChannelNumber, 1, nivelActual, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Datos de ruido subidos a ThingSpeak correctamente.");
    }
    lastConnectionTime = millis();
  }

  Serial.println("-----------------------------------");
  delay(1000);
}