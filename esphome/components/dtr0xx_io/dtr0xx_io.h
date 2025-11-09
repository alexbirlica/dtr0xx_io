#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/components/spi/spi.h"

#include <vector>

namespace esphome {
namespace dtr0xx_io {

class dtr0xx_ioComponent : public PollingComponent,
                           public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                                  spi::CLOCK_POLARITY_LOW,
                                                  spi::CLOCK_PHASE_LEADING,
                                                  spi::DATA_RATE_4MHZ> {
 public:
  dtr0xx_ioComponent() = default;

  void setup() override;
  void update() override;
  float get_setup_priority() const override;
  void dump_config() override;

  void set_dingtian_pl_pin(GPIOPin *pin) { this->dingtian_pl_pin_ = pin; }
  
  void set_sr_count(uint8_t count) {
    this->sr_count_ = count;
    this->input_bytes_.resize(count);
    this->output_bytes_.resize(count);
  }

  void set_dingtian_v2(bool dingtian_v2)
  {
    this->dingtian_v2_ = dingtian_v2;
  }

 protected:
  friend class dtr0xx_ioGPIOPin;
  bool digital_read_(uint16_t pin);
  void digital_write_(uint16_t pin, bool value);
  void transfer_gpio_();
  void write_gpio_();

  GPIOPin *dingtian_pl_pin_;
  uint8_t sr_count_;
  std::vector<uint8_t> input_bytes_;
  std::vector<uint8_t> output_bytes_;
  bool dingtian_v2_;
};

/// Helper class to expose a SN74HC165 pin as an internal input GPIO pin.
class dtr0xx_ioGPIOPin : public GPIOPin, public Parented<dtr0xx_ioComponent> {
 public:
  void setup() override {}
  void pin_mode(gpio::Flags flags) override {}
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_pin(uint16_t pin) { pin_ = pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }

  void set_flags(gpio::Flags flags) { this->flags_ = flags; }
  gpio::Flags get_flags() const override { return this->flags_; }

 protected:
  uint16_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};

}  // namespace dtr0xx_io
}  // namespace esphome
