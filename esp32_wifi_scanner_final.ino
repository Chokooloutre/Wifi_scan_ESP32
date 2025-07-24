/*
 * Scanner WiFi ESP32 v2.1 - Buzzer passif avec mélodies
 * 
 * Fonctionnalités :
 * - Scan toutes les 2 minutes avec mode économie d'énergie
 * - Alertes sonores pour nouveaux réseaux ouverts (15 secondes d'affichage)
 * - Détection des réseaux cachés
 * - Anti-doublons intelligent avec historique
 * - Sauvegarde optimisée sur carte SD
 * - Écran OLED avec statistiques temps réel
 * 
 * Matériel requis :
 * - ESP32 WROOM-32D (ou DevKit)
 * - Écran OLED SSD1306 128x64 (I2C)
 * - Module carte SD
 * - Buzzer passif
 * - Carte micro SD
 */

#include "WiFi.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <map>
#include <set>

// Configuration des pins pour carte SD
#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

// Configuration écran OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Configuration buzzer pour alertes sonores
#define BUZZER_PIN 25

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Structure pour stocker les informations réseau avec historique
struct NetworkInfo {
  String ssid;
  String bssid;
  int rssi;
  int channel;
  bool isOpen;
  bool isHidden;
  String security;
  unsigned long firstSeen;
  unsigned long lastSeen;
  int seenCount;
};

// Stockage des réseaux déjà vus (clé = BSSID, valeur = NetworkInfo)
std::map<String, NetworkInfo> knownNetworks;
std::set<String> currentScanBSSIDs;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Scanner WiFi ESP32 v2.1 ===");
  Serial.println("Alertes sonores + Réseaux cachés + Économie d'énergie");
  Serial.println("Démarrage...");
  
  // Initialisation du buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Test buzzer au démarrage
  playStartupSound();
  
  // Initialisation écran OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Erreur: écran OLED non détecté");
  } else {
    Serial.println("Écran OLED initialisé");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("WiFi Scanner v2.1");
    display.println("Alertes + Caches");
    display.println("Eco energie");
    display.println("Initialisation...");
    display.display();
  }
  
  // Initialisation WiFi en mode station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Initialisation carte SD
  if (!SD.begin(SD_CS)) {
    Serial.println("Carte SD non détectée, sauvegarde sur Serial uniquement");
    displayMessage("SD: Non detectee");
  } else {
    Serial.println("Carte SD initialisée");
    displayMessage("SD: OK");
  }
  
  Serial.println("Scanner prêt !");
  Serial.println("🔊 Alertes sonores activées pour nouveaux réseaux ouverts");
  Serial.println("👁️  Détection des réseaux cachés activée");
  Serial.println("🔋 Mode économie d'énergie activé (scan toutes les 2 minutes)");
  displayMessage("Scanner pret!");
  delay(2000);
}

