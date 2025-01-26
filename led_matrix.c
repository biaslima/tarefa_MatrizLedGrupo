#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"
#include "ws2818b.pio.h"

//Definição de pinos, variáveis e número de LED
#define NUM_LEDS 25
#define MATRIZ_PIN 11 //Tive que mudar porque o teclado já ocupava o pino 7
#define NUM_COLUNAS 5
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
void formar_frames(double frame[NUM_LEDS][3], PIO pio, uint sm);
void gerar_animacao(double animacao[][NUM_LEDS][3], int num_frames, int delay_ms);
char leitura_teclado(void);
void configurar_pino(int pino, bool direcao, bool estado);

//Rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){
    printf("Interrupção ocorreu no pino %d, no evento %d\n", gpio, events);
    printf("HABILITANDO O MODO GRAVAÇÃO");
	reset_usb_boot(0,0); //habilita o modo de gravação do microcontrolador
}

// Definição de pixel GRB
struct pixel_t {
 uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[NUM_LEDS];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
* Inicializa a máquina PIO para controle da matriz de LEDs.
*/
void npInit(uint pin) {

 // Cria programa PIO.
 uint offset = pio_add_program(pio0, &ws2818b_program);
 np_pio = pio0;

 // Toma posse de uma máquina PIO.
 sm = pio_claim_unused_sm(np_pio, false);
 if (sm < 0) {
   np_pio = pio1;
   sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
 }

 // Inicia programa na máquina PIO obtida.
 ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

 // Limpa buffer de pixels.
 for (uint i = 0; i < NUM_LEDS; ++i) {
   leds[i].R = 0;
   leds[i].G = 0;
   leds[i].B = 0;
 }
}

/**
* Atribui uma cor RGB a um LED.
*/
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
 leds[index].R = r;
 leds[index].G = g;
 leds[index].B = b;
}
//Em ordem crescente: 0.8; 0.1; 0.9; 0.4; 0.6; 0.3; 0.2; 0.7; 1.0
void definir_intensidade(const uint index, const double r, const double g, const double b){
 leds[index].R =(uint8_t) round(r*255.0);
 leds[index].G =(uint8_t) round(g*255.0);
 leds[index].B =(uint8_t) round(b*255.0);
 //if(index==0 || index==5)
 //   printf("b = %.2lf\n(index %d) leds[index].B = %d\n",b,index,leds[index].B);
}

/**
* Limpa o buffer de pixels.
*/
void npClear() {
 for (uint i = 0; i < NUM_LEDS; ++i)
   npSetLED(i, 0, 0, 0);
}

/**
* Escreve os dados do buffer nos LEDs.
*/
void npWrite() {
 // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
 for (uint i = 0; i < NUM_LEDS; ++i) {
   pio_sm_put_blocking(np_pio, sm, leds[i].G);
   pio_sm_put_blocking(np_pio, sm, leds[i].R);
   pio_sm_put_blocking(np_pio, sm, leds[i].B);
 }
}

int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return y * 5 + x; // Linha par (esquerda para direita).
    } else {
        return y * 5 + (4 - x); // Linha ímpar (direita para esquerda).
    }
}

//Corrige o Index pra que o LED certo seja acendido
uint correcao_index(int index){
     //Caso esteja numa linha ímpar
     if((index>=5 && index<10) || (index>=15 && index<20))
     return index<10 ? index+10:index-10;
     else
     return NUM_LEDS-index-1;
    }

void gerar_frame(double animacao[NUM_LEDS][3]){
     for(int i=0;i<NUM_LEDS;i++){
      definir_intensidade(correcao_index(i),animacao[i][0],animacao[i][1],animacao[i][2]);
     }
     npWrite();
    }

void gerar_animacao(double animacao[][NUM_LEDS][3], int num_frames, int delay_ms){
 for(int i=0;i<num_frames;i++){
     gerar_frame(animacao[i]);
     putchar('\n');
     sleep_ms(delay_ms);
    }
}

void buttonConfig(const uint BUTTON_PIN)
{
    
    gpio_init(BUTTON_PIN);                //inicializo o pino do botão
    gpio_set_dir(BUTTON_PIN, GPIO_IN);    //defini entrada
    gpio_pull_up(BUTTON_PIN);             //habilito o pull up interno 
}

//Função pra ler o teclado
char leitura_teclado()
{
    char numero = 'n'; // Valor padrão para quando nenhuma tecla for pressionada

    // Desliga todos os pinos das colunas
    for (int i = 0; i < 4; i++)
    {
        gpio_put(colunas[i], 1);
    }

    for (int coluna = 0; coluna < 4; coluna++)
    {
        // Ativa a coluna atual (coloca o pino da coluna como 1)
        gpio_put(colunas[coluna], 0);

        for (int linha = 0; linha < 4; linha++)
        {
            // Verifica o estado da linha. Se estiver em 0, a tecla foi pressionada
            if (gpio_get(linhas[linha]) == 0)
            {
                numero = teclado[linha][coluna]; // Mapeia a tecla pressionada
                // Aguarda a tecla ser liberada (debounce)
                while (gpio_get(linhas[linha]) == 0)
                {
                    sleep_ms(10); // Aguarda a tecla ser liberada
                }
                break; // Sai do laço após detectar a tecla
            }
        }

        // Desativa a coluna atual (coloca o pino da coluna como 0)
        gpio_put(colunas[coluna], 1);

        if (numero != 'n') // Se uma tecla foi pressionada, sai do laço de colunas
        {
            break;
        }
    }
 //printf("Chegou no fim da leitura do teclado\n");
 return numero; // Retorna a tecla pressionada
}

