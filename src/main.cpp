#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NewPing.h>
#include <esp_sleep.h>

// Pins pour l'AJ-SR04M
#define AJSR04M_TRIG 12
#define AJSR04M_ECHO 13
#define MAX_DISTANCE 400 // Distance max en cm (ajuste selon ton bassin)
#define MIN_DISTANCE 2   // Distance min en cm (limite typique de l'AJ-SR04M)

// Initialisation du capteur à ultrasons
NewPing sonar(AJSR04M_TRIG, AJSR04M_ECHO, MAX_DISTANCE);

// Pins pour l'OLED (LilyGO T3 V1.6.1)
#define MY_OLED_SDA 21
#define MY_OLED_SCL 22
#define MY_OLED_RST 4
#define SCREEN_WIDTH 128 // Largeur de l'écran OLED
#define SCREEN_HEIGHT 64 // Hauteur de l'écran OLED

// Pins pour LoRa (basés sur la doc LilyGO T3 V1.6.1)
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 23
#define LORA_DIO0 26

// Paramètres LoRa (France, 868 MHz)
#define LORA_FREQ 868E6 // Fréquence 868 MHz
#define LORA_BW 125E3   // Bandwidth 125 kHz
#define LORA_SF 7       // Spreading Factor 7
#define LORA_CR 5       // Coding Rate 4/5
#define LORA_PWR 10     // Output Power 10 dBm
#define LORA_PREAMBLE 8 // Preamble Length 8
#define LORA_SYNC_WORD 0x12 // Sync Word

// Intervalle d'envoi (en millisecondes)
const long TEST_INTERVAL = 60000; // 1 minute pour les tests
const long PROD_INTERVAL = 14400000; // 4 heures
const long SEND_INTERVAL = PROD_INTERVAL; // Mode production

// Calibration pour la vitesse du son
const float TEMPERATURE = 15.0; // Ajuste selon la température réelle (°C)
const float SPEED_OF_SOUND = 331.0 + 0.6 * TEMPERATURE; // ~340 m/s
const float US_TO_CM = (2.0 * 1000000.0) / (SPEED_OF_SOUND * 100.0); // ~61.63 µs/cm

// Initialisation de l'écran OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, MY_OLED_RST);

void setup() {
  // Initialisation du port série
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("[DEBUG] Démarrage du programme");
  Serial.print("[DEBUG] Vitesse du son utilisée : ");
  Serial.print(SPEED_OF_SOUND);
  Serial.println(" m/s");
  Serial.print("[DEBUG] US_TO_CM : ");
  Serial.print(US_TO_CM);
  Serial.println(" µs/cm");

  // Réinitialisation de l'OLED
  Serial.println("[DEBUG] Réinitialisation de l'OLED...");
  pinMode(MY_OLED_RST, OUTPUT);
  digitalWrite(MY_OLED_RST, LOW);
  delay(20);
  digitalWrite(MY_OLED_RST, HIGH);

  // Initialisation de l'OLED
  Serial.println("[DEBUG] Initialisation de l'OLED...");
  Wire.begin(MY_OLED_SDA, MY_OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println("[DEBUG] Échec de l'initialisation de l'OLED");
    for (;;);
  }
  Serial.println("[DEBUG] OLED initialisé avec succès");
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("LORA SENDER");
  display.println("Initialisation...");
  display.display();

  // Test du capteur à ultrasons
  Serial.println("[DEBUG] Test du capteur AJ-SR04M...");
  unsigned long test_times[5];
  for (int i = 0; i < 5; i++) {
    test_times[i] = sonar.ping();
    Serial.print("[DEBUG] Mesure initiale ");
    Serial.print(i + 1);
    Serial.print(" (µs) : ");
    Serial.println(test_times[i]);
    Serial.print("[DEBUG] Distance calculée ");
    Serial.print(i + 1);
    Serial.print(" (cm) : ");
    Serial.println(test_times[i] / US_TO_CM);
    delay(50);
  }
  unsigned long test_median = sonar.ping_median(5);
  unsigned int test_distance = test_median / US_TO_CM;
  Serial.print("[DEBUG] Temps médian initial (µs) : ");
  Serial.println(test_median);
  Serial.print("[DEBUG] Distance initiale médiane (cm) : ");
  Serial.print(test_distance);
  Serial.println(" cm");

  // Initialisation LoRa
  Serial.println("[DEBUG] Initialisation LoRa...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("[DEBUG] Échec initialisation LoRa");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Échec LoRa");
    display.display();
    while (1);
  }
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setTxPower(LORA_PWR);
  LoRa.setPreambleLength(LORA_PREAMBLE);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  LoRa.enableCrc();
  Serial.println("[DEBUG] LoRa initialisé avec succès");

  // Mesure de la distance
  Serial.println("[DEBUG] Mesure de la distance...");
  unsigned long raw_times[5];
  for (int i = 0; i < 5; i++) {
    raw_times[i] = sonar.ping();
    Serial.print("[DEBUG] Mesure ");
    Serial.print(i + 1);
    Serial.print(" (µs) : ");
    Serial.println(raw_times[i]);
    Serial.print("[DEBUG] Distance calculée ");
    Serial.print(i + 1);
    Serial.print(" (cm) : ");
    Serial.println(raw_times[i] / US_TO_CM);
    delay(50);
  }
  unsigned long median_time = sonar.ping_median(5);
  unsigned int distance = median_time / US_TO_CM;
  Serial.print("[DEBUG] Temps médian (µs) : ");
  Serial.println(median_time);
  if (median_time == 0 || distance < MIN_DISTANCE || distance > MAX_DISTANCE) {
    Serial.println("[DEBUG] Erreur de mesure, distance définie à MAX_DISTANCE");
    distance = MAX_DISTANCE;
  }
  Serial.print("[DEBUG] Distance mesurée médiane (cm) : ");
  Serial.print(distance);
  Serial.println(" cm");

  // Affichage sur l'OLED
  Serial.println("[DEBUG] Mise à jour de l'OLED...");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LORA SENDER");
  display.setCursor(0, 20);
  display.print("Distance: ");
  display.print(distance);
  display.print(" cm");
  display.display();

  // Affichage dans le moniteur série
  Serial.print("[DEBUG] Affichage série : Distance = ");
  Serial.print(distance);
  Serial.println(" cm");

  // Envoi via LoRa (format JSON pour OpenMQTTGateway)
  Serial.println("[DEBUG] Envoi des données via LoRa...");
  LoRa.beginPacket();
  String payload = "{\"sensor\":\"bassin\",\"distance\":" + String(distance) + "}";
  LoRa.print(payload);
  LoRa.endPacket();
  Serial.print("[DEBUG] Données envoyées : ");
  Serial.println(payload);

  // Mode veille profonde
  Serial.println("[DEBUG] Entrée en mode veille profonde...");
  esp_sleep_enable_timer_wakeup(SEND_INTERVAL * 1000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Non utilisé en mode production (deep sleep)
}