void loop() {
  Serial.println("\n--- Début du scan (incluant réseaux cachés) ---");
  displayMessage("Scan + caches...");
  
  // Vider le set des BSSID du scan actuel
  currentScanBSSIDs.clear();
  
  // Scan des réseaux WiFi (incluant les réseaux cachés)
  int networkCount = WiFi.scanNetworks(false, true, true); // async=false, showHidden=true, passive=true
  unsigned long currentTime = millis();
  
  if (networkCount == 0) {
    Serial.println("Aucun réseau détecté");
    displayMessage("Aucun reseau");
  } else {
    Serial.printf("%d réseaux détectés dans ce scan (cachés inclus)\n", networkCount);
    
    // Variables pour compter nouveaux réseaux et mises à jour
    int newNetworks = 0;
    int newOpenNetworks = 0;
    int updatedNetworks = 0;
    int totalOpenNetworks = 0;
    int totalSecuredNetworks = 0;
    int totalHiddenNetworks = 0;
    
    // Analyse de chaque réseau détecté
    for (int i = 0; i < networkCount; i++) {
      String bssid = WiFi.BSSIDstr(i);
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      int channel = WiFi.channel(i);
      
      // Détection des réseaux cachés (SSID vide)
      bool isHidden = (ssid.length() == 0);
      if (isHidden) {
        ssid = "[HIDDEN]";
        totalHiddenNetworks++;
      }
      
      // Marquer ce BSSID comme vu dans ce scan
      currentScanBSSIDs.insert(bssid);
      
      // Détermination du type de sécurité
      wifi_auth_mode_t authMode = WiFi.encryptionType(i);
      bool isOpen = (authMode == WIFI_AUTH_OPEN);
      String security;
      
      switch (authMode) {
        case WIFI_AUTH_OPEN:
          security = "Ouvert";
          break;
        case WIFI_AUTH_WEP:
          security = "WEP";
          break;
        case WIFI_AUTH_WPA_PSK:
          security = "WPA";
          break;
        case WIFI_AUTH_WPA2_PSK:
          security = "WPA2";
          break;
        case WIFI_AUTH_WPA_WPA2_PSK:
          security = "WPA/WPA2";
          break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
          security = "WPA2-Enterprise";
          break;
        case WIFI_AUTH_WPA3_PSK:
          security = "WPA3";
          break;
        default:
          security = "Inconnu";
          break;
      }
      
      // Vérifier si c'est un nouveau réseau ou une mise à jour
      bool isNewNetwork = false;
      bool needsUpdate = false;
      
      if (knownNetworks.find(bssid) == knownNetworks.end()) {
        // Nouveau réseau
        isNewNetwork = true;
        newNetworks++;
        if (isOpen) newOpenNetworks++;
        
        NetworkInfo newNet;
        newNet.ssid = ssid;
        newNet.bssid = bssid;
        newNet.rssi = rssi;
        newNet.channel = channel;
        newNet.isOpen = isOpen;
        newNet.isHidden = isHidden;
        newNet.security = security;
        newNet.firstSeen = currentTime;
        newNet.lastSeen = currentTime;
        newNet.seenCount = 1;
        
        knownNetworks[bssid] = newNet;
        
        String networkType = isHidden ? "👁️ " : "🆕 ";
        Serial.printf("%sNOUVEAU: %-20s %s %3ddBm Ch:%2d %s\n", 
                      networkType.c_str(), ssid.c_str(), bssid.c_str(), rssi, channel, security.c_str());
        
        // 🔊 ALERTE SONORE pour nouveau réseau ouvert uniquement
        if (isOpen) {
          playNewOpenNetworkAlert(ssid);
        }
        
      } else {
        // Réseau déjà connu - vérifier les changements
        NetworkInfo &existingNet = knownNetworks[bssid];
        
        // Mise à jour des compteurs
        existingNet.lastSeen = currentTime;
        existingNet.seenCount++;
        
        // Vérifier si des informations importantes ont changé
        if (existingNet.ssid != ssid || 
            abs(existingNet.rssi - rssi) > 10 || // Changement significatif de signal
            existingNet.channel != channel ||
            existingNet.security != security) {
          
          needsUpdate = true;
          updatedNetworks++;
          
          Serial.printf("🔄 MODIFIÉ: %-20s %s %3ddBm->%3ddBm Ch:%2d %s\n", 
                        ssid.c_str(), bssid.c_str(), existingNet.rssi, rssi, channel, security.c_str());
          
          // Mettre à jour les informations
          existingNet.ssid = ssid;
          existingNet.rssi = rssi;
          existingNet.channel = channel;
          existingNet.isOpen = isOpen;
          existingNet.isHidden = isHidden;
          existingNet.security = security;
        }
      }
    }
    
    // Compter tous les réseaux connus (pas seulement ceux du scan actuel)
    int knownHiddenNetworks = 0;
    for (const auto& pair : knownNetworks) {
      if (pair.second.isOpen) {
        totalOpenNetworks++;
      } else {
        totalSecuredNetworks++;
      }
      if (pair.second.isHidden) {
        knownHiddenNetworks++;
      }
    }
    
    Serial.printf("\n📊 STATISTIQUES:\n");
    Serial.printf("   • Nouveaux réseaux: %d (dont %d ouverts)\n", newNetworks, newOpenNetworks);
    Serial.printf("   • Réseaux modifiés: %d\n", updatedNetworks);
    Serial.printf("   • Total connus: %d (%d ouverts, %d sécurisés, %d cachés)\n", 
                  knownNetworks.size(), totalOpenNetworks, totalSecuredNetworks, knownHiddenNetworks);
    
    // Affichage sur écran OLED
    displayScanResults(knownNetworks.size(), totalOpenNetworks, totalSecuredNetworks, newNetworks, knownHiddenNetworks);
    
    // Sauvegarde seulement si nouveaux réseaux ou modifications importantes
    if (newNetworks > 0 || updatedNetworks > 0) {
      saveNewNetworksToSD(newNetworks, updatedNetworks);
      Serial.println("💾 Fichiers mis à jour avec les nouveaux/modifiés réseaux");
    } else {
      Serial.println("ℹ️  Aucune nouveauté - pas de sauvegarde");
    }
    
    // Affichage des réseaux ouverts uniques
    printUniqueOpenNetworks();
    
    // Nettoyage périodique (toutes les 20 minutes)
    cleanOldNetworks(currentTime);
  }
  
  // Nettoyage du scan
  WiFi.scanDelete();
  
  // Attente avant le prochain scan (2 minutes = 120 secondes)
  Serial.println("\nProchain scan dans 120 secondes (avec économie d'énergie)...");
  displayCountdownWithSleep(120);
}

