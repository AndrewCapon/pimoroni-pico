#include <string.h>
#include <math.h>
#include <vector>
#include <cstdlib>


#include "drivers/ili9488/ili9488.hpp"
#include "drivers/xpt2046/xpt2046.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "hardware/vreg.h"

//#include "elapsed_us.hpp"

using namespace pimoroni;

const char* msg = "Hello ILI9488";

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
 

// Dirty hack to overclock the Pico before class initialisation takes place
class OC {
    public:
    OC(uint32_t freq_khz, vreg_voltage voltage) {
        vreg_set_voltage(voltage);
        sleep_us(100);
        set_sys_clock_khz(freq_khz, true);
    }
};

OC oc(CLOCK_NEEDED/1000, VREG_VOLTAGE_1_20);


class ElapsedUs
{
public:
	ElapsedUs()
	{
		last_time = time_us_64();
	}

	float elapsed(void)
	{
		uint64_t time_now = time_us_64();
		uint64_t elapsed = time_now - last_time;
		last_time = time_now;
		return (float)elapsed/1000.0f;
	}

private:
	uint64_t last_time;
};


SPIPins lcd_spi = {spi0, 20, 2, 3, PIN_UNUSED, 22, PIN_UNUSED };

int main() {
  int tx_pin = 12;
  int rx_pin = 13;

  stdio_uart_init_full(uart_default, PICO_DEFAULT_UART_BAUD_RATE, tx_pin, rx_pin);

  stdio_init_all();

  for(int i=0; i < 10; i++)
    printf("hello %d\n", i);

  SPIPins touch_spi = {spi0, XPT2046_CS, XPT2046_SCK, XPT2046_MOSI, XPT2046_MISO, PIN_UNUSED, PIN_UNUSED};
  XPT2046	xpt2046(XPT2046_WIDTH, XPT2046_HEIGHT, (Rotation)((rotation+XPT2046_ROTATION_OFFSET)%360), touch_spi, XPT2046_IRQ, false, 500'000);

  ILI9488 ili9488(ILI9488_WIDTH, ILI9488_HEIGHT, rotation, false, lcd_spi, ILI9488_RESET_PIN, ILI9488_BAUD_RATE);
  PicoGraphics_PenRGB888_direct graphics(ili9488.width, ili9488.height, ili9488);

  ili9488.set_backlight(255);

  struct pt {
    float      x;
    float      y;
    uint8_t    r;
    float     dx;
    float     dy;
    uint16_t pen;
  };

  Point text_location(0, 0);

  Pen BG = graphics.create_pen(120, 40, 60);
  Pen WHITE = graphics.create_pen(255, 255, 255);
  
  ElapsedUs timer;
  graphics.set_pen(BG);
  graphics.clear();
  printf("Clear took %fms\n", timer.elapsed());

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

    for(uint c =0; c < 1; c++) {
      graphics.set_pen(rand() % 255, rand() % 255, rand() % 255);
      graphics.circle(Point(rand() % graphics.bounds.w, rand() % graphics.bounds.h), (rand() % 10) + 3);
    }

    graphics.set_pen(WHITE);
    graphics.text(msg, text_location, 320);
  }

  return 0;
}
