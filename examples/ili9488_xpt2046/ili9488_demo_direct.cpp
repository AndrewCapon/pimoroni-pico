#include <string.h>
#include <math.h>
#include <vector>
#include <cstdlib>


#include "drivers/ili9488/ili9488.hpp"
#include "drivers/xpt2046/xpt2046.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"

#include "elapsed_us.hpp"

using namespace pimoroni;

const char* msg = "Hello ILI9488";

Rotation rotation = ROTATE_90;

#define ILI9488_WIDTH (320)
#define ILI9488_HEIGHT (480)
// you might need to lower the baud rate
// try 156'250'000 then 312'500'000 if you are having issues
#define ILI9488_BAUD_RATE (625'000'000)

// if you do not have a reset pin wired up change to
// #define RESET_PIN PIN_UNUSED
#define ILI9488_RESET_PIN (21)


#define USE_TOUCHSCREEN 1
#if USE_TOUCHSCREEN
  #define XPT2046_WIDTH (240)
  #define XPT2046_HEIGHT (320)
  #define XPT2046_ROTATION_OFFSET (0)
  #define XPT2046_SCK (2)
  #define XPT2046_MOSI (3)
  #define XPT2046_MISO (0)
  #define XPT2046_IRQ (15)
  #define XPT2046_CS (14)

#endif
 

int main() {
  SPIPins touch_spi = {spi0, XPT2046_CS, XPT2046_SCK, XPT2046_MOSI, XPT2046_MISO, PIN_UNUSED, PIN_UNUSED};
  XPT2046	xpt2046(XPT2046_WIDTH, XPT2046_HEIGHT, (Rotation)((rotation+XPT2046_ROTATION_OFFSET)%360), touch_spi, XPT2046_IRQ, false, 500'000);

// void stdio_uart_init_full(struct uart_inst *uart, uint baud_rate, int tx_pin, int rx_pin) {
//   stdio_uart_init_full(struct uart_inst *uart, uint baud_rate, int tx_pin, int rx_pin) {
  int tx_pin = 12;
  int rx_pin = 13;
  
  stdio_uart_init_full(uart_default, PICO_DEFAULT_UART_BAUD_RATE, tx_pin, rx_pin);

  stdio_init_all();
  for(int i=0; i < 10; i++)
    printf("hello %d\n", i);

  while(true) {  
    xpt2046.update();
    if(xpt2046.is_touched()) {
      Point p = xpt2046.get_point();

      printf("p = %ld, %ld\n", p.x, p.y);
    }
  }

  return 0;
}

//  //SPIPins lcd_spi = get_spi_pins(BG_SPI_FRONT);
SPIPins lcd_spi = {spi0, 20, 2, 3, PIN_UNUSED, 22, PIN_UNUSED };

// int main() {
//   stdio_init_all();
//   for(int i=0; i < 10; i++)
//     printf("hello %d\n", i);

//   ILI9488 ili9488(ILI9488_WIDTH, ILI9488_HEIGHT, rotation, false, lcd_spi, ILI9488_RESET_PIN, ILI9488_BAUD_RATE);
//   PicoGraphics_PenRGB888_direct graphics(ili9488.width, ili9488.height, ili9488);

//   ili9488.set_backlight(255);

//   struct pt {
//     float      x;
//     float      y;
//     uint8_t    r;
//     float     dx;
//     float     dy;
//     uint16_t pen;
//   };

//   Point text_location(0, 0);

//   Pen BG = graphics.create_pen(120, 40, 60);
//   Pen WHITE = graphics.create_pen(255, 255, 255);
//   // Pen RED = graphics.create_pen(255, 0, 0);
//   // Pen GREEN = graphics.create_pen(0, 255, 0);
//   // Pen BLUE = graphics.create_pen(0, 0, 255);
  
//   graphics.set_pen(BG);
//   graphics.clear();


//   //volatile uint uTook;//, uTook2, uTook3;

//   // for(;;)
//   // {
//   //   ElapsedUs elapsed;
//   //   graphics.set_pen(RED);
//   //   graphics.clear();
//   //   uTook = elapsed.elapsed(true);
//   //   graphics.set_pen(GREEN);
//   //   graphics.clear();
//   //   uTook2 = elapsed.elapsed(true);
//   //   graphics.set_pen(BLUE);
//   //   graphics.clear();
//   //   uTook3 = elapsed.elapsed(true);
    
//   //   printf("t=%u.%.2u", uTook/1000, (uTook - ((uTook/1000)*1000)) / 10);
//   //   printf("t=%u.%.2u", uTook2/1000, (uTook2 - ((uTook2/1000)*1000)) / 10);
//   //   printf("t=%u.%.2u", uTook3/1000, (uTook3 - ((uTook3/1000)*1000)) / 10);
//   // }

//   // for(;;)
//   // {
//   //   Rect rect = {0,0,3,1};
//   //   graphics.set_pen(rand() % 255, rand() % 255, rand() % 255);
//   //   ElapsedUs elapsed;
//   //   for(int r = 0; r < 1000; r++)
//   //   {
//   //     graphics.rectangle(rect);
//   //   }
//   //   uTook = elapsed.elapsed(true);
//   //   printf("t=%u.%.2u", uTook/1000, (uTook - ((uTook/1000)*1000)) / 10);
//   // }

//   while(true) {  
//     #if USE_TOUCHSCREEN
//       xpt2046.update();
//       if(xpt2046.is_touched()) {
//         text_location = xpt2046.get_point();

//         int32_t text_width = graphics.measure_text(msg);

//         text_location.x -= text_width/2;
//         text_location.y -= 8;
//       }
//     #endif

//     for(uint c =0; c < 1; c++) {
//       graphics.set_pen(rand() % 255, rand() % 255, rand() % 255);
//       graphics.circle(Point(rand() % graphics.bounds.w, rand() % graphics.bounds.h), (rand() % 10) + 3);
//     }

//     graphics.set_pen(WHITE);
//     graphics.text(msg, text_location, 320);
//   }

//   return 0;
// }