void saveNewNetworksToSD(int newCount, int updatedCount) {
  if (!SD.begin(SD_CS)) return;
  
  String timestamp = String(millis());
  
  // Créer les fichiers CSV pour nouveaux réseaux uniquement
  String csvOpenNetworks = "SSID,BSSID,Signal(dBm),Canal,Sécurité,Type,Première_Detection,Nb_Vues\n";
  String csvSecuredNetworks = "SSID,BSSID,Signal(dBm),Canal,Sécurité,Type,Première_Detection,Nb_Vues\n";
  
  int newOpenCount = 0;
  int newSecuredCount = 0;
  
  // Parcourir tous les réseaux connus
  for (const auto& pair : knownNetworks) {
    const NetworkInfo& net = pair.second;
    
    // Sauvegarder seulement les réseaux nouveaux ou récemment mis à jour
    bool shouldSave = (currentScanBSSIDs.find(net.bssid) != currentScanBSSIDs.end());
    
    if (shouldSave) {
      String networkType = net.isHidden ? "Caché" : "Normal";
      
      String csvLine = "\"" + net.ssid + "\"," + 
                      net.bssid + "," + 
                      String(net.rssi) + "," + 
                      String(net.channel) + "," + 
                      net.security + "," +
                      networkType + "," +
                      String(net.firstSeen) + "," +
                      String(net.seenCount) + "\n";
      
      if (net.isOpen) {
        csvOpenNetworks += csvLine;
        newOpenCount++;
      } else {
        csvSecuredNetworks += csvLine;
        newSecuredCount++;
      }
    }
  }
  
  // Sauvegarder les fichiers seulement s'il y a du contenu
  if (newOpenCount > 0) {
    String openFilename = "/wifi_open_update_" + timestamp + ".csv";
    File openFile = SD.open(openFilename, FILE_WRITE);
    if (openFile) {
      openFile.print(csvOpenNetworks);
      openFile.close();
      Serial.printf("✅ %d réseaux ouverts sauvegardés: %s\n", newOpenCount, openFilename.c_str());
    }
  }
  
  if (newSecuredCount > 0) {
    String securedFilename = "/wifi_secured_update_" + timestamp + ".csv";
    File securedFile = SD.open(securedFilename, FILE_WRITE);
    if (securedFile) {
      securedFile.print(csvSecuredNetworks);
      securedFile.close();
      Serial.printf("✅ %d réseaux sécurisés sauvegardés: %s\n", newSecuredCount, securedFilename.c_str());
    }
  }
  
  // Sauvegarder un fichier de résumé complet périodiquement (toutes les 10 minutes)
  static unsigned long lastFullSave = 0;
  if (millis() - lastFullSave > 600000) { // 10 minutes
    saveFullNetworkDatabase(timestamp);
    lastFullSave = millis();
  }
}

