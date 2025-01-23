#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "pio_matrix.pio.h"

//Definição de pinos, variáveis e número de LED
#define NUM_LEDS 25
#define MATRIZ_PIN 7
const uint8_t colunas[4] = {1, 2, 3, 4}; // Pinos das colunas
const uint8_t linhas[4] = {5, 6, 7, 8};  // Pinos das linhas

// Mapeamento das teclas do teclado
    const char teclado[4][4] = 
    {
    {'1', '2', '3', 'A'}, 
    {'4', '5', '6', 'B'}, 
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
    };

//Funções Utilizadas
static void gpio_irq_handler(uint gpio, uint32_t events);
uint32_t matrix_rgb(double b, double r, double g);
void formar_frames(double frame[NUM_PIXELS][3], PIO pio, uint sm);
void gerar_animacao(double animacao[][NUM_PIXELS][3], int num_frames, int delay_ms, PIO pio, uint sm);
char leitura_teclado(void);
void configurar_pino(int pino, bool direcao, bool estado);

//Rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){
    printf("Interrupção ocorreu no pino %d, no evento %d\n", gpio, events);
    printf("HABILITANDO O MODO GRAVAÇÃO");
	reset_usb_boot(0,0); //habilita o modo de gravação do microcontrolador
}

// Define intensidade para os leds RGB
uint32_t matrix_rgb(double b, double r, double g) {
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

// Lógica para utilizar uma matriz de 3 para configurar os desenhos dos LEDS
void formar_frames(double frame[NUM_LEDS][3], PIO pio, uint sm) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t valor_led = matrix_rgb(frame[i][0], frame[i][1], frame[i][2]); // R, G, B
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

//Função que junta os desenhos para fazer uma animação
void gerar_animacao (double animacao[][NUM_LEDS][3], int num_frames, int delay_ms, PIO pio, uint sm) {
    for (int frame = 0; frame < num_frames; frame++) {
        formar_frames(animacao[frame], pio, sm);
        sleep_ms(delay_ms); // Intervalo entre os quadros
    }
}

//Função pra ler o teclado
char leitura_teclado()
{
    // Desliga todos os pinos das colunas
    for (int i = 0; i < 4; i++){
        gpio_put(colunas[i], 1);
    }

    for (int i = 0; i < 4; i++) {// Desativa todas as colunas
        gpio_put(colunas[i], 1); 
    }

    for (int coluna = 0; coluna < 4; coluna++){ // Ativa a coluna atual (coloca o pino da coluna como 1)
        gpio_put(colunas[coluna], 0);

        for (int linha = 0; linha < 4; linha++){// Verifica o estado da linha. Se estiver em 0, a tecla foi pressionada
            if (gpio_get(linhas[linha]) == 0){
                char tecla = teclado[linha][coluna]; // Mapeia a tecla pressionada
                // Aguarda a tecla ser liberada (debounce)
                while (gpio_get(linhas[linha]) == 0){
                    sleep_ms(10); // Aguarda a tecla ser liberada
                }
                break; // Sai do laço após detectar a tecla
            }
        }

        // Desativa a coluna atual (coloca o pino da coluna como 0)
        gpio_put(colunas[coluna], 1);

    }
    return '\0'; // Retorna a tecla pressionada
}

// Função inicial para configurar os pinos
void configurar_pino(int pino, int direcao, bool estado) {
    gpio_init(pino);
    gpio_set_dir(pino, direcao);
    gpio_put(pino, estado);
}

//Função principal
int main() {
    PIO pio = pio0; 
    bool ok;
    configurar_pino(MATRIZ_PIN, GPIO_OUT, false); //inicializa o pino do LED 

    //Configuração do clock
    ok = set_sys_clock_khz(128000, false);
    stdio_init_all();

    //Configurações PIO
    printf("iniciando a transmissão PIO");
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);
    if (ok) printf("clock set to %ld\n", clock_get_hz(clk_sys));

    uint16_t i;
    uint32_t valor_led;
    double r = 0.0, b = 0.0 , g = 0.0;
    char aux;

    //Inicializa os pinos da matriz
    for (int i = 0; i < 4; i++)
    {//Os pinos de 1 a 4 sao outputs
        gpio_init(colunas[i]);
        gpio_set_dir(colunas[i], GPIO_OUT);
        gpio_put(colunas[i], 1); // Inicializa todas as colunas como baixo (low)
    }

    // Configuração dos pinos das linhas como entradas digitais
    for (int i = 0; i < 4; i++)
    {//Os pinos de 5 a 8 sao inputs
        gpio_init(linhas[i]);
        gpio_set_dir(linhas[i], GPIO_IN);
        gpio_pull_up(linhas[i]); // Habilita pull-up para as linhas
    }
    
    while (true) {
        char tecla = leitura_teclado();

        if (tecla != '\0') {
            printf("Tecla pressionada: %c\n", tecla);

        // Lê a tecla pressionada
        if(aux == '*')
            tecla = aux;
            reset_usb_boot(0,0);
        else if(aux!='n'){
            tecla = aux;
            printf("Tecla pressionada: %c\n", tecla);
            }

            // Executa ações se a tecla de envio foi pressionada e válida
            if (tecla!='*') 
                printf("\nEnviado !\n\n");
                // Executa ações baseadas na tecla 
                switch (tecla) {
                case '1': 
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '2': 
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '3': 
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '4':
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '5':
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '6':
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '7':
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '8':
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '9':
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '0':
                    gerar_animacao(animacao_Bia, 5, 500, pio, sm); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case '#': 
                default: break;
                }
            sleep_ms(200); // Intervalo de tempo menor para uma leitura mais rápida
            }
        return 0;//Teoricamente, nunca chega aqui por causa do loop infinito
    }
}

//Animações
 double animacao_Bia[5][NUM_LEDS][3]{
     { // Quadro 1
            {0.0, 0.0, 0.0}, {0.6, 0.0, 0.2}, {0.0, 0.0, 0.0}, {0.6, 0.0, 0.2}, {0.0, 0.0, 0.0},
            {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2},
            {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2},
            {0.0, 0.0, 0.0}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.6, 0.0, 0.2}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.6, 0.0, 0.2}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}
        },
        { // Quadro 2
            {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2},
            {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2},
            {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2},
            {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0}
        }
        { // Quadro 3
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0},
            {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2},
            {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}
        }
        { // Quadro 4
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}
        }
        { // Quadro 5
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}
        }
 }