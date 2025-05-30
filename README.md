# STM32_VGA
#### Very basic implementation for getting a vga signal out of an stm32F091RC

## Introduction
This repo contains the code that is needed to get a very simple VGA signal out of an STM32F091RC, tested on a NUCLEO-F091RC dev board. I am using 3 Timer and the DMA to get a resolution of about 100x75 pixels in 2-bit per channel color.

**IMPORTANT NOTE:** Right now due to timing issues the image is only 97x75 pixels.

**LESS IMPORTANT NOTE:** The DMA copies 8-bit pixel data to the GPIOC port, so one could potentially have 2-bit per channel, or 2 channels at 3 bit and one at 2 bit. I only tested 1 bit per channel, so your mileage may vary.

## Quick start guide
If you only want to try out getting an image on your VGA screen then follow this guide.

**Step 1**
Make yourself familiar with platform IO and CMSIS, on a basic level.

**Step 2**
Buy a VGA breakout-board or cut open an old VGA cable (thats what I did). When you just want to cut a cable make sure that you keep both "bumps" or cylindrical objects. These contain ferrite beads, which act as a low-pass filter and help blocking EMI.

**Step 3**
Connect all cables accordingly:

 - PB0 &rarr; HSYNC
 - PB1 &rarr; VSYNC
 - PC0 &rarr; RED (through a 270 Ohm resistor)
 - PC1 &rarr; GREEN (through a 270 Ohm resistor)
 - PC2 &rarr; BLUE (through a 270 Ohm resistor)
 - GND &rarr; GND

The resistors make sure that you archieve 0.7V at the channels which corresponds to the full brightness. **You should use at least 270 Ohms or you may damage your monitor.** 

**Step 4**
After making sure you connected everything correctly flash the software to your board and plug the VGA cable into the monitor. Your monitor should awake from sleep and show a test image.

## Examples
I have included some examples inside the examples folder. The most interesting is pong.c. Here you have to copy the whole file to your main.c and also copy numbers.c and number.h to the corresponding folders. To controll both paddles you have to connect two potentiometers to PA0 and PA1. Have fun!

## Uploading your own image
I have included an image blank.png that shows you where you can draw and where the image should stay black. This image already has the right resolution and you can draw where there isn't pink. Make sure that you don't include the pink part in the image you want to display, it is only to show where not to draw. Also make sure that you only use the colors available to you, the python script will show everything as fully colored when not completely black. So for example if your color is #0011FF, the script will convert that to #00FFFF. Afterwards use the image_to_c.py script to convert the image to an array. Overwrite the already existing array inside test_screen.c with your newly created array and upload everything. If you did everything correctly you should see your image on your monitor. As I have explained before, the DMA copies 8bits to the GPIOC port, so you could display more colors. For this you would have to rewrite the python script since it only converts to 1 bit per channel (in a very bad way). 

## Quick overview of how the code works
##### HSYNC
The horizontal sync signal is created by using TIM1 which calls an interrupt every ~28μs. Inside the interrupt the signal is pulled low and the line_counter is increased by one. Also the DMA is updated to display the next line for every 8 lines inside the line_counter. This ensures that every line is drawn 8 times.

##### VSYNC
The vertical sync signal is created using TIM3 which also calls an interrupt every ~17.7ms. Inside the interrupt the line_counter is set to 0 again. 

##### Pixel Signal
The RGB-Signals are created by using the DMA which writes 8 bit from the frame_buffer to GPIOC. The DMA is triggered using TIM2 running at 4.8MHz.

##### Main loop
The main loop right now only waits for interrupts (__WFI()), so you can use this to draw things into the frame_buffer at your liking. Before the loop is entered everything is initialized inside the main function, the timers and the DMA get enabled/started and the array containing the test image is copied over to the frame_buffer. If you want to draw to the frame_buffer make sure that you start drawing after the backporch section. An example for this can be seen inside the for loop that copies the test_screen into the frame_buffer.

Since the stm32-F091RC runs at 48MHz in my configuration, you can't really archieve the timing you need for **SVGA Signal 800 x 600 @ 56 Hz timing**. I had to play around with the timings a bit and my monitor right now thinks that it is running in **800 x 600 @ 60 Hz** mode. As stated before, there are still timing issues which I may fix in the future. The problem is that I am already close to the maximum that the chip can do so timing everything correctly is very difficult (at least for a beginner like me). 