void saveFullNetworkDatabase(String timestamp) {
  String fullDbFilename = "/wifi_database_" + timestamp + ".csv";
  File dbFile = SD.open(fullDbFilename, FILE_WRITE);
  
  if (dbFile) {
    dbFile.println("SSID,BSSID,Signal(dBm),Canal,Sécurité,Type_Reseau,Type_Visibilite,Première_Detection,Dernière_Vue,Nb_Total_Vues");
    
    for (const auto& pair : knownNetworks) {
      const NetworkInfo& net = pair.second;
      
      String networkType = net.isOpen ? "Ouvert" : "Sécurisé";
      String visibilityType = net.isHidden ? "Caché" : "Visible";
      
      dbFile.printf("\"%s\",%s,%d,%d,%s,%s,%s,%lu,%lu,%d\n",
                    net.ssid.c_str(),
                    net.bssid.c_str(),
                    net.rssi,
                    net.channel,
                    net.security.c_str(),
                    networkType.c_str(),
                    visibilityType.c_str(),
                    net.firstSeen,
                    net.lastSeen,
                    net.seenCount);
    }
    
    dbFile.close();
    Serial.printf("📋 Base de données complète sauvegardée: %s (%d réseaux)\n", 
                  fullDbFilename.c_str(), knownNetworks.size());
  }
}

// Nettoyage des anciens réseaux (pas vus depuis 20 minutes)
void cleanOldNetworks(unsigned long currentTime) {
  static unsigned long lastCleanup = 0;
  const unsigned long CLEANUP_INTERVAL = 1200000; // 20 minutes
  const unsigned long MAX_AGE = 1200000; // 20 minutes sans être vu
  
  if (currentTime - lastCleanup > CLEANUP_INTERVAL) {
    int removedCount = 0;
    
    auto it = knownNetworks.begin();
    while (it != knownNetworks.end()) {
      if (currentTime - it->second.lastSeen > MAX_AGE) {
        Serial.printf("🗑️  Suppression réseau ancien: %s (%s)\n", 
                      it->second.ssid.c_str(), it->second.bssid.c_str());
        it = knownNetworks.erase(it);
        removedCount++;
      } else {
        ++it;
      }
    }
    
    if (removedCount > 0) {
      Serial.printf("🧹 Nettoyage terminé: %d anciens réseaux supprimés\n", removedCount);
    }
    
    lastCleanup = currentTime;
  }
}

// 🔊 Fonctions pour les alertes sonores avec mélodies
// Définition des notes musicales (fréquences en Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880

void playStartupSound() {
  Serial.println("🔊 Buzzer passif initialisé - Mélodie de démarrage");
  
  // Mélodie de démarrage : "Do-Mi-Sol-Do" (accord majeur ascendant)
  int startupMelody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
  int startupDuration[] = {150, 150, 150, 300};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, startupMelody[i], startupDuration[i]);
    delay(startupDuration[i] + 50);
    noTone(BUZZER_PIN);
  }
  
  delay(200);
  Serial.println("🎵 Mélodie de démarrage terminée");
}

void playNewOpenNetworkAlert(String ssid) {
  Serial.printf("🔊 ALERTE MÉLODIQUE: Nouveau réseau ouvert détecté! %s\n", ssid.c_str());
  
  // Mélodie d'alerte : Séquence "Attention" + "Victoire"
  // Partie 1: Signal d'attention (notes rapides alternées)
  int alertMelody1[] = {NOTE_G5, NOTE_E5, NOTE_G5, NOTE_E5};
  int alertDuration1[] = {100, 100, 100, 100};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, alertMelody1[i], alertDuration1[i]);
    delay(alertDuration1[i] + 30);
    noTone(BUZZER_PIN);
  }
  
  delay(200);
  
  // Partie 2: Mélodie de confirmation (montée joyeuse)
  int alertMelody2[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5};
  int alertDuration2[] = {150, 150, 150, 200, 300};
  
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, alertMelody2[i], alertDuration2[i]);
    delay(alertDuration2[i] + 50);
    noTone(BUZZER_PIN);
  }
  
  // Affichage spécial sur écran pendant l'alerte
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("!!! ALERTE !!!");
  display.println("===============");
  display.println("Nouveau reseau");
  display.println("OUVERT detecte:");
  display.println();
  
  String displaySSID = ssid;
  if (displaySSID.length() > 18) {
    displaySSID = displaySSID.substring(0, 18) + "...";
  }
  display.setTextSize(1);
  display.println(displaySSID);
  display.println();
  display.println("♪ Melodie jouee ♪");
  
  display.display();
  delay(15000); // Afficher l'alerte pendant 15 secondes
}

