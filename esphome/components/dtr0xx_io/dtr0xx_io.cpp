#include "dtr0xx_io.h"
#include "esphome/core/log.h"
#include "freertos/task.h"
#include "soc/interrupts.h"

namespace esphome {
namespace dtr0xx_io {

static portMUX_TYPE readGpioMux = portMUX_INITIALIZER_UNLOCKED;

static const char *const TAG = "dtr0xx_io";

void dtr0xx_ioComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up dtr0xx_io...");

  // initialize pins
  this->dingtian_pl_pin_->setup();
  this->spi_setup();

  // Set OE to false, PL to true before first read to not flicker the relays
  this->dingtian_pl_pin_->digital_write(true);

  // read state from shift register
  this->transfer_gpio_();
}

void dtr0xx_ioComponent::update() {
  this->transfer_gpio_();
}

void dtr0xx_ioComponent::dump_config() { ESP_LOGCONFIG(TAG, "dtr0xx_io:"); }

bool dtr0xx_ioComponent::digital_read_(uint16_t pin) {
  if (pin >= this->sr_count_ * 8) {
    ESP_LOGE(TAG, "Pin %u is out of range! Maximum pin number with %u chips in series is %u", pin, this->sr_count_,
             (this->sr_count_ * 8) - 1);
    return false;
  }

  return (bool)(this->input_bytes_[0] & (1 << (7-pin)));
}

void dtr0xx_ioComponent::digital_write_(uint16_t pin, bool value)
{
    if (pin >= this->sr_count_ * 8) {
    ESP_LOGE(TAG, "Pin %u is out of range! Maximum pin number with %u chips in series is %u", pin, this->sr_count_,
             (this->sr_count_ * 8) - 1);
    return;
  }

  if (value) {
    this->output_bytes_[0] |= (1 << (7-pin));
  } else {
    this->output_bytes_[0] &= ~(1 << (7-pin));
  }

  this->transfer_gpio_();
}

void dtr0xx_ioComponent::transfer_gpio_() {
  portDISABLE_INTERRUPTS();
  // enter critical area to not disturb the timming
  taskENTER_CRITICAL(&readGpioMux);
  vTaskSuspendAll();  // Stop task switching

  this->enable();

  // if V1 HW
  if ( false == dingtian_v2_ ) {
    // PL needs to be true during reading
    this->dingtian_pl_pin_->digital_write(true);
  }

  for (uint8_t i = 0; i < this->sr_count_; i++) {
    // transfer the data
    input_bytes_[i] = this->transfer_byte(output_bytes_[i]);
  }

  // if V1 HW
  if ( false == dingtian_v2_ ) {
    // PL needs to be fasle during output latching, and always for the relays to work!!
    this->dingtian_pl_pin_->digital_write(false);
  }

  this->disable();

  xTaskResumeAll();   // Resume task switching
  // exit critical area
  taskEXIT_CRITICAL(&readGpioMux);
  portENABLE_INTERRUPTS();
}

float dtr0xx_ioComponent::get_setup_priority() const { return setup_priority::IO; }

bool dtr0xx_ioGPIOPin::digital_read() { 
  return this->parent_->digital_read_(this->pin_) != this->inverted_; }

void dtr0xx_ioGPIOPin::digital_write(bool value) {
  this->parent_->digital_write_(this->pin_, value != this->inverted_);}

std::string dtr0xx_ioGPIOPin::dump_summary() const { return str_snprintf("%u via dtr0xx_io", 18, pin_); }

}  // namespace dtr0xx_io
}  // namespace esphome
