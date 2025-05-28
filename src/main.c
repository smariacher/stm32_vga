#include "main.h"
#include "mci_clock.h"
#include "vga_driver.h"

int main(){
    EPL_SystemClock_Config();

    init_screen();

    while(1) {

    }
}