// Fonction bonus : mélodie d'erreur (optionnel)
void playErrorSound() {
  Serial.println("🔊 Mélodie d'erreur");
  
  // Séquence descendante pour indiquer une erreur
  int errorMelody[] = {NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4};
  int errorDuration[] = {200, 200, 200, 200, 400};
  
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, errorMelody[i], errorDuration[i]);
    delay(errorDuration[i] + 30);
    noTone(BUZZER_PIN);
  }
}

// Fonction bonus : mélodie de fin de scan (optionnel)
void playScanCompleteSound() {
  // Mélodie courte et discrète pour fin de scan
  int completeMelody[] = {NOTE_C5, NOTE_G4};
  int completeDuration[] = {100, 150};
  
  for (int i = 0; i < 2; i++) {
    tone(BUZZER_PIN, completeMelody[i], completeDuration[i]);
    delay(completeDuration[i] + 20);
    noTone(BUZZER_PIN);
  }
}

// Fonctions pour l'écran OLED
void displayMessage(String message) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("WiFi Scanner v2.1");
  display.println("------------------");
  display.println(message);
  display.display();
}

void displayScanResults(int total, int open, int secured, int newNetworks, int hidden) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  
  display.println("=== SCAN RESULTS ===");
  display.println();
  display.printf("Total: %d reseaux\n", total);
  display.printf("Ouverts: %d\n", open);
  display.printf("Securises: %d\n", secured);
  display.printf("Caches: %d\n", hidden);
  
  if (newNetworks > 0) {
    display.printf("Nouveaux: %d\n", newNetworks);
  } else {
    display.println("Aucune nouveaute");
  }
  
  display.display();
}

void displayCountdownWithSleep(int seconds) {
  Serial.println("🔋 Mode économie d'énergie activé");
  
  // Affichage initial avec statistiques
  int openCount = 0, hiddenCount = 0;
  for (const auto& pair : knownNetworks) {
    if (pair.second.isOpen) openCount++;
    if (pair.second.isHidden) hiddenCount++;
  }
  
  // Afficher les 10 premières secondes normalement
  for (int i = seconds; i > seconds - 10 && i > 0; i--) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    
    display.printf("Total: %d reseaux\n", knownNetworks.size());
    display.printf("Gratuits: %d\n", openCount);
    display.printf("Caches: %d\n", hiddenCount);
    display.println("Prochain scan:");
    display.setTextSize(2);
    display.printf("  %02d", i);
    display.setTextSize(1);
    display.println();
    display.println("Mode eco actif");
    
    display.display();
    delay(1000);
  }
  
  // Mode économie d'énergie pour le reste du temps
  int remainingTime = seconds - 10;
  if (remainingTime > 0) {
    // Réduire la luminosité de l'écran (économie)
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(0x01); // Luminosité minimale
    
    // Affichage d'économie d'énergie
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("Mode ECONOMIE");
    display.println("=============");
    display.printf("Reseaux: %d\n", knownNetworks.size());
    display.printf("Gratuits: %d\n", openCount);
    display.println();
    display.println("En veille...");
    display.println("Reveil dans:");
    display.setTextSize(2);
    display.printf("  %02d", remainingTime);
    display.setTextSize(1);
    display.println();
    display.println("secondes");
    display.display();
    
    // Configuration du light sleep
    Serial.printf("🛌 Mise en veille légère pour %d secondes\n", remainingTime);
    
    // Light sleep par blocs de 10 secondes pour mise à jour affichage
    for (int sleepCycles = remainingTime; sleepCycles > 0; sleepCycles -= 10) {
      int sleepTime = (sleepCycles > 10) ? 10 : sleepCycles;
      
      // Configuration light sleep (garde RAM et WiFi config)
      esp_sleep_enable_timer_wakeup(sleepTime * 1000000ULL); // microsecondes
      
      // Mise en veille légère
      esp_light_sleep_start();
      
      // Mise à jour affichage après réveil
      if (sleepCycles > 10) {
        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(1);
        display.println("Mode ECONOMIE");
        display.println("=============");
        display.printf("Reseaux: %d\n", knownNetworks.size());
        display.printf("Gratuits: %d\n", openCount);
        display.println();
        display.println("En veille...");
        display.println("Reveil dans:");
        display.setTextSize(2);
        display.printf("  %02d", sleepCycles - 10);
        display.setTextSize(1);
        display.println();
        display.println("secondes");
        display.display();
      }
    }
    
    // Restaurer la luminosité normale
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(0x8F); // Luminosité normale
  }
  
  // Réveil - affichage final
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("REVEIL!");
  display.println("========");
  display.println("Preparation scan...");
  display.display();
  delay(1000);
  
  Serial.println("⚡ Réveil - préparation du prochain scan");
}

