#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "main.pio.h"    // Header gerado a partir do arquivo PIO para a matriz de LEDs
#include "inc/ssd1306.h" // Biblioteca para o display OLED
#include "inc/font.h"    // Biblioteca de fontes e, possivelmente, ícones

// =================== DEFINIÇÕES ===================
// Display OLED (SSD1306) via I2C
#define I2C_PORT        i2c1
#define I2C_SDA         14
#define I2C_SCL         15
#define OLED_ADDR       0x3C
#define WIDTH           128
#define HEIGHT          64

// Botões para seleção do menu
#define BUTTON_A_PIN    5   // Botão A: seleciona visualizador de áudio ou sai do modo SOS
#define BUTTON_B_PIN    6   // Botão B: seleciona o modo SOS

// Microfone e matriz de LEDs
const uint microfone = 28;            // Microfone: GPIO28 para ADC2
#define NUM_PIXELS      25            // Matriz com 25 LEDs (ex.: 5x5)
#define MATRIX_PIN      7             // Pino de saída para a matriz

// Parâmetros do filtro para áudio
#define NOISE_THRESHOLD 2000          // Limiar de ruído (ajuste conforme necessário)
#define ADC_MAX         (4096 - NOISE_THRESHOLD) // Faixa útil após subtrair o threshold
#define EMA_ALPHA       1.4f          // Fator do filtro exponencial (valor ajustado)

// Pino do LED SOS e buzzer para o sinal SOS
#define SOS_LED_PIN     13            // LED (vermelho)
#define SOS_BUZZER_PIN  21            // Buzzer para o sinal SOS

// =================== TIPOS E VARIÁVEIS GLOBAIS ===================
typedef enum {
    MENU,
    AUDIO_VISUALIZER,
    SOS_MODE
} MenuState;

static MenuState menu_state = MENU;

// =================== PROTÓTIPOS DE FUNÇÕES ===================
void show_menu(ssd1306_t *ssd);
void run_audio_visualizer(PIO pio, uint sm);
void run_sos_mode(ssd1306_t *ssd);
uint32_t matrix_rgb(double r, double g, double b);
uint16_t filter_noise(uint16_t raw);
void display_volume(PIO pio, uint sm, uint16_t filtered_value);

// Funções auxiliares para o modo SOS com saída imediata
bool delay_with_exit(uint32_t ms);
bool send_sos_signal(void);
bool send_sos_signal_led_only(void);

// =================== FUNÇÃO PRINCIPAL ===================
int main() {
    stdio_init_all();

    // Inicializa o display OLED via I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa os botões do menu
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // Inicializa o pino do LED SOS (vermelho)
    gpio_init(SOS_LED_PIN);
    gpio_set_dir(SOS_LED_PIN, GPIO_OUT);
    gpio_put(SOS_LED_PIN, 0);
    
    // Inicializa o buzzer para SOS (pino 21)
    gpio_init(SOS_BUZZER_PIN);
    gpio_set_dir(SOS_BUZZER_PIN, GPIO_OUT);
    gpio_put(SOS_BUZZER_PIN, 0);

    // Inicializa o ADC para o microfone
    adc_init();
    adc_gpio_init(microfone);
    adc_select_input(2); // Seleciona o canal 2 (GPIO28)

    // Inicializa o PIO para a matriz de LEDs
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &main_program);
    uint sm = pio_claim_unused_sm(pio, true);
    main_program_init(pio, sm, offset, MATRIX_PIN);

    // Loop principal do menu
    while (true) {
        if (menu_state == MENU) {
            show_menu(&ssd);
            // Verifica se o usuário pressionou um dos botões
            if (!gpio_get(BUTTON_A_PIN)) {
                // Botão A: inicia o visualizador de áudio
                menu_state = AUDIO_VISUALIZER;
                sleep_ms(200);
            } else if (!gpio_get(BUTTON_B_PIN)) {
                // Botão B: inicia o modo SOS
                menu_state = SOS_MODE;
                sleep_ms(200);
            }
            sleep_ms(100);
        } else if (menu_state == AUDIO_VISUALIZER) {
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "Audio Visualizer", 0, 0);
            ssd1306_draw_string(&ssd, "Press B to exit", 0, 12);
            ssd1306_send_data(&ssd);
            run_audio_visualizer(pio, sm);
            menu_state = MENU;
        } else if (menu_state == SOS_MODE) {
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "SOS Mode", 0, 0);
            ssd1306_draw_string(&ssd, "Press A to exit", 0, 12);
            ssd1306_send_data(&ssd);
            run_sos_mode(&ssd);
            menu_state = MENU;
        }
    }
    
    return 0;
}

// =================== FUNÇÕES AUXILIARES ===================

// Exibe o menu no display OLED
void show_menu(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "Menu de opcoes:", 0, 0);
    ssd1306_draw_string(ssd, "A: Cap. audio", 0, 12);
    ssd1306_draw_string(ssd, "B: SOS Signal", 0, 24);
    ssd1306_draw_string(ssd, "Press A ou B", 0, 36);
    ssd1306_send_data(ssd);
}

// Executa o modo de visualização de áudio
void run_audio_visualizer(PIO pio, uint sm) {
    // Loop que roda até que o usuário pressione o botão B para sair do modo
    while (true) {
        if (!gpio_get(BUTTON_B_PIN)) { // Se o botão B for pressionado, sair do modo
            sleep_ms(200);
            break;
        }
        uint16_t raw_value = adc_read();
        uint16_t filtered_value = filter_noise(raw_value);
        display_volume(pio, sm, filtered_value);
        sleep_ms(50); // Atualização visual (20 Hz)
    }
}

