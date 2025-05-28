#pragma once
#include "main.h"

#define H_PIXEL 1024/8
#define H_FRONTPORCH 24/8
#define H_SYNC 72/8
#define H_BACKPORCH 128/8
#define H_VISIBLE 800/8

#define V_PIXEL 632/8 // Actually 625/8 
#define V_BACKPORCH 24/8 // Actually 22/8

#define TIM1_PSC 0
#define TIM1_ARR 1279
#define TIM1_PULSE_LENGTH 1

#define TIM3_PSC 46
#define TIM3_ARR 17020
#define TIM3_PULSE_LENGTH 39

#define TIM2_PSC 1
#define TIM2_ARR 4

#define RESOLUTION (V_PIXEL * H_PIXEL)

uint8_t frame_buffer[RESOLUTION];

uint16_t current_line;

void delay_us(uint32_t us);
void delay_cycles(uint32_t cycles);

void gpio_init();

void TIM1_BRK_UP_TRG_COM_IRQHandler();
void TIM3_IRQHandler();

void TIM1_init();
void TIM2_init();
void TIM3_init();

void DMA_init();

void init_screen();

