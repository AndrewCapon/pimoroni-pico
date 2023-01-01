add_executable(
  ili9488_demo_direct
  ili9488_demo_direct.cpp
)

# Pull in pico libraries that we need
target_link_libraries(ili9488_demo_direct pico_stdlib hardware_spi hardware_pwm hardware_dma rgbled pico_display pico_graphics ili9488 xpt2046)

# create map/bin/hex file etc.
pico_add_extra_outputs(ili9488_demo_direct)
