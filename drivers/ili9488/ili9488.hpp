#pragma once

#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "common/pimoroni_common.hpp"
#include "common/pimoroni_bus.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"



#include <algorithm>


namespace pimoroni {

  
  class ILI9488 : public DisplayDriver, public IDirectDisplayDriver<uint32_t> {
    spi_inst_t *spi = PIMORONI_SPI_DEFAULT_INSTANCE;
  
  public:
    bool round;

    //--------------------------------------------------
    // Variables
    //--------------------------------------------------
  private:
    // interface pins with our standard defaults where appropriate
    uint cs;
    uint dc;
    uint wr_sck;
    uint rd_sck = PIN_UNUSED;
    uint d0;
    uint bl;
    uint vsync  = PIN_UNUSED; // only available on some products
    uint rst;
    uint st_dma_data;

    // current update rect and full screen rect
    Rect								full_screen_region;
    Rect								current_update_region;

    // dma control blocks used for async partial updates
    struct DMAControlBlock
    {
      uint32_t len; 
      uint8_t* data;
    };

    uint 								st_dma_control_chain;
    DMAControlBlock* 		dma_control_chain_blocks = nullptr;
    dma_channel_config 	dma_data_config;
    dma_channel_config  dma_control_config;
    bool 								use_async_dma = false;
    bool								dma_control_chain_is_enabled = false;

    // sanity flag for dma updates
    bool								in_dma_update = false;


  public:
    ILI9488(uint16_t width, uint16_t height, Rotation rotation, bool round, SPIPins pins, uint reset_pin = PIN_UNUSED, uint baud_rate = 62500000, bool use_async_dma = false) :
      DisplayDriver(width, height, rotation),
      spi(pins.spi), round(round),
      cs(pins.cs), dc(pins.dc), wr_sck(pins.sck), d0(pins.mosi), bl(pins.bl), rst(reset_pin), use_async_dma(use_async_dma) {

      // configure spi interface and pins
      uint baud = spi_init(spi, baud_rate);
      printf("BAUD = %u\n", baud);

      gpio_set_function(pins.sck, GPIO_FUNC_SPI);
      gpio_set_function(pins.mosi, GPIO_FUNC_SPI);
      gpio_set_function(pins.miso, GPIO_FUNC_SPI);

      st_dma_data = dma_claim_unused_channel(true);
      dma_data_config = dma_channel_get_default_config(st_dma_data);
      channel_config_set_transfer_data_size(&dma_data_config, DMA_SIZE_8);
      channel_config_set_bswap(&dma_data_config, false);
      channel_config_set_dreq(&dma_data_config, spi_get_dreq(spi, true));
      dma_channel_configure(st_dma_data, &dma_data_config, &spi_get_hw(spi)->dr, NULL, 0, false);

      setup_dma_control_if_needed();

      common_init();
    }
  
    virtual ~ILI9488()
    {
      cleanup();
    }

    void cleanup() override;
    void update(PicoGraphics *graphics) override;
    void partial_update(PicoGraphics *display, Rect region) override;
    void set_backlight(uint8_t brightness) override;

    bool is_busy() override
    {
      if(use_async_dma && dma_control_chain_is_enabled) {
        return !(dma_hw->intr & 1u << st_dma_data);
      }
      else {
        return dma_channel_is_busy(st_dma_data);
      }
    }

    bool is_using_async_dma() {
      return use_async_dma;
    }

    void wait_for_update_to_finish()
    {
      if(use_async_dma && dma_control_chain_is_enabled) {
        while (!(dma_hw->intr & 1u << st_dma_data)) {
          tight_loop_contents();
        }

        // disable control chain dma
        enable_dma_control(false);
      }
      else {
        dma_channel_wait_for_finish_blocking(st_dma_data);
      }

      // deselect 
      gpio_put(cs, 1);

      // set sanity flag
      in_dma_update = false;
    }

