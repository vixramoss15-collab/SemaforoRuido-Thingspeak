#include <ESP8266WiFi.h>

// --- Configuración de Red Wi-Fi ---
const char* ssid = "Personal-801";         
const char* password = "jhNwmj2tYw";   

// --- Configuración de ThingSpeak ---
const char* server = "api.thingspeak.com";
String writeAPIKey = "EBB94WPYJTBH4II6"; 
const unsigned long postingInterval = 15000;      
unsigned long lastConnectionTime = 0;

// --- Pines de Hardware ---
#define MICROFONO A0
#define LED_VERDE D5
#define LED_AMARILLO D6
#define LED_ROJO D7

// --- Umbrales de sonido (del segundo código) ---
#define UMBRAL_VERDE 10
#define UMBRAL_ROJO 32

// --- Cantidad de mediciones para suavizar la lectura ---
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

// --- Función para enviar datos a ThingSpeak ---
void enviarDatosThingSpeak(int valor) {
  if (client.connect(server, 80)) {
    String postStr = writeAPIKey;
    postStr += "&field1=";
    postStr += String(valor); 
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.println("Datos enviados a ThingSpeak correctamente.");
  } else {
    Serial.println("Fallo en la conexión a ThingSpeak.");
  }
  client.stop();
}

void setup() {
  Serial.begin(115200);

  // Configurar pines de los LEDs como salida
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
  Serial.println("SEMÁFORO DE RUIDO INICIADO");
}

void loop() {
  long suma = 0;

  // Promediamos cuatro mediciones para suavizar la lectura
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

  // Envío de datos a ThingSpeak cada 15 segundos
  if (millis() - lastConnectionTime >= postingInterval) {
    enviarDatosThingSpeak(nivelActual);
    lastConnectionTime = millis();
  }

  Serial.println("-----------------------");
}