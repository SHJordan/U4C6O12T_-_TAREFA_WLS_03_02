#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include "hardware/uart.h"

// Inclui o cabeçalho do display e o PIO para WS2812
#include "inc/ssd1306.h"
#include "ws2812.pio.h"
#include "inc/font.h"

// Definições de pinos
#define PIN_WS2812 7 // Matriz 5x5 WS2812 na GPIO 7

// LEDs RGB
#define PIN_LED_GREEN 12 // LED verde
#define PIN_LED_RED 13   // LED vermelho (não utilizado)
#define PIN_LED_BLUE 11  // LED azul

// Botões
#define PIN_BUTTON_A 6 // Botão A para LED verde
#define PIN_BUTTON_B 5 // Botão B para LED azul

// I2C para o display SSD1306
#define PIN_I2C_SDA 14
#define PIN_I2C_SCL 15

// Tempo de debounce em milissegundos (200 ms)
#define DEBOUNCE_TIME_MS 200

// Variáveis globais para debounce e estado dos LEDs
volatile uint32_t last_button_a_press = 0;
volatile uint32_t last_button_b_press = 0;

volatile bool led_green_state = false;
volatile bool led_blue_state = false;

// Instância global do display SSD1306
#define I2C_PORT i2c1
ssd1306_t ssd;

// Configuração da matriz WS2812 (5x5)
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define NUM_PIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
static uint32_t led_colors[NUM_PIXELS]; // Buffer estático para as cores dos LEDs

// Variáveis para controle do WS2812 via PIO
PIO pio_ws2812;
uint sm_ws2812;
uint offset_ws2812;

/* =======================
   Funções WS2812 customizadas
   ======================= */

void ws2812_init(uint pin)
{
    pio_ws2812 = pio0; // Utiliza o PIO0
    sm_ws2812 = 0;     // State machine 0
    offset_ws2812 = pio_add_program(pio_ws2812, &ws2812_program);
    ws2812_program_init(pio_ws2812, sm_ws2812, offset_ws2812, pin, 800000, false);
}

void ws2812_clear()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        led_colors[i] = 0;
    }
}

void ws2812_set_pixel(int x, int y, uint32_t color)
{
    int index = y * MATRIX_WIDTH + x;
    if (index >= 0 && index < NUM_PIXELS)
    {
        led_colors[index] = color;
    }
}

void ws2812_show()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        pio_sm_put_blocking(pio_ws2812, sm_ws2812, led_colors[i] << 8u);
    }
}

/* =======================
   Debounce e Interrupções
   ======================= */

// Função de callback para interrupções dos botões (agora com debounce)
void gpio_callback(uint gpio, uint32_t events)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    char msg[64]; // Aumentei o tamanho para evitar overflow
    char msg2[64];
    char btnStr;
    if (gpio == PIN_BUTTON_A)
    {
        // Debounce para o botão A
        if (now - last_button_a_press >= DEBOUNCE_TIME_MS)
        {
            last_button_a_press = now;
            // Inverte estado.
            led_green_state = !led_green_state;
            gpio_put(PIN_LED_GREEN, led_green_state);

            btnStr = 'A';
            // Atualiza o display e envia mensagem via UART
            if (led_green_state)
            {
                snprintf(msg2, sizeof(msg2), "LED Verde ON");
            }
            else
            {
                snprintf(msg2, sizeof(msg2), "LED Verde OFF");
            }
            printf("%s\n", msg2);
            ssd1306_fill(&ssd, false); // Limpa o display
            ssd1306_draw_string(&ssd, "== SHJORDAN ==", 8, 35);
            sprintf(msg, "Botao %c", btnStr);
            ssd1306_draw_string(&ssd, msg, 0, 0);   // Escreve a mensagem
            ssd1306_draw_string(&ssd, msg2, 0, 20); // Escreve a mensagem
            ssd1306_send_data(&ssd);                // Envia para o display
        }
    }
    else if (gpio == PIN_BUTTON_B)
    {
        // Debounce para o botão B
        if (now - last_button_b_press >= DEBOUNCE_TIME_MS)
        {
            last_button_b_press = now;

            // Inverte estado.
            led_blue_state = !led_blue_state;
            gpio_put(PIN_LED_BLUE, led_blue_state);

            btnStr = 'B';
            // Atualiza o display e envia mensagem via UART
            if (led_blue_state)
            {
                snprintf(msg2, sizeof(msg2), "LED Azul %s", "ON");
            }
            else
            {
                snprintf(msg2, sizeof(msg2), "LED Azul %s", "OFF");
            }
            printf("%s\n", msg2);
            ssd1306_fill(&ssd, false); // Limpa o display
            ssd1306_draw_string(&ssd, "== SHJORDAN ==", 8, 35);
            sprintf(msg, "Botao %c", btnStr);
            ssd1306_draw_string(&ssd, msg, 0, 0);   // Escreve a mensagem
            ssd1306_draw_string(&ssd, msg2, 0, 20); // Escreve a mensagem
            ssd1306_send_data(&ssd);                // Envia para o display
        }
    }
}

