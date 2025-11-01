

#include <ESP8266WiFi.h>
#include <espnow.h>

// MAC address of Sender (the one with the button + buzzer)
uint8_t peerAddress[] = {0xEC, 0xFA, 0xBC, 0xD4, 0x9A, 0xA4};  // Sender MAC


bool awaitingAck = false;
String lastCommand = "";
unsigned long lastSendTime = 0;
const unsigned long resendInterval = 1000; // 1s retry window

void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Send Status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}

void onReceive(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  char msg[len + 1];
  memcpy(msg, incomingData, len);
  msg[len] = '\0';

  Serial.print("Received: ");
  Serial.println(msg);

  if (strcmp(msg, "press") == 0) {
    const char *reply = "buzz";
    esp_now_send(peerAddress, (uint8_t *)reply, strlen(reply));
    awaitingAck = true;
    lastCommand = "buzz";
    lastSendTime = millis();
    Serial.println("Sent: buzz (waiting for ACK)");
  } 
  else if (strcmp(msg, "ack_buzz") == 0 && lastCommand == "buzz") {
    Serial.println("ACK received for buzz ✅");
    awaitingAck = false;
  } 
  else if (strcmp(msg, "ack_stopbuzz") == 0 && lastCommand == "stopbuzz") {
    Serial.println("ACK received for stopbuzz ✅");
    awaitingAck = false;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);
  esp_now_add_peer(peerAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  Serial.println("Receiver ready with ACK system.");
}

void loop() {
  // Auto stop buzz after 5 seconds (test logic)
  
    const char *reply = "stopbuzz";
    esp_now_send(peerAddress, (uint8_t *)reply, strlen(reply));
    awaitingAck = true;
    lastCommand = "stopbuzz";
    lastSendTime = millis();
    Serial.println("Sent: stopbuzz (waiting for ACK)");
  }

  // Retry logic
  if (awaitingAck && millis() - lastSendTime > resendInterval) {
    Serial.print("Resending command: ");
    Serial.println(lastCommand);
    esp_now_send(peerAddress, (uint8_t *)lastCommand.c_str(), lastCommand.length());
    lastSendTime = millis();
  }

  delay(20);
}

