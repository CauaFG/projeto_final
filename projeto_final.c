#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// Definições de pinos (BitdogLab)
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define SSD1306_ADDR 0x3C
#define JOYSTICK_X_PIN 26 // ADC0
#define BOTAO_TREINAR 5   // Botão de treinamento
#define BOTAO_CONTADOR 6  // Botão do contador
#define LED_TREINAR 13    
#define LED_ENCONTRADO 11
#define BUZZER 21

// Parâmetros
#define MAX_SAMPLES 24 // Número máximo de amostras por dispositivo
#define MAX_DEVICES 3 // Número máximo de dispositivos treináveis
#define THRESHOLD 1000 // Limite para identificar um dispositivo
#define NUM_LEITURAS_INICIAL 10 // Leituras iniciais para a calibração do Joystick
#define TOLERANCIA 50 // Margem de erro para centralização do Joystick
#define LIMITE_CONTADOR 10000 // Limite do contador

// Estrutura para armazenar dispositivos
typedef struct {
    uint16_t signature[MAX_SAMPLES]; // Assinatura dos dispositivos
    char name[8]; // Nome do dispositivo
} Device;

// Variáveis globais
static Device devices[MAX_DEVICES]; // Lista de dispositivos armazenados
static volatile uint8_t device_count = 0; // Contador de dispositivos treinados
static volatile bool training = false; // Estado do modo de treinamento
static volatile bool modo_contador = false; // Estado do modo contador
static volatile uint32_t soma_contador = 0; // Acumulador do contador
static volatile uint16_t centro_joystick = 0; // Valor médio do Joystick no centro
static ssd1306_t ssd; // Estrutura para controle do display OLED

// Inicializa os periféricos
void init_hardware() {
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_select_input(0);

    // Calcula o valor médio inicial do joystick
    uint32_t soma_inicial = 0;
    for (uint8_t i = 0; i < NUM_LEITURAS_INICIAL; i++) {
        soma_inicial += adc_read();
        sleep_ms(10);
    }
    centro_joystick = soma_inicial / NUM_LEITURAS_INICIAL;
    printf("Centro do joystick: %u\n", centro_joystick);

    // Inicializa o barramento I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, SSD1306_ADDR, I2C_PORT);
    ssd1306_config(&ssd);

    // Inicializa os GPIOs
    gpio_init(BOTAO_TREINAR);
    gpio_set_dir(BOTAO_TREINAR, GPIO_IN);
    gpio_pull_up(BOTAO_TREINAR);
    
    gpio_init(BOTAO_CONTADOR);
    gpio_set_dir(BOTAO_CONTADOR, GPIO_IN);
    gpio_pull_up(BOTAO_CONTADOR);
    
    gpio_init(LED_TREINAR);
    gpio_set_dir(LED_TREINAR, GPIO_OUT);
    gpio_put(LED_TREINAR, 0);
    
    gpio_init(LED_ENCONTRADO);
    gpio_set_dir(LED_ENCONTRADO, GPIO_OUT);
    gpio_put(LED_ENCONTRADO, 0);
    
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 0);
}

// Lê valor ajustado do joystick com centro dinâmico
uint16_t ler_valor_joystick() {
    uint16_t valor_adc = adc_read();
    int16_t valor_centralizado = valor_adc - centro_joystick;
    if (abs(valor_centralizado) < TOLERANCIA) {
        return 0;
    }
    return abs(valor_centralizado);
}

// Coleta a assinatura do dispositivo
void read_signature(uint16_t* buffer) {
    for (uint8_t i = 0; i < MAX_SAMPLES; i++) {
        buffer[i] = ler_valor_joystick();
        sleep_ms(1);
    }
}

// Treina um novo dispositivo
void train_device(const char* name) {
    if (device_count < MAX_DEVICES) {
        training = true;
        printf("Treinando %s\n", name);
        gpio_put(LED_TREINAR, 1);
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Treinar:", 8, 8);
        ssd1306_draw_string(&ssd, name, 48, 8);
        ssd1306_send_data(&ssd);
        read_signature(devices[device_count].signature);
        strncpy(devices[device_count].name, name, 7);
        devices[device_count].name[7] = '\0';
        device_count++;
        training = false;
        gpio_put(LED_TREINAR, 0);
        printf("Treinamento completo\n");
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Pronto!", 8, 8);
        ssd1306_send_data(&ssd);
        sleep_ms(500);
    }
}

// Identifica um dispositivo baseado na assinatura
bool identify_device(uint16_t* current, char* result) {
    for (uint8_t i = 0; i < device_count; i++) {
        uint32_t diff = 0;
        for (uint8_t j = 0; j < MAX_SAMPLES; j++) {
            diff += abs(current[j] - devices[i].signature[j]);
            if (diff > THRESHOLD) break;
        }
        if (diff < THRESHOLD) {
            strcpy(result, devices[i].name);
            printf("Identificado: %s\n", result);
            return true;
        }
    }
    strcpy(result, "Unknown");
    return false;
}

int main() {
    stdio_init_all();
    init_hardware();

    // Teste inicial dos LEDs
    printf("Testando LEDs...\n");
    gpio_put(LED_TREINAR, 1);
    gpio_put(LED_ENCONTRADO, 1);
    sleep_ms(1000);
    gpio_put(LED_TREINAR, 0);
    gpio_put(LED_ENCONTRADO, 0);

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Energy Mon", 8, 8);
    ssd1306_draw_string(&ssd, "A: Treinar", 8, 24);
    ssd1306_draw_string(&ssd, "B: Contador", 8, 40);
    ssd1306_send_data(&ssd);
    sleep_ms(1000);

    uint16_t current[MAX_SAMPLES];
    char result[8];
    char buffer[16];
    absolute_time_t ultimo_tempo = get_absolute_time();

    while (1) {
        // Checagem manual do Botao_B
        if (!gpio_get(BOTAO_CONTADOR)) {
            modo_contador = !modo_contador;
            soma_contador = 0; // Reseta o contador ao mudar modo
            sleep_ms(500);     // Debounce
        }

        if (!gpio_get(BOTAO_TREINAR) && !training && !modo_contador) {
            train_device(device_count == 0 ? "Geladeira" : device_count == 1 ? "TV" : "Lamp");
            sleep_ms(1000);
        } else if (!training) {
            read_signature(current);
            uint16_t valor_positivo = ler_valor_joystick();

            if (modo_contador) {
                absolute_time_t tempo_atual = get_absolute_time();
                if (absolute_time_diff_us(ultimo_tempo, tempo_atual) >= 1000000) { // 1 segundo
                    if (soma_contador + valor_positivo <= LIMITE_CONTADOR) {
                        soma_contador += valor_positivo;
                    }
                    ultimo_tempo = tempo_atual;
                }
                sprintf(buffer, "Total: %lu", soma_contador);
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, buffer, 8, 8);
                ssd1306_send_data(&ssd);
            } else {
                bool found = identify_device(current, result);
                sprintf(buffer, "X: %d", valor_positivo);
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, buffer, 8, 8);
                ssd1306_draw_string(&ssd, result, 8, 24);
                ssd1306_send_data(&ssd);
                if (found) {
                    gpio_put(LED_ENCONTRADO, 1);
                    gpio_put(BUZZER, 1);
                    sleep_ms(200);
                    gpio_put(BUZZER, 0);
                    sleep_ms(300);
                    gpio_put(LED_ENCONTRADO, 0);
                }
            }
        }
        sleep_ms(100);
    }
    return 0;
}
