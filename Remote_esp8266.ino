#include <ESP8266WiFi.h>
#include <espnow.h>

#define BUTTON_PIN D5
#define BUZZER_PIN D8

// MAC address of Receiver
uint8_t peerAddress[] = {0x48, 0x3F, 0xDA, 0x03, 0x00, 0xCA}; //Reciever MAC Address

// --- State ---
bool buzzing = false;
bool awaitingAck = false;
String lastCommand = "";
unsigned long lastSendTime = 0;
const unsigned long resendInterval = 1000; // retry every 1s if no ACK
unsigned long lastBuzzTime = 0;
const unsigned long buzzInterval = 500;

// --- Callbacks ---
void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Send Status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}

void onReceive(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  char msg[len + 1];
  memcpy(msg, incomingData, len);
  msg[len] = '\0';

  Serial.print("Message received: ");
  Serial.println(msg);

  if (strcmp(msg, "buzz") == 0) {
    buzzing = true;
    awaitingAck = false;
    const char *ack = "ack_buzz";
    esp_now_send(peerAddress, (uint8_t *)ack, strlen(ack));
    Serial.println("ACK sent for buzz");
  } 
  else if (strcmp(msg, "stopbuzz") == 0) {
    buzzing = false;
    digitalWrite(BUZZER_PIN, LOW);
    awaitingAck = false;
    const char *ack = "ack_stopbuzz";
    esp_now_send(peerAddress, (uint8_t *)ack, strlen(ack));
    Serial.println("ACK sent for stopbuzz");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.mode(WIFI_STA);
  Serial.print("Sender MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);
  esp_now_add_peer(peerAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  Serial.println("Sender ready with ACK system.");
}

void loop() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_PIN);

  // Detect button press
  if (lastState == HIGH && currentState == LOW) {
    const char *msg = "press";
    esp_now_send(peerAddress, (uint8_t *)msg, strlen(msg));
    Serial.println("Sent: press");
  }
  lastState = currentState;

  // Handle buzzer blinking
  if (buzzing) {
    unsigned long now = millis();
    if (now - lastBuzzTime >= buzzInterval) {
      lastBuzzTime = now;
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));  // toggle buzzer state
    }
  }

  // Resend unacknowledged command
  if (awaitingAck && millis() - lastSendTime > resendInterval) {
    Serial.print("Resending command: ");
    Serial.println(lastCommand);
    esp_now_send(peerAddress, (uint8_t *)lastCommand.c_str(), lastCommand.length());
    lastSendTime = millis();
  }

  delay(20);
}

