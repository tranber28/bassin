Capteur de Niveau d'Eau avec LoRa et LilyGO T3 V1.6.1
Ce projet utilise une carte LilyGO T3 V1.6.1 avec un capteur à ultrasons AJ-SR04M pour mesurer le niveau d'eau dans un bassin et transmettre les données via LoRa à un gateway (par exemple, OpenMQTTGateway). Les mesures sont affichées sur un écran OLED et envoyées à une instance Home Assistant via MQTT.
Fonctionnalités

Mesure du niveau d'eau avec un capteur à ultrasons AJ-SR04M.
Transmission des données via LoRa (868 MHz) vers un OpenMQTTGateway.
Affichage de la distance sur un écran OLED 128x64.
Utilisation du mode veille profonde pour économiser la batterie (intervalle de 4 heures par défaut).
Intégration avec Home Assistant via MQTT.

Matériel

Carte : LilyGO T3 V1.6.1 (ESP32 avec module LoRa).
Capteur : AJ-SR04M (capteur à ultrasons étanche).
Écran OLED : SSD1306 128x64 (I2C, broches SDA 21, SCL 22).
Alimentation : USB ou batterie (veille profonde pour faible consommation).

Configuration des broches

AJ-SR04M : Trig (12), Echo (13).
OLED : SDA (21), SCL (22), RST (4).
LoRa : SCK (5), MISO (19), MOSI (27), SS (18), RST (23), DIO0 (26).

Prérequis logiciels

PlatformIO pour compiler et téléverser le code.
Bibliothèques :
LoRa par Sandeep Mistry (v0.8.0)
Adafruit SSD1306 (v2.5.7)
Adafruit GFX Library (v1.11.9)
NewPing par Tim Eckel (v1.9.7)



Installez les dépendances via platformio.ini.
Installation

Clonez ce dépôt :git clone https://github.com/tranber28/bassin.git


Ouvrez le projet dans VS Code avec l’extension PlatformIO.
Configurez platformio.ini :[env:ttgo-lora32-v1]
platform = espressif32
board = ttgo-lora32-v1
framework = arduino
monitor_speed = 115200
upload_port = COM8
lib_deps = 
    sandeepmistry/LoRa@^0.8.0
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.9
    teckel12/NewPing@^1.9.7


Connectez la carte LilyGO à votre ordinateur (port COM8).
Compilez et téléversez le code :pio run -t upload


Ouvrez le Moniteur Série pour vérifier :pio device monitor



Calibration

Le capteur est calibré pour une vitesse du son de 340 m/s (15°C), avec US_TO_CM = 61.63 µs/cm.
Testé avec précision à 20 cm (exact) et 50 cm (49 cm mesuré).
Ajustez TEMPERATURE dans src/main.cpp si la température ambiante diffère :const float TEMPERATURE = 15.0; // Ajustez à la température réelle (°C)



Intégration avec Home Assistant
Ajoutez ceci à votre configuration.yaml :
mqtt:
  sensor:
      - name: "Niveau_Bassin"
        state_topic: "home/OMG_ESP32_LORA/LORAtoMQTT"
        value_template: "{{ value_json.distance }}"
        unit_of_measurement: 'cm'
        device_class: distance
        unique_id: "bassin_distance"   


Redémarrez Home Assistant pour détecter le capteur.
Ajoutez sensor.niveau_bassin à votre tableau de bord.

Utilisation

Placez le capteur AJ-SR04M au-dessus de la surface de l’eau (perpendiculaire, eau calme).
Le capteur mesure la distance toutes les 4 heures (configurable via SEND_INTERVAL).
Les données sont envoyées via LoRa à OpenMQTTGateway, puis relayées à Home Assistant via MQTT.
L’écran OLED affiche la distance mesurée.

Dépannage

Distance incorrecte : Vérifiez l’alignement du capteur, la surface de l’eau, ou ajustez TEMPERATURE.
Pas de données MQTT : Vérifiez la configuration d’OpenMQTTGateway (868 MHz, SF=7, BW=125 kHz, Sync Word=0x12).
Moniteur Série : Utilisez pio device monitor à 115200 bauds pour déboguer.

Licence
Licence MIT 
