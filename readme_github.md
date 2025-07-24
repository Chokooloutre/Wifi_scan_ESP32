# 📡 Scanner WiFi ESP32 v2.1

Un scanner WiFi intelligent et économe en énergie basé sur ESP32 WROOM-32D avec alertes sonores, détection de réseaux cachés et sauvegarde automatique.

[![ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Arduino](https://img.shields.io/badge/Framework-Arduino-00979D.svg)](https://www.arduino.cc/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## ✨ Fonctionnalités

### 🔍 **Scan Intelligent**
- **Détection automatique** des réseaux WiFi toutes les 2 minutes
- **Anti-doublons** avec historique complet des réseaux
- **Réseaux cachés** détectés automatiquement
- **Nettoyage automatique** des anciens réseaux (20 min d'inactivité)

### 🔊 **Alertes Sonores**
- **Buzzer passif ** pour nouvelles détections
- **Alertes uniquement** pour nouveaux réseaux ouverts
- **Mélodie distinctive**
- **Affichage visuel 15 secondes** sur écran OLED

### 🔋 **Économie d'Énergie**
- **Light sleep** intelligent entre les scans
- **Luminosité adaptive** de l'écran OLED
- **Autonomie étendue** : 15-20 heures avec batterie 2000mAh
- **Préservation des données** en mémoire

### 💾 **Sauvegarde Optimisée**
- **Fichiers séparés** pour réseaux ouverts/sécurisés
- **Sauvegarde intelligente** (seulement les nouveautés)
- **Base de données complète** toutes les 10 minutes
- **Format CSV** compatible Excel/LibreOffice

## 📦 Matériel Requis

### Composants Obligatoires
| Composant | Quantité | Prix approx. |
|-----------|----------|--------------|
| ESP32 WROOM-32D | 1 | ~10€ |
| Écran OLED SSD1306 128x64 (I2C) | 1 | ~6€ |
| Module carte micro SD | 1 | ~4€ |
| Buzzer passif 5V | 1 | ~2€ |
| Carte micro SD (8-32GB) | 1 | ~5€ |
| Breadboard + câbles Dupont | 1 | ~5€ |

### Composants Optionnels
- Batterie Li-Po 3.7V (1000-2000mAh)
- Module de charge TP4056
- Boîtier de protection

## 🔌 Schéma de Câblage

```
ESP32 WROOM-32D
    ├── OLED SSD1306 (I2C)
    │   ├── 3.3V → VCC
    │   ├── GND  → GND
    │   ├── GPIO21 → SDA
    │   └── GPIO22 → SCL
    ├── Module SD (SPI)
    │   ├── 3.3V → VCC
    │   ├── GND  → GND
    │   ├── GPIO5  → CS
    │   ├── GPIO23 → MOSI
    │   ├── GPIO19 → MISO
    │   └── GPIO18 → SCK
    └── Buzzer
        ├── GPIO25 → + (signal)
        └── GND    → - (masse)
```

## 🚀 Installation

### 1. Prérequis Arduino IDE
```bash
# URLs supplémentaires de cartes
https://dl.espressif.com/dl/package_esp32_index.json

# Bibliothèques requises
Adafruit GFX Library (v1.11.9+)
Adafruit SSD1306 (v2.5.7+)
```

### 2. Configuration ESP32
- **Carte** : "ESP32 Dev Module"
- **Upload Speed** : 921600
- **CPU Frequency** : 240MHz (WiFi/BT)
- **Flash Mode** : QIO
- **Flash Size** : 4MB

### 3. Upload du Code
1. Cloner ce repository
2. Ouvrir `wifi_scanner_v2.1.ino` dans Arduino IDE
3. Sélectionner le bon port COM
4. Téléverser le code
5. Vérifier le fonctionnement (3 bips au démarrage)

## 📱 Utilisation

### Démarrage
1. **Alimentation** : Connecter USB ou batterie
2. **Test automatique** : 3 bips courts (buzzer OK)
3. **Initialisation** : Écran affiche "Scanner prêt!"
4. **Premier scan** : Détection de tous les réseaux (nouveaux)

### Fonctionnement Normal
```
🔍 Scan (10s) → 📊 Résultats → 🔊 Alerte (si nouveau ouvert) → 
📱 Décompte (10s) → 🛌 Veille (110s) → 🔁 Répétition
```

### Interface Console Série
```bash
=== Scanner WiFi ESP32 v2.1 ===
🆕 NOUVEAU: FreeWiFi-Cafe      aa:bb:cc:dd:ee:ff  -45dBm Ch: 6 Ouvert
👁️ NOUVEAU: [HIDDEN]           11:22:33:44:55:77  -65dBm Ch:11 WPA2
🔊 ALERTE SONORE: Nouveau réseau ouvert détecté! FreeWiFi-Cafe

📊 STATISTIQUES:
   • Nouveaux réseaux: 2 (dont 1 ouverts)
   • Total connus: 47 (12 ouverts, 35 sécurisés, 8 cachés)
```

### Interface Écran OLED
```
RESEAUX GRATUITS:
----------------
1.FreeWiFi-Cafe
  -45dBm Ch:6 x15
2.[HIDDEN]
  -62dBm Ch:11 x3H
3.GUEST_NETWORK
  -51dBm Ch:1 x7
H=Cache
```

## 📁 Fichiers de Sortie

### Structure des Fichiers CSV

**Réseaux Ouverts** (`wifi_open_update_[timestamp].csv`)
```csv
SSID,BSSID,Signal(dBm),Canal,Sécurité,Type,Première_Detection,Nb_Vues
"FreeWiFi-Cafe",aa:bb:cc:dd:ee:ff,-45,6,Ouvert,Normal,1642550400,3
"[HIDDEN]",11:22:33:44:55:66,-62,11,Ouvert,Caché,1642550460,1
```

**Réseaux Sécurisés** (`wifi_secured_update_[timestamp].csv`)
```csv
SSID,BSSID,Signal(dBm),Canal,Sécurité,Type,Première_Detection,Nb_Vues
"MonWiFi_Home",aa:bb:cc:dd:ee:11,-38,1,WPA2,Normal,1642550400,5
"[HIDDEN]",11:22:33:44:55:88,-71,6,WPA2,Caché,1642550420,2
```

**Base Complète** (`wifi_database_[timestamp].csv`)
```csv
SSID,BSSID,Signal(dBm),Canal,Sécurité,Type_Reseau,Type_Visibilite,Première_Detection,Dernière_Vue,Nb_Total_Vues
"FreeWiFi-Cafe",aa:bb:cc:dd:ee:ff,-45,6,Ouvert,Ouvert,Visible,1642550400,1642551200,15
"[HIDDEN]",11:22:33:44:55:66,-62,11,Ouvert,Ouvert,Caché,1642550460,1642551180,8
```

## ⚙️ Configuration Avancée

### Modifier l'Intervalle de Scan
```cpp
// Dans displayCountdownWithSleep()
displayCountdownWithSleep(120);  // 2 minutes (défaut)
displayCountdownWithSleep(300);  // 5 minutes
displayCountdownWithSleep(60);   // 1 minute
```

### Ajuster la Durée d'Alerte
```cpp
// Dans playNewOpenNetworkAlert()
delay(15000); // 15 secondes (défaut)
delay(30000); // 30 secondes
delay(5000);  // 5 secondes
```

### Désactiver le Buzzer
```cpp
// Commenter cette ligne dans la fonction loop()
// playNewOpenNetworkAlert(ssid);
```

## 🔧 Dépannage

### Problèmes Courants

**Écran OLED ne s'allume pas**
- ✅ Vérifier câblage I2C (GPIO21/22)
- ✅ Tester adresse I2C (0x3C ou 0x3D)
- ✅ Alimenter en 3.3V (pas 5V)

**Carte SD non détectée**
- ✅ Formater en FAT32
- ✅ Vérifier connexions SPI
- ✅ Tester avec une autre carte SD

**Buzzer silencieux**
- ✅ Vérifier GPIO25 + GND
- ✅ Tester buzzer actif vs passif
- ✅ Ajouter résistance 220Ω si nécessaire

**Mémoire insuffisante**
```cpp
// Réduire le temps de rétention
const unsigned long MAX_AGE = 600000; // 10 min au lieu de 20
```

### Tests de Diagnostic
```cpp
// Surveillance mémoire
Serial.printf("Mémoire libre: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Réseaux stockés: %d\n", knownNetworks.size());

// Test I2C
Wire.begin();
Wire.beginTransmission(0x3C);
if (Wire.endTransmission() == 0) {
  Serial.println("OLED détecté à 0x3C");
}
```

## 📊 Performances

### Autonomie Mesurée
| Configuration | Autonomie | Scans/heure |
|---------------|-----------|-------------|
| Batterie 1000mAh + Mode Eco | ~8-12h | 30 |
| Batterie 2000mAh + Mode Eco | ~15-20h | 30 |
| Alimentation USB | ∞ | 30 |

### Consommation
- **Scan actif** : ~120mA
- **Mode économie** : ~20mA
- **Alerte buzzer** : +20mA (2-3s)
- **Veille légère** : ~15mA

## 🎯 Cas d'Usage

### 🚶 Wardriving Urbain
- Détection automatique hotspots gratuits
- Alerte sonore sans regarder l'écran
- Cartographie des réseaux publics

### 🏢 Audit Sécurité
- Identification réseaux cachés suspects
- Monitoring des changements de sécurité
- Base de données pour analyse forensique

### 🏠 Surveillance Réseau
- Détection nouveaux équipements
- Alertes intrusion réseau
- Historique évolution environnement WiFi

## 🤝 Contribution

Les contributions sont les bienvenues ! 

1. **Fork** le projet
2. **Créer** une branche (`git checkout -b feature/nouvelle-fonctionnalite`)
3. **Commit** les changements (`git commit -am 'Ajout nouvelle fonctionnalité'`)
4. **Push** vers la branche (`git push origin feature/nouvelle-fonctionnalite`)
5. **Ouvrir** une Pull Request

### Idées d'Améliorations
- [ ] Interface web pour configuration à distance
- [ ] Support GPS pour géolocalisation
- [ ] Export Google Earth (KML)
- [ ] Interface boutons pour navigation manuelle
- [ ] Détection d'anomalies de sécurité
- [ ] Support LoRaWAN pour transmission longue distance

## 📄 Licence

Ce projet est sous licence MIT - voir le fichier [LICENSE](LICENSE) pour plus de détails.

## 👥 Auteurs

- **Développeur Principal** - *Conception et développement initial*

## 🙏 Remerciements

- **Espressif Systems** pour l'ESP32
- **Adafruit** pour les bibliothèques OLED
- **Communauté Arduino** pour les ressources et l'inspiration

---

⭐ **N'oubliez pas de mettre une étoile si ce projet vous a été utile !**

## 📞 Support

Pour toute question ou problème :
- 📝 **Issues** : Ouvrir une issue GitHub
- 📧 **Email** : [votre-email]
- 💬 **Discord** : [votre-discord]

---

*Scanner WiFi ESP32 v2.1 - Détectez, Analysez, Cartographiez* 📡