    int __not_in_flash_func(SpiSetBlocking)(const uint32_t uSrc, size_t uLen) 
	  {
      // Use 16 bit writes if possible
      uint16_t buffer[6];
      buffer[0] = buffer[3] = uSrc >> 8;
      buffer[1] = buffer[4] = ((uSrc & 0xff) << 8) | ((uSrc & 0xff0000) >> 16);
      buffer[2] = buffer[5] = uSrc & 0xffff;
      
      // send 16 bit values
      spi_set_format(spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
      size_t uLen16 = (uLen/2)*3;
      //uint16_t uSrc16 = uSrc;

      // Deliberately overflow FIFO, then clean up afterward, to minimise amount
      // of APB polling required per halfword
      uint_fast8_t w = 0;
      for (size_t i = 0; i < uLen16; ++i) {
          while (!spi_is_writable(spi))
              tight_loop_contents();
          spi_get_hw(spi)->dr = buffer[w];
          if(++w==3)
            w = 0;
      }

      // finish up 16 bit transfers
      while (spi_is_readable(spi))
          (void)spi_get_hw(spi)->dr;
      while (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS)
          tight_loop_contents();
      while (spi_is_readable(spi))
          (void)spi_get_hw(spi)->dr;

      // Don't leave overrun flag set
      spi_get_hw(spi)->icr = SPI_SSPICR_RORIC_BITS;


      // set spi back to 8 bits
      spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

      // handle remaining 3 bytes as 8 bit spi is needed
      if(uLen & 1)
      {
        volatile uint8_t *p = 2+(uint8_t *)&uSrc;
        for (size_t b = 0; b < 3; ++b) {
          while (!spi_is_writable(spi))
              tight_loop_contents();
          spi_get_hw(spi)->dr = *p--;
        }

        // finish up 8 bit transfers
        while (spi_is_readable(spi))
            (void)spi_get_hw(spi)->dr;
        while (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS)
            tight_loop_contents();
        while (spi_is_readable(spi))
            (void)spi_get_hw(spi)->dr;

        // Don't leave overrun flag set
        spi_get_hw(spi)->icr = SPI_SSPICR_RORIC_BITS;
      }

			return (int)uLen;
	  }

    void write_pixel(const Point &p, uint32_t colour) override {
      set_addr_window(p.x, p.y, 1, 1);
      gpio_put(cs, 0);
      gpio_put(dc, 1); 
      SpiSetBlocking(colour, 1);
      gpio_put(cs, 1);
    }

    void write_pixel_span(const Point &p, uint l, uint32_t colour) override {
      set_addr_window(p.x, p.y, l, 1);
      gpio_put(cs, 0);
      gpio_put(dc, 1); 
      SpiSetBlocking(colour, l);
      gpio_put(cs, 1);
    }

    void write_pixel_rect(const Rect &r, uint32_t colour) override {
      set_addr_window(r.x, r.y, r.w, r.h);
      gpio_put(cs, 0);
      gpio_put(dc, 1); 
      SpiSetBlocking(colour, r.w * r.h);
      gpio_put(cs, 1);
    }

    uint8_t SPI4W_Write_Byte(uint8_t value) {
	    spi_write_blocking(spi, &value, 1);
      return 0;
    }

    void LCD_WriteReg(uint8_t reg) {
      gpio_put(dc, 0); 
      gpio_put(cs, 0);

    	SPI4W_Write_Byte(reg);

      gpio_put(cs, 1);
    }

    void LCD_WriteData(uint16_t data) {
      gpio_put(dc, 1); 
      gpio_put(cs, 0);

    	//SPI4W_Write_Byte(data >> 8);
		  SPI4W_Write_Byte(data & 0XFF);

      gpio_put(cs, 1);
    }

  private:
    void common_init();
    void configure_display(Rotation rotate);
    void write_blocking_dma(const uint8_t *src, size_t len);
    void command(uint8_t command, size_t len = 0, const char *data = NULL, bool bDataDma = false);
    void setup_dma_control_if_needed();
    void enable_dma_control(bool enable);
    void start_dma_control();
    bool set_update_region(Rect& update_rect);
    void set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void reset();

  };
}
