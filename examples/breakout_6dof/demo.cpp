#include "pico/stdlib.h"

#include "breakout_6dof.hpp"

using namespace pimoroni;

Breakout6DOF dof;

int main() {
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  dof.init();   

  while(true) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    sleep_ms(1000);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    sleep_ms(1000);
  }

  return 0;
}
