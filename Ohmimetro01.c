#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28

int R_conhecido = 10000;   
float R_x = 0.0;           
float ADC_VREF = 3.31;     
int ADC_RESOLUTION = 4095; 

// Função para aproximar o valor para uma resistência comercial (série E12)
float aproximar_resistencia(float resistencia) {
    // Tabela E12 (valores nominais para 1 década)
    float valores_e12[] = {1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2};
    int multiplicador = 1;

    // Trazer resistência para faixa entre 1 e 10
    while (resistencia >= 10.0f) {
        resistencia /= 10.0f;
        multiplicador *= 10;
    }
    while (resistencia < 1.0f) {
        resistencia *= 10.0f;
        multiplicador /= 10;
    }

    // Encontrar o valor mais próximo na tabela E12
    float melhor_valor = valores_e12[0];
    float menor_erro = fabs(resistencia - valores_e12[0]);

    for (int i = 1; i < 12; i++) {
        float erro = fabs(resistencia - valores_e12[i]);
        if (erro < menor_erro) {
            melhor_valor = valores_e12[i];
            menor_erro = erro;
        }
    }

    // Ajusta novamente para a escala original
    return melhor_valor * multiplicador;
}

// Função para converter valor de resistência em código de cores
void resistor_to_color_code(float resistencia, char *banda1, char *banda2, char *multiplicador) {
    int potencias[] = {1, 10, 100, 1000, 10000, 100000, 1000000};
    int base = 0;

    if (resistencia < 1) resistencia = 1; // Evitar problemas

    // Encontrar o melhor multiplicador
    while (resistencia >= 100 && base < 6) {
        resistencia /= 10;
        base++;
    }

    int valor = (int)(resistencia + 0.5); // Aproximação do valor

    int d1 = valor / 10;
    int d2 = valor % 10;

    char *cores[] = {
        "Preto", "Marrom", "Vermelho", "Laranja", "Amarelo",
        "Verde", "Azul", "Violeta", "Cinza", "Branco"
    };

    if (d1 >= 0 && d1 <= 9 && d2 >= 0 && d2 <= 9 && base >= 0 && base <= 6) {
        sprintf(banda1, "%s", cores[d1]);
        sprintf(banda2, "%s", cores[d2]);
        sprintf(multiplicador, "%s", cores[base]);
    } else {
        sprintf(banda1, "ERRO");
        sprintf(banda2, "ERRO");
        sprintf(multiplicador, "ERRO");
    }
}

int main()
{

  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  ssd1306_t ssd;
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN);

  char str_x[10];
  char str_y[10];
  char banda1[10], banda2[10], mult[10];

  bool cor = true;
  while (true)
  {
    adc_select_input(2);

    float soma = 0.0f;
    for (int i = 0; i < 500; i++)
    {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

    float R_medido = (R_conhecido * media) / (ADC_RESOLUTION - media);
    float R_aproximado = aproximar_resistencia(R_medido);

    sprintf(str_x, "%1.0f", media);
    sprintf(str_y, "%1.0f", R_aproximado);

    resistor_to_color_code(R_aproximado, banda1, banda2, mult);

    ssd1306_fill(&ssd, !cor);
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);
    ssd1306_line(&ssd, 3, 20, 123, 20, cor);

    ssd1306_draw_string(&ssd, "Ohmimetro", 15, 6);
    ssd1306_draw_string(&ssd, "ADC", 5, 26);
    ssd1306_draw_string(&ssd, "Res.", 5, 38);

    ssd1306_draw_string(&ssd, str_x, 45, 26);
    ssd1306_draw_string(&ssd, str_y, 45, 38);

    // Exibe o código de cores
    ssd1306_draw_string(&ssd, "Cores:", 5, 50);
    ssd1306_draw_string(&ssd, banda1, 40, 50);
    ssd1306_draw_string(&ssd, banda2, 70, 50);
    ssd1306_draw_string(&ssd, mult, 100, 50);

    ssd1306_send_data(&ssd);
    sleep_ms(700);
  }
}
