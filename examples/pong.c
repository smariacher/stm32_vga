/* 
PONG EXAMPLE

USAGE:
Connect two potentiometers to PA0 and PA1 to play.
Also include number.c and numbers.h in your project.

NOTE: 
I haven't implemented a win condition because I'm lazy
so after any player reaches 9 points it resets to zero

*/

#include "main.h"
#include "mci_clock.h"
#include "vga_driver.h"
#include "numbers.h"
#include <stdint.h>

#define H_RES 97
#define V_RES 75

#define SCREEN_RES (H_RES * V_RES)

uint8_t screen[SCREEN_RES];

void draw_rectangle(int x, int y, int width, int height, uint8_t color, uint8_t filled) {
    for (int row = 0; row < height; row++) {
        int current_y = y + row;
        if (current_y < 0 || current_y >= V_RES)
            continue;

        for (int col = 0; col < width; col++) {
            int current_x = x + col;
            if (current_x < 0 || current_x >= H_RES)
                continue;

            if (filled || row == 0 || row == height - 1 || col == 0 || col == width - 1) {
                screen[current_y * H_RES + current_x] = color;
            }
        }
    }
}

void clear_screen(uint8_t color) {
    for (int i = 0; i < SCREEN_RES; i++) {
        screen[i] = color;
    }
}

void delay_ms(volatile uint32_t ms) {
    // crude software delay loop (adjust as needed)
    for (volatile uint32_t i = 0; i < ms * 4000; i++) {
        __asm__("nop");
    }
}

void adc_init() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    // Set PA0 and PA1 to analog mode
    GPIOA->MODER |= GPIO_MODER_MODER0 | GPIO_MODER_MODER1;

    // Disable scan mode and continuous mode
    ADC1->CFGR1 &= ~(ADC_CFGR1_CONT | ADC_CFGR1_SCANDIR);

    // Set sample time
    ADC1->SMPR |= ADC_SMPR_SMP_0;

    // Clear ADRDY if set
    if ((ADC1->ISR & ADC_ISR_ADRDY) != 0) {
        ADC1->ISR |= ADC_ISR_ADRDY;
    }

    // Enable ADC
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
}

uint16_t adc_read_channel(uint8_t channel) {
    ADC1->CHSELR = 0; // Clear previous selection
    ADC1->CHSELR |= (1 << channel); // Select desired channel

    ADC1->CR |= ADC_CR_ADSTART; // Start conversion
    while (!(ADC1->ISR & ADC_ISR_EOC)); // Wait for conversion to finish

    return (uint16_t)ADC1->DR; // Return ADC value
}

int main() {
    EPL_SystemClock_Config();
    adc_init();
    init_screen();

    // Paddle settings
    const uint8_t paddle_height = 15;
    const uint8_t paddle_width = 2;
    const uint8_t paddle_margin = 3;

    // Ball settings
    int ball_x = H_RES / 2;
    int ball_y = V_RES / 2;
    int ball_dx = 1;
    int ball_dy = 1;
    const uint8_t ball_size = 2;

    uint8_t player1_score = 0;
    uint8_t player2_score = 0;

    delay_ms(2000);

    while (1) {
        clear_screen(0); // Clear screen to black

        // Read paddles from analog inputs
        uint8_t left_paddle_y = adc_read_channel(0) / 55;
        uint8_t right_paddle_y = adc_read_channel(1) / 55;

        // Draw paddles
        draw_rectangle(paddle_margin, left_paddle_y, paddle_width, paddle_height, 0b001, 1); // blue
        draw_rectangle(H_RES - paddle_margin - paddle_width, right_paddle_y, paddle_width, paddle_height, 0b100, 1); // red

        // Draw ball
        draw_rectangle(ball_x, ball_y, ball_size, ball_size, 0b111, 1); // white

        // Ball movement
        ball_x += ball_dx;
        ball_y += ball_dy;

        // Bounce off top and bottom
        if (ball_y <= 0 || ball_y + ball_size >= V_RES) {
            ball_dy = -ball_dy;
        }

        // Bounce off left paddle
        if (ball_x <= paddle_margin + paddle_width &&
            ball_y + ball_size >= left_paddle_y &&
            ball_y <= left_paddle_y + paddle_height) {
            ball_dx = -ball_dx;
            ball_x = paddle_margin + paddle_width; // prevent sticking
        }

        // Bounce off right paddle
        if (ball_x + ball_size >= H_RES - paddle_margin - paddle_width &&
            ball_y + ball_size >= right_paddle_y &&
            ball_y <= right_paddle_y + paddle_height) {
            ball_dx = -ball_dx;
            ball_x = H_RES - paddle_margin - paddle_width - ball_size; // prevent sticking
        }

        // Reset ball if out of bounds
        if (ball_x < 0) {
            ball_x = H_RES / 2;
            ball_y = V_RES / 2;
            ball_dx = (ball_dx > 0) ? -1 : 1; // switch serve direction
            ball_dy = (ball_dy > 0) ? -1 : 1;
            player2_score += 1;
            if (player2_score >= 10){player2_score = 0;}
        }

        if (ball_x > H_RES) {
            ball_x = H_RES / 2;
            ball_y = V_RES / 2;
            ball_dx = (ball_dx > 0) ? -1 : 1; // switch serve direction
            ball_dy = (ball_dy > 0) ? -1 : 1;
            player1_score += 1;
            if (player1_score >= 10){player1_score = 0;}
        }

        for (int x = 0; x < 5; x++){
            for (int y = 0; y < 5; y++){
                screen[(x+40) + (y+1)*H_RES] = numbers[player1_score][x + y*5];
            }
        }

        for (int x = 0; x < 3; x++){
            screen[(x+47) + (3*H_RES)] = 0b111;
        }

        for (int x = 0; x < 5; x++){
            for (int y = 0; y < 5; y++){
                screen[(x+52) + (y+1)*H_RES] = numbers[player2_score][x + y*5] * 4;
            }
        }

        draw_to_frame_buffer(screen);
        delay_ms(10);
    }
}

