#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

#define LEDV 11 //verde
#define LEDVM 13 //vermelho

#define DISPLAY_WIDTH 128  // Largura do display em pixels
#define CHAR_WIDTH 12      // Largura média do caractere (se fonte maior)
#define START_COUNT 10     // Número inicial da contagem regressiva
#define HISTORICO_TAM 10   // Número de leituras armazenadas para média

// Histórico de temperaturas
float historico_temperaturas[HISTORICO_TAM] = {0};
int indice = 0;  // Índice para armazenar as temperaturas no histórico

// Função para calcular a média das últimas 10 temperaturas
float calcular_media_temperaturas() {
    float soma = 0;
    for (int i = 0; i < HISTORICO_TAM; i++) {
        soma += historico_temperaturas[i];
    }
    return soma / HISTORICO_TAM;
}

// Função para exibir um número centralizado no display contagem regressiva 
void display_countdown(int number) {
    
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    // Calcula o tamanho do buffer
    calculate_render_area_buffer_length(&frame_area);

    // Zera o display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);

    // Converte o número para string
    char text[3];
    sprintf(text, "%d", number);

    // Cálculo para centralizar o número no display
    int x = (DISPLAY_WIDTH - (strlen(text) * CHAR_WIDTH)) / 2;

    // Exibe o número no display
    ssd1306_draw_string(ssd, x, 20, text);

    // Renderiza no display
    render_on_display(ssd, &frame_area);
}

// Função para exibir texto no display
void display_text(const char *text1, const char *text2, const char *text3) {
    // Preparar área de renderização para o display
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    // Calcula o tamanho do buffer
    calculate_render_area_buffer_length(&frame_area);

    // Zera o display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);

    // Exibe os textos no display
    ssd1306_draw_string(ssd, 5, 8, text1);
    ssd1306_draw_string(ssd, 5, 28, text2);
    ssd1306_draw_string(ssd, 5, 48, text3);

    // Renderiza no display
    render_on_display(ssd, &frame_area);
}

int main() {
    char str[20];
    char media_str[20];

    stdio_init_all();

    gpio_init(LEDV);
    gpio_init(LEDVM);

    gpio_set_dir(LEDV, GPIO_OUT);    
    gpio_set_dir(LEDVM, GPIO_OUT);

    // Configura o ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // Inicialização do I2C
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);


    // Inicialização do display OLED
    ssd1306_init();

    sleep_ms(1000);  // Aguarda 1 segundo antes de atualizar o número
    display_text("MONITORAMENTO", "TEMPERATURA"," ");  // Exibe a abertura no display
    sleep_ms(5000);  // Aguarda 5 segundo antes de atualizar o número

    // Contagem regressiva de START_COUNT até 0
    for (int i = START_COUNT; i >= 0; i--) {
        display_countdown(i);
        sleep_ms(1000);  // Aguarda 1 segundo antes de atualizar o número
    }

    // Exibe uma mensagem após a contagem
    display_countdown(0);
    sleep_ms(2000);

    while (true) {
        uint16_t raw = adc_read();
        // Converte a voltagem em temperatura
        const float conversion = 3.3f / (1 << 12);
        float voltage = raw * conversion;
        // Converte a voltagem em temperatura (RP2040 datasheet)
        float temperature = 27 - (voltage - 0.706) / 0.001721;
        sprintf(str, "%.2f C", temperature);  // Converte a temperatura para string com 2 casas decimais
        
        // Armazena a temperatura no histórico
        historico_temperaturas[indice] = temperature;
        indice = (indice + 1) % HISTORICO_TAM; // Mantém o índice circular

        // Calcula a média das últimas 10 temperaturas
        float media = calcular_media_temperaturas();
        sprintf(media_str, "Media: %.2f C", media);

        printf("Temperatura: %s C\n", str);
        printf("%s | %s\n", str, media_str);

        sleep_ms(1500);
        display_text("Temperatura", str, media_str);  // Exibe a temperatura no display
        if (temperature >= 35.0)
        {
            gpio_put(LEDVM, 1);
            gpio_put(LEDV, 0);
        }
        else
        {
            gpio_put(LEDVM, 0);
            gpio_put(LEDV, 1);
        }
    }

    return 0;
}
