// === LED Pin Assignments ===
const int LED1 = 13;
const int LED2 = 12;
const int LED3 = 27;
const int LED4 = 14;
const int LED5 = 26;
const int LED6 = 25;

const int leds[] = {LED1, LED2, LED3, LED4, LED5, LED6};
const int ledCount = sizeof(leds) / sizeof(leds[0]);

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println("ESP32 LED Test Starting...");

  // Set all LED pins as outputs
  for (int i = 0; i < ledCount; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW); // all off at start
  }
}

void loop() {
  // Test each LED one by one
  for (int i = 0; i < ledCount; i++) {
    Serial.print("Turning ON LED ");
    Serial.println(i + 1);

    digitalWrite(leds[i], HIGH);
    delay(500);

    digitalWrite(leds[i], LOW);
    delay(200);
  }

  // Blink all LEDs together 3 times
  Serial.println("Blinking all LEDs...");
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < ledCount; i++) digitalWrite(leds[i], HIGH);
    delay(500);
    for (int i = 0; i < ledCount; i++) digitalWrite(leds[i], LOW);
    delay(500);
  }

  // Small pause before next cycle
  delay(1000);
}
