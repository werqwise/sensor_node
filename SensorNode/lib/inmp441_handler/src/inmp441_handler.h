#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

float get_db();
int setup_inmp441();
float calculateSPL(float rms, float reference);
#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 8000
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_14
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

int setup_inmp441()
{

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    int err_code = i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
    Serial.print("err_code INMP441: ");
    Serial.println(err_code);
    while (1)
    {
        get_db();
    }
    return 1;
}

int32_t raw_samples[SAMPLE_BUFFER_SIZE];

float calculateRMS(int32_t *samples, int num_samples)
{
    double sum = 0;
    for (int i = 0; i < num_samples; i++)
    {
        float sample = (float)samples[i] / 2147483648.0; // Convert to floating point in range -1 to 1
        sum += sample * sample;
    }
    return sqrt(sum / num_samples);
}

float calculateSPL(float rms, float reference)
{
    return 20 * log10(rms / reference);
}

float get_db()
{
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int32_t);

    float rms = calculateRMS(raw_samples, samples_read);
    float spl = calculateSPL(rms, 0.00002); // Reference pressure 20 µPa

    Serial.printf("RMS: %f, SPL: %f dB\n", rms, spl);

    // Check for patterns indicating no sensor
    if (rms < 0.00001 || spl <= -92.0 || isinf(spl))
    {
        return 0.0; // Return 0 if sensor likely disconnected
    }

    return spl;
}
