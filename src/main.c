#include "main.h"
#include "mci_clock.h"
#include "vga_driver.h"
#include <stdint.h>

#define H_RES 97
#define V_RES 75

#define SCREEN_RES (H_RES * V_RES)

uint8_t screen[SCREEN_RES];

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

int main() {
    EPL_SystemClock_Config();
    init_screen();

    for (int x = 0; x < H_RES; x++){
        for (int y = 0; y < V_RES; y++){
            screen[x + y*H_RES] = x+y;
        }
    }

    draw_to_frame_buffer(screen);

    while (1) {
        __WFI();
    }
}

