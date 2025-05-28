#include "main.h"
#include "mci_clock.h"
#include "vga_driver.h"
#include "test_screen.h"

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

int main() {
    EPL_SystemClock_Config();
    init_screen();

    int x = 10;
    int y = 10;
    int dx = 1;
    int dy = 1;
    int size = 15;

    uint8_t color = 0b000;

    while (1) {
        clear_screen(0); // black background

        if (dx > 0 && dy > 0){color = 0b100;}
        if (dx < 0 && dy > 0){color = 0b010;}
        if (dx < 0 && dy < 0){color = 0b001;}
        if (dx > 0 && dy < 0){color = 0b111;}
        

        draw_rectangle(x, y, size, size, color, 1); // white filled square

        draw_to_frame_buffer(screen);

        x += dx;
        y += dy;

        if (x <= 0 || x + size >= H_RES) {
            dx = -dx;
        }
        if (y <= 0 || y + size >= V_RES) {
            dy = -dy;
        }

        delay_ms(10); // tweak speed
    }
}
