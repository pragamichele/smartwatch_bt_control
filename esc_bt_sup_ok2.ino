/*
  This Arduino code is provided "as is", without any warranty of any kind.
  It may be used, modified, and distributed exclusively for non-commercial purposes.
  Use for commercial purposes is expressly prohibited without prior written consent from the author.
  
  Author: Praga Michele

  License: Non-commercial use only
*/
#include <Arduino.h>
#include <BluetoothA2DPSink.h>
#include <ESP32Servo.h>
#include <Ticker.h>

// Dichiarazioni delle variabili globali
Servo escServo;
Ticker safetyTimer;

const int escPin = 18;            // Pin al quale è collegato l'ESC
const int safetySwitchPin = 4;    // Pin collegato all'interruttore di sicurezza (normalmente chiuso)

// Variabili per gestire lo stato dell'interruttore
bool isSafetySwitchOpen = false;

// Crea una classe che eredita da A2DPVolumeControl
class MyVolumeControl : public A2DPVolumeControl {
  public:
    void set_volume(uint8_t volume) override {
      if (isSafetySwitchOpen) {
        // Se l'interruttore di sicurezza è aperto, non cambia la velocità del motore
        escServo.writeMicroseconds(1000);  // Imposta la velocità a zero
        Serial.println("Interruttore di sicurezza aperto. Velocità del motore impostata a 0.");
        return;
      }

      // Reset del timer ogni volta che riceviamo un nuovo volume
      safetyTimer.detach();  // Stoppa il timer se è già attivo

      // Callback per gestire il cambio di volume
      Serial.printf("Volume ricevuto: %d\n", volume);

      // Converti il livello del volume in un segnale PWM per l'ESC
      int pwmValue = map(volume, 0, 127, 1000, 2000);  // Adatta la mappatura secondo le specifiche del tuo ESC
      escServo.writeMicroseconds(pwmValue);

      Serial.printf("Volume: %d -> PWM: %d\n", volume, pwmValue);

      // Riavvia il timer di sicurezza
      safetyTimer.attach(300, []() {
        if (!isSafetySwitchOpen) {
          escServo.writeMicroseconds(1000);  // Imposta il minimo segnale PWM all'ESC
          Serial.println("Connessione persa. Velocità del motore impostata a 0.");
        }
      });
    }
};

BluetoothA2DPSink a2dp_sink;
MyVolumeControl myVolumeControl;  // Istanza della classe MyVolumeControl

void setup() {
  Serial.begin(115200);
  Serial.println("Inizializzazione del dispositivo A2DP Sink...");

  // Inizializza il dispositivo A2DP con il controllo del volume
  a2dp_sink.set_volume_control(&myVolumeControl);
  a2dp_sink.start("Motor_sup");

  // Configura il pin dell'ESC
  escServo.attach(escPin);
  escServo.writeMicroseconds(1000);  // Imposta il minimo segnale PWM all'ESC

  // Configura il pin dell'interruttore di sicurezza
  pinMode(safetySwitchPin, INPUT_PULLUP);

  Serial.println("A2DP Sink avviato. Nome del dispositivo: ESP32_A2DP_Sink");

  // Avvia il timer di sicurezza
  safetyTimer.attach(300, []() {
    if (!isSafetySwitchOpen) {
      escServo.writeMicroseconds(1000);  // Imposta il minimo segnale PWM all'ESC
      Serial.println("Connessione persa. Velocità del motore impostata a 0.");
    }
  });
}

void loop() {
  // Leggi lo stato dell'interruttore di sicurezza
  isSafetySwitchOpen = (digitalRead(safetySwitchPin) == LOW);

  // Se l'interruttore di sicurezza è aperto, imposta la velocità a zero
  if (isSafetySwitchOpen) {
    escServo.writeMicroseconds(1000);  // Imposta la velocità a zero
    Serial.println("Interruttore di sicurezza aperto. Velocità del motore impostata a 0.");
  }
  
  // Aggiungi un breve delay per evitare il polling continuo
  delay(10);
}
