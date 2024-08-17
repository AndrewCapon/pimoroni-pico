#include <string.h>
#include <math.h>
#include <vector>
#include <cstdlib>

#include "boards/pimoroni_pico_plus2_rp2350.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "drivers/ili9488/ili9488.hpp"
#include "drivers/xpt2046/xpt2046.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"

#include "rp2_psram.h"

using namespace pimoroni;

const char* msg = "Hello ILI9341";
Rotation rotation = ROTATE_270;

// reduce ILI9488_BAUD_RATE if you have problems.
// the system clock will also change and is 4 times this speed.
#define ILI9488_WIDTH (320)
#define ILI9488_HEIGHT (480)
#define ILI9488_BAUD_RATE (67'500'000)
#define CLOCK_NEEDED (ILI9488_BAUD_RATE * 4)

// if you do not have a reset pin wired up change to
// #define RESET_PIN PIN_UNUSED
#define ILI9488_RESET_PIN (21)


#define USE_TOUCHSCREEN 1
#if USE_TOUCHSCREEN
  #define XPT2046_WIDTH (240)
  #define XPT2046_HEIGHT (320)
  #define XPT2046_ROTATION_OFFSET (270)
  #define XPT2046_SCK (2)
  #define XPT2046_MOSI (3)
  #define XPT2046_MISO (0)
  #define XPT2046_IRQ (15)
  #define XPT2046_CS (14)
#endif
 

// // Dirty hack to overclock the Pico before class initialisation takes place
// class OC {
//     public:
//     OC(uint32_t freq_khz, vreg_voltage voltage) {
//         vreg_set_voltage(voltage);
//         sleep_us(100);
//         set_sys_clock_khz(freq_khz, true);
//     }
// };

// OC oc(CLOCK_NEEDED/1000, VREG_VOLTAGE_1_20);

SPIPins lcd_spi = {spi0, 20, 2, 3, PIN_UNUSED, 22, PIN_UNUSED };

int main() {
  // setup psram
  const int clock_hz = clock_get_hz(clk_sys);
 	size_t uRamSize = psram_init(PIMORONI_PICO_PLUS2_PSRAM_CS_PIN);

  int tx_pin = 12;
  int rx_pin = 13;

  stdio_uart_init_full(uart_default, PICO_DEFAULT_UART_BAUD_RATE, tx_pin, rx_pin);

  stdio_init_all();

  if(uRamSize == 0)
  {
    printf("Failed to allocate psram\n");
    return 0;
  }

  printf("Clock = %d, Spi = %u\n", clock_hz, ILI9488_BAUD_RATE);


  SPIPins touch_spi = {spi0, XPT2046_CS, XPT2046_SCK, XPT2046_MOSI, XPT2046_MISO, PIN_UNUSED, PIN_UNUSED};
  XPT2046	xpt2046(XPT2046_WIDTH, XPT2046_HEIGHT, (Rotation)((rotation+XPT2046_ROTATION_OFFSET)%360), touch_spi, XPT2046_IRQ, false, 500'000);

  ILI9488 ili9488(ILI9488_WIDTH, ILI9488_HEIGHT, rotation, false, lcd_spi, ILI9488_RESET_PIN, ILI9488_BAUD_RATE);
  PicoGraphics_PenRGB888 graphics(ili9488.width, ili9488.height, (void *)PSRAM_LOCATION);
  PicoGraphics_PenRGB888_direct dgraphics(ili9488.width, ili9488.height, ili9488);
  ili9488.set_backlight(255);

  struct pt {
    float      x;
    float      y;
    uint8_t    r;
    float     dx;
    float     dy;
    uint16_t pen;
  };

  std::vector<pt> shapes;
  for(int i = 0; i < 100; i++) {
    pt shape;
    shape.x = rand() % graphics.bounds.w;
    shape.y = rand() % graphics.bounds.h;
    shape.r = (rand() % 10) + 3;
    shape.dx = float(rand() % 255) / 64.0f;
    shape.dy = float(rand() % 255) / 64.0f;
    shape.pen = graphics.create_pen(rand() % 255, rand() % 255, rand() % 255);
    shapes.push_back(shape);
  }

  Point text_location(0, 0);

//  Pen BG = graphics.create_pen(120, 40, 60);
  Pen BG = graphics.create_pen(100, 100, 100);
  //Pen WHITE = graphics.create_pen(255, 255, 255);

  while(true) {  
    #if USE_TOUCHSCREEN
      xpt2046.update();
      if(xpt2046.is_touched()) {
        text_location = xpt2046.get_point();

        int32_t text_width = graphics.measure_text(msg);

        text_location.x -= text_width/2;
        text_location.y -= 8;
      }
    #endif

    graphics.set_pen(BG);
    graphics.clear();

    // for(auto &shape : shapes) {
    //   shape.x += shape.dx;
    //   shape.y += shape.dy;
    //   if((shape.x - shape.r) < 0) {
    //     shape.dx *= -1;
    //     shape.x = shape.r;
    //   }
    //   if((shape.x + shape.r) >= graphics.bounds.w) {
    //     shape.dx *= -1;
    //     shape.x = graphics.bounds.w - shape.r;
    //   }
    //   if((shape.y - shape.r) < 0) {
    //     shape.dy *= -1;
    //     shape.y = shape.r;
    //   }
    //   if((shape.y + shape.r) >= graphics.bounds.h) {
    //     shape.dy *= -1;
    //     shape.y = graphics.bounds.h - shape.r;
    //   }

    //   graphics.set_pen(shape.pen);
    //   graphics.circle(Point(shape.x, shape.y), shape.r);
    // }


    // graphics.set_pen(WHITE);
    // graphics.text(msg, text_location, 320);

    // update screen
    ili9488.update(&graphics);

    BG = graphics.create_pen(255, 0, 0);
    dgraphics.set_pen(BG);
    dgraphics.clear();

    BG = graphics.create_pen(0, 255, 0);
    dgraphics.set_pen(BG);
    dgraphics.clear();

    BG = graphics.create_pen(0, 0, 255);
    dgraphics.set_pen(BG);
    dgraphics.clear();
  }

  return 0;
}
