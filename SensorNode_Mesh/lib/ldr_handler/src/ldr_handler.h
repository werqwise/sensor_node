#define LIGHT_SENSOR_PIN 36 // ESP32 pin GIOP36 (ADC0)

int get_ldr()
{
    // reads the input on analog pin (value between 0 and 4095)
    int analogValue = analogRead(LIGHT_SENSOR_PIN);

    return analogValue;
}