// Função auxiliar para delay com checagem de saída (botão A)
// Retorna true se o botão A for pressionado durante o delay.
bool delay_with_exit(uint32_t ms) {
    uint32_t elapsed = 0;
    while (elapsed < ms) {
        sleep_ms(10);
        elapsed += 10;
        if (!gpio_get(BUTTON_A_PIN)) {  // Se o botão A for pressionado, retorna true.
            return true;
        }
    }
    return false;
}

// Função que transmite o sinal SOS (Morse: ... --- ...), controlando LED e buzzer (pino 21)
// Retorna true se o usuário solicitar a saída (ao pressionar A)
bool send_sos_signal(void) {
    // S = "dot dot dot"
    for (int i = 0; i < 3; i++) {
        gpio_put(SOS_LED_PIN, 1);
        gpio_put(SOS_BUZZER_PIN, 1);  // Se o buzzer for ativo baixo, inverta para 0
        if (delay_with_exit(250)) return true;
        gpio_put(SOS_LED_PIN, 0);
        gpio_put(SOS_BUZZER_PIN, 0);
        if (delay_with_exit(250)) return true;
    }
    if (delay_with_exit(750)) return true;
    // O = "dash dash dash"
    for (int i = 0; i < 3; i++) {
        gpio_put(SOS_LED_PIN, 1);
        gpio_put(SOS_BUZZER_PIN, 1);
        if (delay_with_exit(750)) return true;
        gpio_put(SOS_LED_PIN, 0);
        gpio_put(SOS_BUZZER_PIN, 0);
        if (delay_with_exit(250)) return true;
    }
    if (delay_with_exit(750)) return true;
    // S = "dot dot dot"
    for (int i = 0; i < 3; i++) {
        gpio_put(SOS_LED_PIN, 1);
        gpio_put(SOS_BUZZER_PIN, 1);
        if (delay_with_exit(250)) return true;
        gpio_put(SOS_LED_PIN, 0);
        gpio_put(SOS_BUZZER_PIN, 0);
        if (delay_with_exit(250)) return true;
    }
    return false;
}

// Função que transmite apenas o padrão LED de SOS (sem controlar o buzzer)
// Retorna true se o usuário solicitar a saída (ao pressionar A)
bool send_sos_signal_led_only(void) {
    // S = "dot dot dot"
    for (int i = 0; i < 3; i++) {
        gpio_put(SOS_LED_PIN, 1);
        if (delay_with_exit(250)) return true;
        gpio_put(SOS_LED_PIN, 0);
        if (delay_with_exit(250)) return true;
    }
    if (delay_with_exit(750)) return true;
    // O = "dash dash dash"
    for (int i = 0; i < 3; i++) {
        gpio_put(SOS_LED_PIN, 1);
        if (delay_with_exit(750)) return true;
        gpio_put(SOS_LED_PIN, 0);
        if (delay_with_exit(250)) return true;
    }
    if (delay_with_exit(750)) return true;
    // S = "dot dot dot"
    for (int i = 0; i < 3; i++) {
        gpio_put(SOS_LED_PIN, 1);
        if (delay_with_exit(250)) return true;
        gpio_put(SOS_LED_PIN, 0);
        if (delay_with_exit(250)) return true;
    }
    return false;
}

// Executa o modo SOS: transmite o sinal SOS e permite sair imediatamente ao pressionar o botão A
void run_sos_mode(ssd1306_t *ssd) {
    absolute_time_t start_time = get_absolute_time();
    while (true) {
        // Checa se o botão A foi pressionado para sair imediatamente
        if (!gpio_get(BUTTON_A_PIN)) {
            break;
        }
        uint32_t elapsed = to_ms_since_boot(start_time);
        bool exit_requested = false;
        if (elapsed < 2000) {
            exit_requested = send_sos_signal();
        } else {
            gpio_put(SOS_BUZZER_PIN, 1);  // Liga o buzzer continuamente (se for ativo baixo, inverta para 0)
            exit_requested = send_sos_signal_led_only();
        }
        if (exit_requested) {
            break;
        }
    }
    gpio_put(SOS_BUZZER_PIN, 0);  // Garante que o buzzer seja desligado ao sair do modo
    sleep_ms(200);
}

// Converte valores de R, G, B (0 a 1) em um inteiro de 32 bits para a matriz
uint32_t matrix_rgb(double r, double g, double b) {
    unsigned char R = (unsigned char)(r * 255);
    unsigned char G = (unsigned char)(g * 255);
    unsigned char B = (unsigned char)(b * 255);
    return (G << 24) | (R << 16) | (B << 8);
}

// Filtra o ruído utilizando um filtro exponencial (EMA)
// Se o valor filtrado estiver abaixo do limiar, retorna 0; senão, retorna o valor acima do threshold
uint16_t filter_noise(uint16_t raw) {
    static float ema = 0.0f;
    ema = EMA_ALPHA * raw + (1 - EMA_ALPHA) * ema;
    if (ema < NOISE_THRESHOLD)
        return 0;
    else
        return (uint16_t)(ema - NOISE_THRESHOLD);
}

// Exibe o nível de áudio na matriz de LEDs
void display_volume(PIO pio, uint sm, uint16_t filtered_value) {
    int num_lit = (filtered_value * NUM_PIXELS) / ADC_MAX;
    double brightness = 0.5; // Fator de brilho
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t color;
        if (i < num_lit) {
            // LED aceso: cor verde
            color = matrix_rgb(0.0, brightness, 0.0);
        } else {
            // LED apagado
            color = matrix_rgb(0.0, 0.0, 0.0);
        }
        pio_sm_put_blocking(pio, sm, color);
    }
}