// Exibe um padrão na matriz WS2812 para um dígito (0 a 9)
void display_digit_on_matrix(char digit)
{
    ws2812_clear();

    switch (digit)
    {
    case '0':
        // Desenha a borda em verde formando "0"
        for (int y = 0; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH; x++)
            {
                if (y == 0 || y == MATRIX_HEIGHT - 1 || x == 0 || x == MATRIX_WIDTH - 1)
                    ws2812_set_pixel(x, y, 0x00FF00);
            }
        }
        break;
    case '1':
        // Coluna vertical azul para "1"
        for (int y = 0; y < MATRIX_HEIGHT; y++)
            ws2812_set_pixel(2, y, 0x0000FF);
        ws2812_set_pixel(3, 0, 0x0000FF);
        ws2812_set_pixel(1, 0, 0x0000FF);
        ws2812_set_pixel(1, 3, 0x0000FF);
        break;
    case '2':
        // Diagonal vermelha para "2"
        // [ 4, 4], [ 3, 4], [ 2, 4], [ 1, 4][0, 4],
        // [ 0, 3], [ 1, 3], [ 2, 3], [ 3, 3][4, 3],
        // [ 4, 2], [ 3, 2], [ 2, 2], [ 1, 2][0, 2],
        // [ 0, 1], [ 1, 1], [ 2, 1], [ 3, 1][4, 1],
        // [ 4, 0], [ 3, 0], [ 2, 0], [ 1, 0][0, 0]
        ws2812_set_pixel(3, 4, 0xFF0000);
        ws2812_set_pixel(2, 4, 0xFF0000);
        ws2812_set_pixel(1, 4, 0xFF0000);
        ws2812_set_pixel(3, 3, 0xFF0000);
        ws2812_set_pixel(1, 2, 0xFF0000);
        ws2812_set_pixel(2, 2, 0xFF0000);
        ws2812_set_pixel(3, 2, 0xFF0000);
        ws2812_set_pixel(1, 1, 0xFF0000);
        ws2812_set_pixel(3, 0, 0xFF0000);
        ws2812_set_pixel(2, 0, 0xFF0000);
        ws2812_set_pixel(1, 0, 0xFF0000);
        break;
    case '3':
        ws2812_set_pixel(1, 4, 0x00FF00);
        ws2812_set_pixel(3, 3, 0x00FF00);
        ws2812_set_pixel(1, 2, 0x00FF00);
        ws2812_set_pixel(3, 1, 0x00FF00);
        ws2812_set_pixel(1, 0, 0x00FF00);
        ws2812_set_pixel(3, 4, 0x00FF00);
        ws2812_set_pixel(2, 4, 0x00FF00);
        ws2812_set_pixel(3, 2, 0x00FF00);
        ws2812_set_pixel(2, 2, 0x00FF00);
        ws2812_set_pixel(3, 0, 0x00FF00);
        ws2812_set_pixel(2, 0, 0x00FF00);
        break;
    case '4':
        ws2812_set_pixel(3, 4, 0x0000FF);
        ws2812_set_pixel(1, 4, 0x0000FF);
        ws2812_set_pixel(1, 3, 0x0000FF);
        ws2812_set_pixel(3, 3, 0x0000FF);
        ws2812_set_pixel(3, 2, 0x0000FF);
        ws2812_set_pixel(2, 2, 0x0000FF);
        ws2812_set_pixel(1, 2, 0x0000FF);
        ws2812_set_pixel(3, 1, 0x0000FF);
        ws2812_set_pixel(1, 0, 0x0000FF);
        break;
    case '5':
        ws2812_set_pixel(3, 4, 0xFF0000);
        ws2812_set_pixel(2, 4, 0xFF0000);
        ws2812_set_pixel(1, 4, 0xFF0000);
        ws2812_set_pixel(1, 3, 0xFF0000);
        ws2812_set_pixel(3, 2, 0xFF0000);
        ws2812_set_pixel(2, 2, 0xFF0000);
        ws2812_set_pixel(1, 2, 0xFF0000);
        ws2812_set_pixel(3, 1, 0xFF0000);
        ws2812_set_pixel(3, 0, 0xFF0000);
        ws2812_set_pixel(2, 0, 0xFF0000);
        ws2812_set_pixel(1, 0, 0xFF0000);
        break;
    case '6':
        ws2812_set_pixel(3, 4, 0x00FF00);
        ws2812_set_pixel(2, 4, 0x00FF00);
        ws2812_set_pixel(1, 4, 0x00FF00);
        ws2812_set_pixel(1, 3, 0x00FF00);
        ws2812_set_pixel(3, 2, 0x00FF00);
        ws2812_set_pixel(2, 2, 0x00FF00);
        ws2812_set_pixel(1, 2, 0x00FF00);
        ws2812_set_pixel(1, 1, 0x00FF00);
        ws2812_set_pixel(3, 1, 0x00FF00);
        ws2812_set_pixel(3, 0, 0x00FF00);
        ws2812_set_pixel(2, 0, 0x00FF00);
        ws2812_set_pixel(1, 0, 0x00FF00);
        break;
    case '7':
        ws2812_set_pixel(3, 4, 0x0000FF);
        ws2812_set_pixel(2, 4, 0x0000FF);
        ws2812_set_pixel(1, 4, 0x0000FF);
        ws2812_set_pixel(3, 3, 0x0000FF);
        ws2812_set_pixel(1, 2, 0x0000FF);
        ws2812_set_pixel(3, 1, 0x0000FF);
        ws2812_set_pixel(1, 0, 0x0000FF);
        break;
    case '8':
        ws2812_set_pixel(3, 4, 0xFF0000);
        ws2812_set_pixel(2, 4, 0xFF0000);
        ws2812_set_pixel(1, 4, 0xFF0000);
        ws2812_set_pixel(1, 3, 0xFF0000);
        ws2812_set_pixel(3, 3, 0xFF0000);
        ws2812_set_pixel(3, 2, 0xFF0000);
        ws2812_set_pixel(2, 2, 0xFF0000);
        ws2812_set_pixel(1, 2, 0xFF0000);
        ws2812_set_pixel(1, 1, 0xFF0000);
        ws2812_set_pixel(3, 1, 0xFF0000);
        ws2812_set_pixel(3, 0, 0xFF0000);
        ws2812_set_pixel(2, 0, 0xFF0000);
        ws2812_set_pixel(1, 0, 0xFF0000);
        break;
    case '9':
        ws2812_set_pixel(3, 4, 0x00FF00);
        ws2812_set_pixel(2, 4, 0x00FF00);
        ws2812_set_pixel(1, 4, 0x00FF00);
        ws2812_set_pixel(1, 3, 0x00FF00);
        ws2812_set_pixel(3, 3, 0x00FF00);
        ws2812_set_pixel(3, 2, 0x00FF00);
        ws2812_set_pixel(2, 2, 0x00FF00);
        ws2812_set_pixel(1, 2, 0x00FF00);
        ws2812_set_pixel(3, 1, 0x00FF00);
        ws2812_set_pixel(1, 0, 0x00FF00);
        break;
    default:
        ws2812_clear();
        // Caso genérico: todos os LEDs em branco
        /*for (int y = 0; y < MATRIX_HEIGHT; y++)
        {
            for (int x = 0; x < MATRIX_WIDTH; x++)
            {
                ws2812_set_pixel(x, y, 0xFFFFFF);
            }
        }*/
        break;
    }
    ws2812_show();
}

