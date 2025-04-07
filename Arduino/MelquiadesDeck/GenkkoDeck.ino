#include <driver/i2s.h>
#include <driver/adc.h>

// Pines del PCM5102
#define I2S_BCK 26    // Bit Clock
#define I2S_LRCK 27   // Word Select (LRCK)
#define I2S_DATA 25   // Data Out (DIN)

// Potenciómetros (ADC1 Channels)
#define POT1 ADC1_CHANNEL_4  // GPIO32
#define POT2 ADC1_CHANNEL_5  // GPIO33
#define POT3 ADC1_CHANNEL_6  // GPIO34
#define POT4 ADC1_CHANNEL_7  // GPIO35
#define POT5 ADC1_CHANNEL_0  // GPIO36 (VP)
#define POT6 ADC1_CHANNEL_3  // GPIO39 (VN)

// Pulsadores (entrada digital)
#define BTN1 4
#define BTN2 16
#define BTN3 17
#define BTN4 18
#define BTN5 19
#define BTN6 21

// Configuración de I2S
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100, // Frecuencia de muestreo
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_LRCK,
    .data_out_num = I2S_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// Configuración inicial
void setup() {
  Serial.begin(115200);

  // Configurar I2S para el PCM5102
  setupI2S();

  // Configurar canales ADC
  adc1_config_width(ADC_WIDTH_BIT_12); // Resolución de 12 bits
  adc1_config_channel_atten(POT1, ADC_ATTEN_DB_11); // Rango 0-3.6V
  adc1_config_channel_atten(POT2, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(POT3, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(POT4, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(POT5, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(POT6, ADC_ATTEN_DB_11);

  // Configurar pines de los pulsadores como entrada digital con pull-up interno
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);
  pinMode(BTN5, INPUT_PULLUP);
  pinMode(BTN6, INPUT_PULLUP);

  Serial.println("Sistema iniciado...");
}

// Búfer para enviar audio (senoidal)
const int16_t audioBuffer[64] = {
  0, 3000, 6000, 9000, 12000, 15000, 18000, 21000, 24000, 27000, 30000, 32767, 30000, 27000, 24000, 21000,
  18000, 15000, 12000, 9000, 6000, 3000, 0, -3000, -6000, -9000, -12000, -15000, -18000, -21000, -24000,
  -27000, -30000, -32768, -30000, -27000, -24000, -21000, -18000, -15000, -12000, -9000, -6000, -3000
};

// Bucle principal
void loop() {
  // Leer valores de los potenciómetros
  int pot1 = formatearPotenciometro(adc1_get_raw(POT1));
  int pot2 = formatearPotenciometro(adc1_get_raw(POT2));
  int pot3 = formatearPotenciometro(adc1_get_raw(POT3));
  int pot4 = formatearPotenciometro(adc1_get_raw(POT4));
  int pot5 = formatearPotenciometro(adc1_get_raw(POT5));
  int pot6 = formatearPotenciometro(adc1_get_raw(POT6));

  // Imprimir valores en el monitor serial
  Serial.printf("Potenciómetros: %d, %d, %d, %d, %d, %d\n", pot1, pot2, pot3, pot4, pot5, pot6);

  // Leer estados de los pulsadores
  bool btn1 = digitalRead(BTN1) == LOW; // Activo en LOW
  bool btn2 = digitalRead(BTN2) == LOW;
  bool btn3 = digitalRead(BTN3) == LOW;
  bool btn4 = digitalRead(BTN4) == LOW;
  bool btn5 = digitalRead(BTN5) == LOW;
  bool btn6 = digitalRead(BTN6) == LOW;

  // Imprimir estados en el monitor serial
  Serial.printf("Pulsadores: %d, %d, %d, %d, %d, %d\n", btn1, btn2, btn3, btn4, btn5, btn6);

  // Enviar una señal de audio al PCM5102
  size_t bytesWritten;
  i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesWritten, portMAX_DELAY);

  delay(200); // Intervalo de actualización
}

int formatearPotenciometro(int lectura){
  int valor = ((lectura * 100) / 4095);
  return valor;
}
