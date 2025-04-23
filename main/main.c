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
#include "hardware/i2c.h"
#include "mpu6050.h"
#include "Fusion.h"

#define SAMPLE_PERIOD (0.01f) 

const int MPU_ADDRESS = 0x68;
const int I2C_SDA_GPIO = 4;
const int I2C_SCL_GPIO = 5;
QueueHandle_t xQueuePos;

typedef struct coord
{
    int axis;
    int val;
} coord;

typedef struct adc
{
    int axis;
    int val;
} adc_t;

const int BTN_A = 15;
const int BTN_B = 14;
const int BTN_X = 13;
const int BTN_Y = 12;
const int BTN_SELECT = 10;
const int BTN_START = 11;
const int GPx = 28;
const int GPy = 27;

QueueHandle_t xQueueADC;
QueueHandle_t xQueueBTNS;

SemaphoreHandle_t xSemaphore_BTN_A;
SemaphoreHandle_t xSemaphore_BTN_B;
SemaphoreHandle_t xSemaphore_BTN_X;
SemaphoreHandle_t xSemaphore_BTN_Y;
SemaphoreHandle_t xSemaphore_BTN_SELECT;
SemaphoreHandle_t xSemaphore_BTN_START;


static void mpu6050_reset()
{
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x6B, 0x00};
    i2c_write_blocking(i2c_default, MPU_ADDRESS, buf, 2, false);
}

static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp)
{
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c_default, MPU_ADDRESS, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, MPU_ADDRESS, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    val = 0x43;
    i2c_write_blocking(i2c_default, MPU_ADDRESS, &val, 1, true);
    i2c_read_blocking(i2c_default, MPU_ADDRESS, buffer, 6, false); // False - finished with bus

    for (int i = 0; i < 3; i++)
    {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
        ;
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(i2c_default, MPU_ADDRESS, &val, 1, true);
    i2c_read_blocking(i2c_default, MPU_ADDRESS, buffer, 2, false); // False - finished with bus

    *temp = buffer[0] << 8 | buffer[1];
}

void btn_callback(uint gpio, uint32_t events)
{
    if (gpio == BTN_A && events == 0x4)
    { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_BTN_A, 0);
    }
    if (gpio == BTN_B && events == 0x4)
    { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_BTN_B, 0);
    }
    if (gpio == BTN_X && events == 0x4)
    { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_BTN_X, 0);
    }
    if (gpio == BTN_Y && events == 0x4)
    { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_BTN_Y, 0);
    }
    if (gpio == BTN_SELECT && events == 0x4)
    { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_BTN_SELECT, 0);
    }
    if (gpio == BTN_START && events == 0x4)
    { // fall edge
        xSemaphoreGiveFromISR(xSemaphore_BTN_START, 0);
    }
}

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

void btn_init(void)
{
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
    gpio_init(BTN_X);
    gpio_set_dir(BTN_X, GPIO_IN);
    gpio_pull_up(BTN_X);
    gpio_init(BTN_Y);
    gpio_set_dir(BTN_Y, GPIO_IN);
    gpio_pull_up(BTN_Y);
    gpio_init(BTN_SELECT);
    gpio_set_dir(BTN_SELECT, GPIO_IN);
    gpio_pull_up(BTN_SELECT);
    gpio_init(BTN_START);
    gpio_set_dir(BTN_START, GPIO_IN);
    gpio_pull_up(BTN_START);
}

