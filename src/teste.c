#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"  // Certifique-se de que essa biblioteca está configurada corretamente

#define LED_PIN 12          // Pino conectado à matriz WS2812
#define BUTTON_PIN 14       // Pino onde o botão está conectado
#define NUM_LEDS 8          // Número de LEDs na matriz

// Função para configurar os LEDs em uma cor específica
void set_pixel(uint32_t color, uint led_index, uint32_t* buffer) {
    buffer[led_index] = color;
}

// Função para enviar os dados para os LEDs
void show(uint32_t* buffer, PIO pio, uint sm, uint num_leds) {
    for (int i = 0; i < num_leds; i++) {
        pio_sm_put_blocking(pio, sm, buffer[i] << 8u);  // Envia os dados para cada LED
    }
}

// Função principal
int main() {
    // Inicialização
    stdio_init_all();
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Configura o botão com resistor pull-up

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, true);

    uint32_t pixels[NUM_LEDS] = {0};  // Buffer dos LEDs

    while (1) {
        // Verifica se o botão foi pressionado
        if (gpio_get(BUTTON_PIN) == 0) {  // Botão pressionado (nível baixo)
            // Liga os LEDs em uma animação simples
            for (int i = 0; i < NUM_LEDS; i++) {
                set_pixel(0x00FF00, i, pixels);  // Verde
                show(pixels, pio, sm, NUM_LEDS);
                sleep_ms(100);  // Atraso entre os LEDs
            }
            sleep_ms(500);  // Pausa antes de apagar

            // Apaga os LEDs
            for (int i = 0; i < NUM_LEDS; i++) {
                set_pixel(0x000000, i, pixels);  // Desliga
            }
            show(pixels, pio, sm, NUM_LEDS);
        }
    }
}