void printUniqueOpenNetworks() {
  Serial.println("\n=== RÉSEAUX OUVERTS UNIQUES (incluant cachés) ===");
  
  // Affichage sur écran des réseaux ouverts
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("RESEAUX GRATUITS:");
  display.println("----------------");
  
  int openNetworkIndex = 0;
  int maxDisplayLines = 3; // Réduire à 3 pour faire de la place pour l'info "caché"
  
  // Parcourir tous les réseaux connus et afficher seulement les ouverts
  for (const auto& pair : knownNetworks) {
    const NetworkInfo& net = pair.second;
    
    if (net.isOpen) {
      // Affichage console avec informations d'historique et statut caché
      String hiddenStatus = net.isHidden ? " [CACHÉ]" : "";
      Serial.printf("📶 %-25s | %s | %3ddBm | Ch:%2d | Vu %dx%s\n",
                    net.ssid.c_str(),
                    net.bssid.c_str(),
                    net.rssi,
                    net.channel,
                    net.seenCount,
                    hiddenStatus.c_str());
      
      // Affichage écran (limité aux 3 premiers)
      if (openNetworkIndex < maxDisplayLines) {
        String ssid = net.ssid;
        if (net.isHidden) {
          ssid = "[HIDDEN]";
        } else if (ssid.length() > 12) {
          ssid = ssid.substring(0, 12) + "...";
        }
        
        display.printf("%d.%s\n", openNetworkIndex + 1, ssid.c_str());
        String hiddenIndicator = net.isHidden ? "H" : "";
        display.printf("  %ddBm Ch:%d x%d%s\n", net.rssi, net.channel, net.seenCount, hiddenIndicator.c_str());
      }
      openNetworkIndex++;
    }
  }
  
  if (openNetworkIndex > maxDisplayLines) {
    display.printf("...+%d autres", openNetworkIndex - maxDisplayLines);
  }
  
  if (openNetworkIndex == 0) {
    display.println("Aucun reseau ouvert");
    Serial.println("Aucun réseau ouvert détecté");
  } else {
    display.println("H=Cache");
  }
  
  display.display();
  Serial.println("=============================================");
}

