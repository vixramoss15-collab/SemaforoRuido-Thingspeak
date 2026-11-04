#include <ESP8266WiFi.h>
#include "ThingSpeak.h"

// --- Configuración de Red Wi-Fi ---
const char* ssid = "Personal-801";          
const char* password = "jhNwmj2tYw";   

// --- Configuración de ThingSpeak ---
unsigned long myChannelNumber = 3525715;       // Tu ID de canal
const char* myWriteAPIKey = "EBB94WPYJTBH4II6"; // Tu API Key de escritura
const unsigned long postingInterval = 15000;     // Intervalo de 20 segundos
unsigned long lastConnectionTime = 0;

// --- Pines de Hardware ---
#define MICROFONO A0
#define LED_VERDE D5
#define LED_AMARILLO D6
#define LED_ROJO D7

// --- Umbrales de sonido ---
#define UMBRAL_VERDE 10
#define UMBRAL_ROJO 32

#define MUESTRAS 4
int nivelActual = 0;

WiFiClient client;

// --- Funciones auxiliares para los LEDs ---
void apagarLeds() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);
}

void encenderVerde() {
  apagarLeds();
  digitalWrite(LED_VERDE, HIGH);
}

void encenderAmarillo() {
  apagarLeds();
  digitalWrite(LED_AMARILLO, HIGH);
}

void encenderRojo() {
  apagarLeds();
  digitalWrite(LED_ROJO, HIGH);
}

// --- Medición de ruido por rango (máximo - mínimo) ---
int medirSonido() {
  int minimo = 1023;
  int maximo = 0;

  for (int i = 0; i < 100; i++) {
    int lectura = analogRead(MICROFONO);
    if (lectura < minimo) minimo = lectura;
    if (lectura > maximo) maximo = lectura;
    delay(2);
  }

  return maximo - minimo;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  apagarLeds();
  encenderVerde();

  // Conexión a la red Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡Conectado a Wi-Fi!");

  // Inicializar cliente ThingSpeak
  ThingSpeak.begin(client);
  Serial.println("SEMÁFORO DE RUIDO INICIADO");
}

void loop() {
  long suma = 0;

  for (int i = 0; i < MUESTRAS; i++) {
    suma += medirSonido();
  }

  nivelActual = suma / MUESTRAS;

  Serial.print("Nivel de ruido: ");
  Serial.println(nivelActual);

  // Control del semáforo según umbrales
  if (nivelActual <= UMBRAL_VERDE) {
    encenderVerde();
    Serial.println("ESTADO: SILENCIO - VERDE");
  }
  else if (nivelActual < UMBRAL_ROJO) {
    encenderAmarillo();
    Serial.println("ESTADO: RUIDO MODERADO - AMARILLO");
  }
  else {
    encenderRojo();
    Serial.println("ESTADO: RUIDO ALTO - ROJO");
  }

  // Envío de datos utilizando la librería oficial (Mucho más robusto ante tormentas/cortes)
  if (millis() - lastConnectionTime >= postingInterval) {
    int x = ThingSpeak.writeField(myChannelNumber, 1, nivelActual, myWriteAPIKey);
    
    if (x == 200) {
      Serial.println("Canal actualizado correctamente.");
    } else {
      Serial.print("Fallo en la conexión a ThingSpeak. Código HTTP: ");
      Serial.println(x);
    }
    
    lastConnectionTime = millis();
  }

  Serial.println("-----------------------");
  delay(1000);
}