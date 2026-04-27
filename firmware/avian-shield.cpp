/*
 * Avian Shield – main firmware
 *
 * Plays bobcat.wav / hawk.wav / owl.wav in random order over I2S with a
 * 500 ms pause between clips. Audio is embedded directly in the binary via
 * PlatformIO's board_build.embed_files (see platformio.ini).
 *
 * Hardware: ESP32 DOIT DevKit V1 + MAX98357A I2S amplifier
 *   BCLK  → GPIO 26
 *   LRCLK → GPIO 25
 *   DOUT  → GPIO 22
 *
 * Setup: copy media/bobcat.wav, hawk.wav, owl.wav into <pio-project>/data/
 */

#include <Arduino.h>
#include <driver/i2s.h>

// Symbols injected by board_build.embed_files = data/bobcat.wav ...
extern const uint8_t bobcat_start[] asm("_binary_data_bobcat_wav_start");
extern const uint8_t bobcat_end[]   asm("_binary_data_bobcat_wav_end");
extern const uint8_t hawk_start[]   asm("_binary_data_hawk_wav_start");
extern const uint8_t hawk_end[]     asm("_binary_data_hawk_wav_end");
extern const uint8_t owl_start[]    asm("_binary_data_owl_wav_start");
extern const uint8_t owl_end[]      asm("_binary_data_owl_wav_end");

#define I2S_PORT       I2S_NUM_0
#define I2S_BCLK_PIN   26
#define I2S_LRCLK_PIN  25
#define I2S_DOUT_PIN   22
#define DMA_BUF_COUNT  8
#define DMA_BUF_LEN    512

// Scans RIFF chunks and returns a pointer to the raw PCM samples.
// Sets dataBytes to the size of the data chunk on success.
static const uint8_t *findPcmData(const uint8_t *wav, size_t len, uint32_t &dataBytes) {
    if (len < 44 || memcmp(wav, "RIFF", 4) != 0 || memcmp(wav + 8, "WAVE", 4) != 0)
        return nullptr;

    const uint8_t *p   = wav + 12;
    const uint8_t *end = wav + len;

    while (p + 8 <= end) {
        uint32_t chunkLen = (uint32_t)p[4]
                          | (uint32_t)p[5] << 8
                          | (uint32_t)p[6] << 16
                          | (uint32_t)p[7] << 24;
        if (memcmp(p, "data", 4) == 0) {
            dataBytes = chunkLen;
            return p + 8;
        }
        p += 8 + chunkLen + (chunkLen & 1); // keep word-aligned
    }
    return nullptr;
}

static void setupI2S() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = 44100,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = DMA_BUF_COUNT,
        .dma_buf_len          = DMA_BUF_LEN,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
    };
    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_BCLK_PIN,
        .ws_io_num    = I2S_LRCLK_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };
    i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
    i2s_set_pin(I2S_PORT, &pins);
    i2s_zero_dma_buffer(I2S_PORT);
}

static void playWav(const uint8_t *start, const uint8_t *end, const char *name) {
    uint32_t dataBytes = 0;
    const uint8_t *pcm = findPcmData(start, end - start, dataBytes);
    if (!pcm) {
        Serial.printf("[!] %s: invalid WAV\n", name);
        return;
    }
    Serial.printf(">> %s (%u bytes)\n", name, dataBytes);

    size_t bytesWritten;
    const uint8_t *p    = pcm;
    const uint8_t *tail = pcm + dataBytes;
    while (p < tail) {
        size_t chunk = min((size_t)(tail - p), (size_t)(DMA_BUF_LEN * 4));
        i2s_write(I2S_PORT, p, chunk, &bytesWritten, portMAX_DELAY);
        p += chunk;
    }
    i2s_zero_dma_buffer(I2S_PORT);
}

struct Clip { const uint8_t *start, *end; const char *name; };

static const Clip clips[] = {
    { bobcat_start, bobcat_end, "bobcat" },
    { hawk_start,   hawk_end,   "hawk"   },
    { owl_start,    owl_end,    "owl"    },
};
static constexpr int NUM_CLIPS = sizeof(clips) / sizeof(clips[0]);

void setup() {
    Serial.begin(115200);
    Serial.println("\nAvian Shield v1.0");
    randomSeed(esp_random());
    setupI2S();
}

void loop() {
    int i = random(NUM_CLIPS);
    playWav(clips[i].start, clips[i].end, clips[i].name);
    delay(500);
}