// Fonction pour scan unique (appel via Serial)
void scanOnce() {
  Serial.println("Scan unique en cours (incluant réseaux cachés)...");
  displayMessage("Scan unique+caches...");
  
  int networks = WiFi.scanNetworks(false, true, true); // Inclure réseaux cachés
  unsigned long currentTime = millis();
  
  String jsonOutput = "{\"scan_time\":\"" + String(currentTime) + "\",\"networks\":[";
  int openCount = 0;
  int hiddenCount = 0;
  int newInThisScan = 0;
  
  for (int i = 0; i < networks; i++) {
    if (i > 0) jsonOutput += ",";
    
    String bssid = WiFi.BSSIDstr(i);
    String ssid = WiFi.SSID(i);
    bool isHidden = (ssid.length() == 0);
    bool isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    
    if (isHidden) {
      ssid = "[HIDDEN]";
      hiddenCount++;
    }
    if (isOpen) openCount++;
    
    // Vérifier si c'est un nouveau réseau
    bool isNew = (knownNetworks.find(bssid) == knownNetworks.end());
    if (isNew) newInThisScan++;
    
    jsonOutput += "{";
    jsonOutput += "\"ssid\":\"" + ssid + "\",";
    jsonOutput += "\"bssid\":\"" + bssid + "\",";
    jsonOutput += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    jsonOutput += "\"channel\":" + String(WiFi.channel(i)) + ",";
    jsonOutput += "\"open\":" + String(isOpen ? "true" : "false") + ",";
    jsonOutput += "\"hidden\":" + String(isHidden ? "true" : "false") + ",";
    jsonOutput += "\"new\":" + String(isNew ? "true" : "false");
    jsonOutput += "}";
  }
  
  jsonOutput += "],\"summary\":{";
  jsonOutput += "\"total\":" + String(networks) + ",";
  jsonOutput += "\"open\":" + String(openCount) + ",";
  jsonOutput += "\"secured\":" + String(networks - openCount) + ",";
  jsonOutput += "\"hidden\":" + String(hiddenCount) + ",";
  jsonOutput += "\"new_in_scan\":" + String(newInThisScan) + ",";
  jsonOutput += "\"known_total\":" + String(knownNetworks.size());
  jsonOutput += "}}";
  
  Serial.println("JSON_OUTPUT:" + jsonOutput);
  
  // Affichage résumé sur écran
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("SCAN UNIQUE FINI");
  display.println("----------------");
  display.printf("Ce scan: %d\n", networks);
  display.printf("Nouveaux: %d\n", newInThisScan);
  display.printf("Gratuits: %d\n", openCount);
  display.printf("Caches: %d\n", hiddenCount);
  display.printf("Total connu: %d\n", knownNetworks.size());
  display.display();
  
  WiFi.scanDelete();
}

// Fonction pour afficher les statistiques
void printNetworkStats() {
  Serial.println("\n=== STATISTIQUES GLOBALES ===");
  Serial.printf("Total réseaux connus: %d\n", knownNetworks.size());
  
  int openCount = 0, securedCount = 0, hiddenCount = 0;
  int wpaCount = 0, wpa2Count = 0, wpa3Count = 0, wepCount = 0;
  int hiddenOpenCount = 0;
  
  for (const auto& pair : knownNetworks) {
    const NetworkInfo& net = pair.second;
    
    if (net.isHidden) {
      hiddenCount++;
      if (net.isOpen) hiddenOpenCount++;
    }
    
    if (net.isOpen) {
      openCount++;
    } else {
      securedCount++;
      if (net.security == "WPA") wpaCount++;
      else if (net.security == "WPA2") wpa2Count++;
      else if (net.security == "WPA3") wpa3Count++;
      else if (net.security == "WEP") wepCount++;
    }
  }
  
  Serial.printf("Réseaux ouverts: %d (%.1f%%)\n", openCount, (float)openCount/knownNetworks.size()*100);
  Serial.printf("  - dont cachés ouverts: %d\n", hiddenOpenCount);
  Serial.printf("Réseaux sécurisés: %d (%.1f%%)\n", securedCount, (float)securedCount/knownNetworks.size()*100);
  Serial.printf("  - WPA: %d\n", wpaCount);
  Serial.printf("  - WPA2: %d\n", wpa2Count);
  Serial.printf("  - WPA3: %d\n", wpa3Count);
  Serial.printf("  - WEP: %d\n", wepCount);
  Serial.printf("Réseaux cachés (total): %d (%.1f%%)\n", hiddenCount, (float)hiddenCount/knownNetworks.size()*100);
  Serial.println("=================================");
}