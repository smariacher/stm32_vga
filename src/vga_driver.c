#include "vga_driver.h"

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
        current_line += 1; // Increase line (also used as timing for pulse)
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

void DMA_init() {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    DMA1_Channel2->CPAR = (uint32_t)&GPIOC->ODR; // Peripheral address (destination)
    DMA1_Channel2->CMAR = (uint32_t)frame_buffer; // Memory address (source)
    DMA1_Channel2->CNDTR = H_PIXEL;

    DMA1_Channel2->CCR = 0; // Clear CCR register

    DMA1_Channel2->CCR |= DMA_CCR_MINC; // Memory increment mode
    DMA1_Channel2->CCR |= DMA_CCR_DIR; // read from memory

    DMA1_Channel2->CCR |= DMA_CCR_CIRC; // Circular mode
}

void init_screen(){
    // Reset frame buffer
    for (int i = 0; i < RESOLUTION; i++){frame_buffer[i] = 0;}

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
}