/* =======================
   Função principal
   ======================= */
int main()
{
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando o programa...\n");

    // LEDs RGB
    gpio_init(PIN_LED_GREEN);
    gpio_set_dir(PIN_LED_GREEN, GPIO_OUT);
    gpio_put(PIN_LED_GREEN, led_green_state); // Inicializa com o estado correto

    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);
    gpio_put(PIN_LED_RED, 0); // Vermelho desligado

    gpio_init(PIN_LED_BLUE);
    gpio_set_dir(PIN_LED_BLUE, GPIO_OUT);
    gpio_put(PIN_LED_BLUE, led_blue_state); // Inicializa com o estado correto

    // Botões com pull-up interno
    gpio_init(PIN_BUTTON_A);
    gpio_set_dir(PIN_BUTTON_A, GPIO_IN);
    gpio_pull_up(PIN_BUTTON_A);

    gpio_init(PIN_BUTTON_B);
    gpio_set_dir(PIN_BUTTON_B, GPIO_IN);
    gpio_pull_up(PIN_BUTTON_B);

    // Interrupções (apenas borda de descida - FALLING)
    gpio_set_irq_enabled_with_callback(PIN_BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(PIN_BUTTON_B, GPIO_IRQ_EDGE_FALL, true); // Usa a mesma callback

    // I2C para o display
    i2c_init(I2C_PORT, 400 * 1000); // Usar i2c_default
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);

    // Inicializa a estrutura do display SSD1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, 0x3C, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_draw_string(&ssd, "== SHJORDAN ==", 8, 35);
    ssd1306_send_data(&ssd); // Envia os dados (tela limpa)

    // Inicializa WS2812
    ws2812_init(PIN_WS2812);
    ws2812_clear();
    ws2812_show();

    // Loop principal
    while (true)
    {
        int c = getchar_timeout_us(10000); // Timeout curto
        if (c != PICO_ERROR_TIMEOUT && c != EOF)
        {
            char ch = (char)c;
            char buf[2] = {ch, '\0'};

            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "== SHJORDAN ==", 8, 35);
            sprintf(buf, "Digito: %c", ch);
            ssd1306_draw_string(&ssd, buf, 0, 0);
            ssd1306_send_data(&ssd);

            printf("Recebido: %c\n", ch);

            if (ch >= '0' && ch <= '9')
            {
                display_digit_on_matrix(ch);
            }
        }
        sleep_ms(10); // Pequeno delay
    }
    // Não precisa do free, pois não há malloc.
    return 0;
}