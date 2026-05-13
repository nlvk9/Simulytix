// -------------------------------------------------------
//  Your Arduino sketch — edit this file only.
//  Uses the same pinMode / digitalWrite / delay as Arduino.
// -------------------------------------------------------

const uint8_t LED_PIN = 13;

void setup()
{
    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
}
