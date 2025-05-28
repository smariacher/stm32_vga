#include "main.h"
#include "mci_clock.h"
#include "bat_animation.h"
#include "test_screen.h"

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

uint8_t line_buffer[H_PIXEL];
uint8_t frame_buffer[RESOLUTION];

uint16_t current_line = 0;

void delay_us(uint32_t us)
{
    // Simple delay loop - adjust multiplier based on optimization level
    volatile uint32_t count = us * 12; // Approximate for 48MHz, no optimization
    while(count--);
}

void delay_cycles(uint32_t cycles)
{
    volatile uint32_t count = cycles;
    while(count--);
}

void gpio_init() {
    // Enable GPIOB 0,1 for horizontal and vertical clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    
    // Set as output
    GPIOB->MODER |= (GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0); 
    
    // Set to high-speed
    GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR0 | GPIO_OSPEEDER_OSPEEDR1);

    // Initialize high
    GPIOB->BSRR |= (GPIO_BSRR_BS_0 | GPIO_BSRR_BS_1);
    
    // Enable GPIOC 0-7 for color output 
    // (can effectivly use 6 ports but maybe 2 color channels have higher bit depth)
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    // Set as output and high-speed
    for (int pin = 0; pin < 8; pin++){
        GPIOC->MODER |= 0b01 << (2*pin);
        GPIOC->OSPEEDR |= 0b11 << (2*pin);
    }
}

void TIM1_BRK_UP_TRG_COM_IRQHandler() {
    if (TIM1->SR & TIM_SR_UIF){
        TIM1->SR &= ~TIM_SR_UIF; // Clear interrupt flag

        // Generate low pulse on PB0
        GPIOB->BSRR = GPIO_BSRR_BR_0;
        current_line += 1;
        delay_cycles(6);
        GPIOB->BSRR = GPIO_BSRR_BS_0;
        
        DMA1_Channel2->CMAR = (uint32_t)frame_buffer + (current_line >> 3) * H_PIXEL;

    }
}

void TIM3_IRQHandler() {
    if (TIM3->SR & TIM_SR_UIF){
        TIM3->SR &= ~TIM_SR_UIF; // Clear interrupt flag

        // Gernate low pulse on PB1
        GPIOB->BSRR |= GPIO_BSRR_BR_1;

        delay_us(TIM3_PULSE_LENGTH);

        GPIOB->BSRR |= GPIO_BSRR_BS_1;

        // Reset line
        current_line = 0;
    }
}

void TIM1_init() {
    // Enable TIM1 clock
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // TIM1 configuration for 28µs period on PB0
    TIM1->CR1 = 0;
    TIM1->PSC = TIM1_PSC;
    TIM1->ARR = TIM1_ARR;
    TIM1->CNT = 0;
    
    // Enable update interrupt
    TIM1->DIER |= TIM_DIER_UIE;
    
    // Generate update event to load prescaler
    TIM1->EGR |= TIM_EGR_UG;
    
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);
}

void TIM2_init() {
    // Enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // TIM2 configuration for 4.8MHz period for pixel transfer
    TIM2->PSC = TIM2_PSC;
    TIM2->ARR = TIM2_ARR;

    TIM2->DIER |= TIM_DIER_UDE;
}

void TIM3_init() {
    // Enable TIM3 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // TIM3 configuration for 17.7ms period on PB1
    TIM3->CR1 = 0;
    TIM3->PSC = TIM3_PSC;
    TIM3->ARR = TIM3_ARR;
    TIM3->CNT = 0;

    // Enable update interrupt
    TIM3->DIER |= TIM_DIER_UIE;

    // Generate update event to load prescaler
    TIM3->EGR |= TIM_EGR_UG;

    // Enable timer interrupt in NVIC and set priority
    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM3_IRQn, 1);
}

void DMA1_Channel4_5_IRQHandler(){
    if (DMA1->ISR & DMA_ISR_TCIF4){
        DMA1->IFCR |= DMA_IFCR_CTCIF4;
    }
}

void DMA_init() {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    DMA1_Channel2->CPAR = (uint32_t)&GPIOC->ODR; // Peripheral address (destination)
    DMA1_Channel2->CMAR = (uint32_t)frame_buffer; // Memory address (source)
    DMA1_Channel2->CNDTR = H_PIXEL;

    DMA1_Channel2->CCR = 0; // Clear CCR register

    DMA1_Channel2->CCR |= DMA_CCR_MINC; // Memory increment mode
    DMA1_Channel2->CCR |= DMA_CCR_DIR; // read from memory

    DMA1_Channel2->CCR |= DMA_CCR_CIRC; // Circular mode
    
    DMA1_Channel2->CCR |= DMA_CCR_TCIE; // Enable transfer complete interupt

    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn); // Enable DMA interrupt in NVIC
    NVIC_SetPriority(DMA1_Channel4_5_IRQn, 2); // Set priority
}

int main(){
    EPL_SystemClock_Config();

    // for (int i = 0; i < H_PIXEL; i++){
            
    //     if (i >= 15){line_buffer[i] = i;}
    //     else {line_buffer[i] = 0;}

    //     if (i <= H_BACKPORCH){line_buffer[i] = 0;}
    //     if (i >= (H_BACKPORCH + (H_VISIBLE))){line_buffer[i] = 0;}
    // }

    // for (int v = 0; v < V_PIXEL; v++){
    //     for (int h = 0; h < H_PIXEL; h++){
    //         frame_buffer[h + v*H_PIXEL] = line_buffer[h] + v;
    //     }
    // }

    // uint8_t SQUARE_SIZE = 30;

    // int square_left = H_PIXEL / 2 - SQUARE_SIZE / 2;
    // int square_top = V_PIXEL / 2 - SQUARE_SIZE / 2;

    // for (int v = 0; v < V_PIXEL; v++) {
    //     for (int h = 0; h < H_PIXEL; h++) {
    //         if (h >= square_left && h < square_left + SQUARE_SIZE &&
    //             v >= square_top  && v < square_top  + SQUARE_SIZE) {
    //             frame_buffer[h + v * H_PIXEL] = 0b111;  // Inside the square
    //         } else {
    //             frame_buffer[h + v * H_PIXEL] = 0;      // Outside the square
    //         }
    //     }
    // }

    // // Bat Animation
    // for (int i = 0; i < 9600; i++){
    //     frame_buffer[i] = all_frames[0][i];
    // }

    // Reset frame buffer
    for (int i = 0; i < RESOLUTION; i++){frame_buffer[i] = 0;}

    // Test screen
    for (int i = 0; i < 9600; i++){
        frame_buffer[i + (V_BACKPORCH * H_PIXEL)] = image_test_screen[i];
    }

    gpio_init();
    TIM1_init();
    TIM2_init();
    TIM3_init();

    DMA_init();

    // Enable DMA transfer
    DMA1_Channel2->CCR |= DMA_CCR_EN;

    // Start timers
    TIM1->CR1 |= TIM_CR1_CEN;
    TIM3->CR1 |= TIM_CR1_CEN;
    delay_cycles(10);
    TIM2->CR1 |= TIM_CR1_CEN;

    uint8_t frame = 0;

    while(1){
        
        // // Bat animation
        // for (int i = 0; i < 9600; i++){
        //     frame_buffer[i] = all_frames[frame][i];
        // }

        // frame++;
        // if (frame >= 12) {frame = 0;}
        // delay_us(25000);

        __WFI();

    }
}