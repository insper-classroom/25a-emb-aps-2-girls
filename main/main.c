/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/uart.h"

typedef struct adc
{
    int axis;
    int val;
} adc_t;

const int GPx = 28;
const int GPy = 27;

QueueHandle_t xQueueADC;

int converte_escala_ADC(int sinal)
{
    int escala_centrada_0 = sinal - 2047;
    int menor_resol = escala_centrada_0 * 255 / 2047;
    if (menor_resol >= -50 && menor_resol <= 50)
    {
        return 0;
    }
    else
    {
        return menor_resol;
    }
}
void x_task(void *p)
{
    adc_gpio_init(GPx);
    int dados[5] = {0};
    int numeros = 0;

    while (1)
    {
        adc_select_input(2); // GPIO28 -> x
        int resultx = adc_read();

        if (numeros < 5)
        {
            dados[numeros] = resultx;
            numeros++;
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                dados[i] = dados[i + 1];
            }
            dados[4] = resultx;
        }

        if (numeros > 0)
        {
            int soma = 0;
            for (int i = 0; i < numeros; i++)
            {
                soma += dados[i];
            }
            int media = soma / 5;

            int resultado_convert = converte_escala_ADC(media);
            adc_t adc_x;
            adc_x.axis = 0;
            adc_x.val = resultado_convert;

            if (adc_x.val != 0)
            {
                xQueueSend(xQueueADC, &adc_x, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p)
{
    adc_gpio_init(GPy);
    int dados[5] = {0};
    int numeros = 0;

    while (1)
    {
        adc_select_input(1); // GPIO27 -> y
        int resulty = adc_read();

        if (numeros < 5)
        {
            dados[numeros] = resulty;
            numeros++;
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                dados[i] = dados[i + 1];
            }
            dados[4] = resulty;
        }

        if (numeros > 0)
        {
            int soma = 0;
            for (int i = 0; i < numeros; i++)
            {
                soma += dados[i];
            }
            int media = soma / 5;

            int resultado_convert = converte_escala_ADC(media);
            adc_t adc_y;
            adc_y.axis = 1;
            adc_y.val = resultado_convert;
            if (adc_y.val != 0)
            {
                xQueueSend(xQueueADC, &adc_y, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
void uart_task(void *p)
{
    adc_t recebido;
    uart_init(uart0, 115200);

    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    while (1)
    {
        if (xQueueReceive(xQueueADC, &recebido, portMAX_DELAY))
        {
            uint8_t vec[4];
            vec[0] = 0xFF; // byte de sincronização QUE O PYTHON TA ESPERANDO rpa começar a ler os dados
            vec[1] = (uint8_t)recebido.axis;
            vec[2] = (uint8_t)(recebido.val & 0xFF);
            vec[3] = (uint8_t)((recebido.val >> 8) & 0xFF);
            uart_write_blocking(uart0, vec, 4); // 4.1.29.7.26. uart_write_blocking do manual da PICO
        }
    }
}

int main()
{
    stdio_init_all();
    adc_init();

    xQueueADC = xQueueCreate(64, sizeof(adc_t));
    xTaskCreate(x_task, "x task", 4095, NULL, 1, NULL);
    xTaskCreate(y_task, "y task", 4095, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart task", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}