// Função inicial para configurar os pinos
void configurar_pino(int pino, bool direcao, bool estado) {
    gpio_init(pino);
    gpio_set_dir(pino, direcao);
    gpio_put(pino, estado);
}

typedef struct pixel_t pixel_t;

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[NUM_LEDS];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

// Funções de teclas específicas
void desligarTodosOsLeds() {
   npClear();
    npWrite();

}
void ligarLEDsAzuis() {
    for (int i = 0; i < NUM_LEDS; i++) {
        definir_intensidade(correcao_index(i), 0.0, 0.0, 1.0);
    }
    npWrite();
}
void ligarLEDsVermelhos() {
    for (int i = 0; i < NUM_LEDS; i++) {
        definir_intensidade(correcao_index(i), 0.8, 0.0, 0.0);
    }
    npWrite();
}
void ligarLEDsVerdes() {
    for (int i = 0; i < NUM_LEDS; i++) {
        definir_intensidade(correcao_index(i), 0.0, 0.5, 0.0);
    }
    npWrite();
}

void ligarLEDsBrancos() {
    for (int i = 0; i < NUM_LEDS; i++) {
        definir_intensidade(correcao_index(i), 0.2, 0.2, 0.2);
    }
    npWrite();
}

//Animações
//A 1ª dimensao é os frames, a 2ª o índice do LED, a 3ª a cor (RGB)
 double animacao_Bia[5][NUM_LEDS][3]={
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
        },
        { // Quadro 3
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0},
            {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2},
            {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 0.2}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}
        },
        { // Quadro 4
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.2}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}
        },
        { // Quadro 5
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}
        }
 };

 double animacao_Lorenzo[5][NUM_LEDS][3]={
     { // Quadro 1
            {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
            {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
            {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}
        },
        { // Quadro 2
            {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}
        },
        { // Quadro 3
            {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}
        },
        { // Quadro 4
            {0.0, 0.0, 0.0}, {0.7, 0.0, 0.8}, {0.0, 0.0, 0.0}, {0.7, 0.0, 0.8}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.7, 0.0, 0.8}, {0.7, 0.0, 0.8}, {0.7, 0.0, 0.8}, {0.0, 0.0, 0.0},
            {0.7, 0.0, 0.8}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.7, 0.0, 0.8},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}
        },
        { // Quadro 5
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0},
            {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0},
            {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}
        }
 };


double animacao_vini[29][NUM_LEDS][3] = {
    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },

    {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
    },
};

//Função principal
int main() {
     char tecla;
     stdio_init_all();
     npInit(MATRIZ_PIN);

     // Configuração dos pinos das colunas como saídas digitais
    for (int i = 0; i < 4; i++)
    {//Os pinos de 1 a 4 sao outputs
        gpio_init(colunas[i]);
        gpio_set_dir(colunas[i], GPIO_OUT);
        gpio_put(colunas[i], 1); // Inicializa todas as colunas como baixo
    }

    // Configuração dos pinos das linhas como entradas digitais
    for (int i = 0; i < 4; i++)
    {//Os pinos de 5 a 8 sao inputs
        gpio_init(linhas[i]);
        gpio_set_dir(linhas[i], GPIO_IN);
        gpio_pull_up(linhas[i]); // Habilita pull-up para as linhas
    }


    
    while (true) {
        tecla = leitura_teclado();

        // Lê a tecla pressionada
        if (tecla == '*') {
        printf("Reiniciando para modo de gravação...\n");
        reset_usb_boot(0, 0);
        }else if (tecla != 'n'){
            printf("Tecla pressionada: %c\n", tecla);
                // Executa ações baseadas na tecla 
                switch (tecla) {
                case '1': //printf("Yay, caso 1\n");
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '2': 
                    gerar_animacao(animacao_Lorenzo, 5, 1000); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '3': 
                    gerar_animacao(animacao_vini, 29, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '4':
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '5':
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '6':
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '7':
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '8':
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '9':
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case '0':
                    gerar_animacao(animacao_Bia, 5, 500); //Nome da aniimação, n de frames, fps , pio, sn
                    break;
                case 'A':
                    desligarTodosOsLeds();
                    break;
                case 'B':
                    ligarLEDsAzuis();
                    break;
                case 'C':
                    ligarLEDsVermelhos();
                    break;
                case 'D':
                    ligarLEDsVerdes();
                    break;
                case '#': 
                    ligarLEDsBrancos();
                default: break;
                }
             npClear();
             npWrite();
            sleep_ms(200); // Intervalo de tempo menor para uma leitura mais rápida
            }
     sleep_ms(150);
    }
 return 0;//Teoricamente, nunca chega aqui por causa do loop infinito
}