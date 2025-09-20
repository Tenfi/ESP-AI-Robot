#include <stdio.h>
#include "led_indicator.h"
#include "light_control.h"

static const char *TAG = "led_strips";
static led_indicator_handle_t led_handle = NULL;

static const blink_step_t charging_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0, 0, 0), 0},
    {LED_BLINK_HSV, SET_IHSV(1, 0,255,0), 0},
    {LED_BLINK_BREATHE, INSERT_INDEX(1,LED_STATE_50_PERCENT), 200},
    {LED_BLINK_BREATHE,  INSERT_INDEX(1,LED_STATE_OFF), 500},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t battery_power_low_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0, 0, 0), 50},
    {LED_BLINK_HSV, SET_IHSV(1, 0,255,LED_STATE_25_PERCENT), 100},
    {LED_BLINK_HSV, SET_IHSV(1, 0,255,LED_STATE_OFF), 50},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t white_always_on_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0, 0, 127), 0},
    {LED_BLINK_STOP, 0, 0},
};

static const blink_step_t white_fast_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,127), 200},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 200},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t breath_white_slow[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,255), 0},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 1000},
    {LED_BLINK_BREATHE,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 1000},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 1000},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t breath_white_fast[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,255), 0},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 300},
    {LED_BLINK_BREATHE,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 200},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 300},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t flowing_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0, MAX_SATURATION, MAX_BRIGHTNESS), 0},
    {LED_BLINK_HSV_RING, SET_IHSV(MAX_INDEX, MAX_HUE, MAX_SATURATION, MAX_BRIGHTNESS), 2000},
    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t custom_light_show[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,0), 50},
    {LED_BLINK_HSV, SET_IHSV(0, 0,0,200), 60},
    {LED_BLINK_HSV, SET_IHSV(3, 0,0,200), 60},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 20},
    {LED_BLINK_HSV, SET_IHSV(1, 0,0,200), 60},
    {LED_BLINK_HSV, SET_IHSV(4, 0,0,200), 60},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 20},
    {LED_BLINK_HSV, SET_IHSV(2, 0,0,200), 60},
    {LED_BLINK_HSV, SET_IHSV(5, 0,0,200), 60},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 20},
    {LED_BLINK_HSV, SET_IHSV(2, 0,0,200), 60},
    {LED_BLINK_HSV, SET_IHSV(5, 0,0,200), 60},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 20},
    {LED_BLINK_HSV, SET_IHSV(1, 0,0,200), 60},
    {LED_BLINK_HSV, SET_IHSV(4, 0,0,200), 60},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 20},
    {LED_BLINK_HSV, SET_IHSV(0, 0,0,200), 60},
    {LED_BLINK_HSV, SET_IHSV(3, 0,0,200), 60},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 20},

    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 300},
    {LED_BLINK_BREATHE,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 200},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 300},
    {LED_BLINK_BREATHE,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 200},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 300},
    {LED_BLINK_BREATHE,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 200},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX,LED_STATE_ON), 300},
    {LED_BLINK_BREATHE,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 200},

    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,150), 600},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 100},
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,150), 600},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 100},
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,150), 600},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 100},

    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0, MAX_SATURATION, MAX_BRIGHTNESS), 0},
    {LED_BLINK_HSV_RING, SET_IHSV(MAX_INDEX, MAX_HUE, MAX_SATURATION, MAX_BRIGHTNESS), 5000},

    {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t light_sleep_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0,0,127), 200},
    {LED_BLINK_BRIGHTNESS,  INSERT_INDEX(MAX_INDEX,LED_STATE_OFF), 5000},

    {LED_BLINK_LOOP, 0, 0},
};

blink_step_t const *led_mode[] = {
    [LIGHT_MODE_CHARGING_BREATH] = charging_blink,
    [LIGHT_MODE_POWER_LOW] = battery_power_low_blink,
    [LIGHT_MODE_ALWAYS_ON] = white_always_on_blink,
    [LIGHT_MODE_BLINK] = white_fast_blink,
    [LIGHT_MODE_WHITE_BREATH_SLOW] = breath_white_slow,
    [LIGHT_MODE_WHITE_BREATH_FAST] = breath_white_fast,
    [LIGHT_MODE_FLOWING] = flowing_blink,
    [LIGHT_MODE_SHOW] = custom_light_show,
    [LIGHT_MODE_SLEEP] = light_sleep_blink,
    [LIGHT_MODE_MAX] = NULL,
};

void rgb_ws2812_init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO_NUM,              // The GPIO that connected to the LED strip's data line
        .max_leds = WS2812_STRIPS_NUM,                  // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,       // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,                  // LED strip model
        .flags.invert_out = false,                      // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .spi_bus = SPI2_HOST, // RMT counter clock frequency
        .flags.with_dma = true,               // DMA feature is available on ESP target like ESP32-S3
    };

    led_indicator_strips_config_t strips_config = {
        .led_strip_cfg = strip_config,
        .led_strip_driver = LED_STRIP_SPI,
        .led_strip_spi_cfg = spi_config,
    };

    const led_indicator_config_t config = {
        .mode = LED_STRIPS_MODE,
        .led_indicator_strips_config = &strips_config,
        .blink_lists = led_mode,
        .blink_list_num = LIGHT_MODE_MAX,
    };

    led_handle = led_indicator_create(&config);
    assert(led_handle != NULL);
}

void rgb_ws2812_mode_set(light_mode_t mode)
{
    led_indicator_preempt_start(led_handle, mode);
}
