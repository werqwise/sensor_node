
#define LIGHT_SENSOR_PIN 34 // ESP32 pin GIOP34 (ADC0)

int get_ldr() //less value; brighter light
{
    // reads the input on analog pin (value between 0 and 4095)
    int analogValue = analogRead(LIGHT_SENSOR_PIN);

    return analogValue;
}