void btn_task(void *p)
{
    btn_init();

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);
    // callback outros botões que usa. o msm callback já configurado
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_X, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_Y, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_SELECT, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_START, GPIO_IRQ_EDGE_FALL, true);

    while (true)
    {
        if (xSemaphoreTake(xSemaphore_BTN_A, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            int info_botao = 0;
            xQueueSend(xQueueBTNS, &info_botao, 0);
        }
        if (xSemaphoreTake(xSemaphore_BTN_B, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            int info_botao = 1;
            xQueueSend(xQueueBTNS, &info_botao, 0);
        }
        if (xSemaphoreTake(xSemaphore_BTN_X, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            int info_botao = 2;
            xQueueSend(xQueueBTNS, &info_botao, 0);
        }
        if (xSemaphoreTake(xSemaphore_BTN_Y, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            int info_botao = 3;
            xQueueSend(xQueueBTNS, &info_botao, 0);
        }
        if (xSemaphoreTake(xSemaphore_BTN_SELECT, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            int info_botao = 4;
            xQueueSend(xQueueBTNS, &info_botao, 0);
        }
        if (xSemaphoreTake(xSemaphore_BTN_START, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            int info_botao = 5;
            xQueueSend(xQueueBTNS, &info_botao, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
void x_task(void *p)
{
    int dados[5] = {0};
    adc_gpio_init(GPx);
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

        vTaskDelay(pdMS_TO_TICKS(50));
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

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void mpu6050_task(void *p)
{
    // configuracao do I2C
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(I2C_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_GPIO);
    gpio_pull_up(I2C_SCL_GPIO);

    FusionAhrs ahrs;
    FusionAhrsInitialise(&ahrs);

    mpu6050_reset();
    int16_t acceleration[3], gyro[3], temp;


    while (1)
    {
        mpu6050_read_raw(acceleration, gyro, &temp);

        FusionVector gyroscope = {
            .axis.x = gyro[0] / 131.0f, // Conversão para graus/s
            .axis.y = gyro[1] / 131.0f,
            .axis.z = gyro[2] / 131.0f,
        };

        FusionVector accelerometer = {
            .axis.x = acceleration[0] / 16384.0f, // Conversão para g
            .axis.y = acceleration[1] / 16384.0f,
            .axis.z = acceleration[2] / 16384.0f,
        };

        FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, SAMPLE_PERIOD);

        // const FusionEuler euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
 
        float aceler_x_g = accelerometer.axis.x;
        if (aceler_x_g  > 1.3 )
        {
            coord ace;
            ace.axis = 2;
            ace.val = 1;
            xQueueSend(xQueuePos, &ace, 0);
        }


        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

void uart_task(void *p)
{
    adc_t recebido;
    int info_botao;
    coord coord;
    uart_init(uart0, 115200);

    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    while (1)
    {
        if (xQueueReceive(xQueueADC, &recebido, pdMS_TO_TICKS(10)))
        {

            uint8_t vec[4];
            vec[0] = 0xFF; // byte de sincronização QUE O PYTHON TA ESPERANDO rpa começar a ler os dados
            vec[1] = (uint8_t)recebido.axis;
            vec[2] = (uint8_t)(recebido.val & 0xFF);
            vec[3] = (uint8_t)((recebido.val >> 8) & 0xFF);
            uart_write_blocking(uart0, vec, 4); // 4.1.29.7.26. uart_write_blocking do manual da PICO
        }
        if (xQueueReceive(xQueueBTNS, &info_botao, pdMS_TO_TICKS(10))){
            uint8_t pacote[4];
            pacote[0] = 0xFE;             
            pacote[1] = (uint8_t)info_botao; // ID do botão
            pacote[2] = 1;                   // pressionando = true
            pacote[3] = 0;
            uart_write_blocking(uart0, pacote, 4);
        }
        if (xQueueReceive(xQueuePos, &coord, pdMS_TO_TICKS(10)))
        {
            uint8_t vec[4];
            vec[0] = 0xFD; // byte de sincronização QUE O PYTHON TA ESPERANDO rpa começar a ler os dados
            vec[1] = (uint8_t)coord.axis;
            vec[2] = (uint8_t)(coord.val & 0xFF);
            vec[3] = (uint8_t)((coord.val >> 8) & 0xFF);
            uart_write_blocking(uart0, vec, 4); // 4.1.29.7.26. uart_write_blocking do manual da PICO
       }
    }
}

int main()
{
    stdio_init_all();
    adc_init();


    xSemaphore_BTN_A = xSemaphoreCreateBinary();
    xSemaphore_BTN_B = xSemaphoreCreateBinary();
    xSemaphore_BTN_X = xSemaphoreCreateBinary();
    xSemaphore_BTN_Y = xSemaphoreCreateBinary();
    xSemaphore_BTN_SELECT = xSemaphoreCreateBinary();
    xSemaphore_BTN_START = xSemaphoreCreateBinary();

    xQueueBTNS = xQueueCreate(10, sizeof(int));
    xQueueADC = xQueueCreate(10, sizeof(adc_t));
    xQueuePos = xQueueCreate(30, sizeof(coord));
    
    xTaskCreate(x_task, "x task", 4095, NULL, 1, NULL);
    xTaskCreate(y_task, "y task", 4095, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart task", 4095, NULL, 1, NULL);
    xTaskCreate(btn_task, "btn task", 4095, NULL, 1, NULL);
    xTaskCreate(mpu6050_task, "mpu6050_Task 1", 8192, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}




