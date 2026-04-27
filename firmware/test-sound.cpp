#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

#define I2S_PORT        I2S_NUM_0
#define I2S_BCLK_PIN    26
#define I2S_LRCLK_PIN   25
#define I2S_DOUT_PIN    22

#define SAMPLE_RATE     44100
#define BUFFER_SAMPLES  256

static void setupI2S() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = BUFFER_SAMPLES,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
    };
    i2s_pin_config_t pins = {
        .bck_io_num    = I2S_BCLK_PIN,
        .ws_io_num     = I2S_LRCLK_PIN,
        .data_out_num  = I2S_DOUT_PIN,
        .data_in_num   = I2S_PIN_NO_CHANGE,
    };
    i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
    i2s_set_pin(I2S_PORT, &pins);
    i2s_zero_dma_buffer(I2S_PORT);
}

// Plays a sine-wave tone at the given frequency for durationMs milliseconds.
// amplitude is 0.0–1.0; keep it below 0.8 to avoid clipping on the MAX98357A.
static void playTone(float frequency, int durationMs, float amplitude = 0.6f) {
    static int16_t buf[BUFFER_SAMPLES * 2]; // interleaved L/R
    int totalSamples = (SAMPLE_RATE * durationMs) / 1000;
    int written = 0;
    size_t bytesOut;

    while (written < totalSamples) {
        int chunk = min(BUFFER_SAMPLES, totalSamples - written);
        for (int i = 0; i < chunk; i++) {
            float t      = (float)(written + i) / SAMPLE_RATE;
            int16_t s    = (int16_t)(amplitude * 32767.0f * sinf(2.0f * M_PI * frequency * t));
            buf[i * 2]   = s;
            buf[i * 2+1] = s;
        }
        i2s_write(I2S_PORT, buf, chunk * sizeof(int16_t) * 2, &bytesOut, portMAX_DELAY);
        written += chunk;
    }
}

static void silence(int ms) {
    i2s_zero_dma_buffer(I2S_PORT);
    delay(ms);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\navian-shield sound test");
    Serial.printf("I2S  BCLK=GPIO%d  LRCLK=GPIO%d  DOUT=GPIO%d\n",
                  I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN);
    setupI2S();
    Serial.println("I2S ready.");
}

void loop() {
    // Three spot-check tones
    Serial.println("500 Hz – 1 s");
    playTone(500.0f, 1000);
    silence(400);

    Serial.println("1000 Hz – 1 s");
    playTone(1000.0f, 1000);
    silence(400);

    Serial.println("2000 Hz – 1 s");
    playTone(2000.0f, 1000);
    silence(400);

    // Sweep 200 → 4000 Hz in 100 Hz steps (100 ms each)
    Serial.println("Sweep 200–4000 Hz");
    for (float f = 200.0f; f <= 4000.0f; f += 100.0f) {
        playTone(f, 100);
    }

    Serial.println("Cycle done. Waiting 3 s...\n");
    silence(3000);
}
