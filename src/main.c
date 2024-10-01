#include <lvgl.h>
#include <lvgl_input_device.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "logo_image.h"

#define BASIC_TEST

#define DISPLAY_BUFFER_PITCH 128

LOG_MODULE_REGISTER(display);

#ifdef BASIC_TEST
static uint32_t count;

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static struct gpio_dt_spec btn0_gpio = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback btn0_callback;

static void btn01_isr_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    count = 0;
}

static void lv_btn_click_callback(lv_event_t *e)
{
    ARG_UNUSED(e);

    count = 0;
}

int main(void)
{
    if (display == NULL)
    {
        LOG_ERR("device pointer is NULL");
        return 0;
    }

    if (!device_is_ready(display))
    {
        LOG_ERR("display device is not ready");
        return 0;
    }

    if (gpio_is_ready_dt(&btn0_gpio))
    {
        int err;

        err = gpio_pin_configure_dt(&btn0_gpio, GPIO_INPUT);
        if (err)
        {
            LOG_ERR("failed to configure button gpio: %d", err);
            return 0;
        }

        gpio_init_callback(&btn0_callback, btn01_isr_callback, BIT(btn0_gpio.pin));

        err = gpio_add_callback(btn0_gpio.port, &btn0_callback);
        if (err)
        {
            LOG_ERR("failed to add button callback: %d", err);
            return 0;
        }

        err = gpio_pin_interrupt_configure_dt(&btn0_gpio, GPIO_INT_EDGE_TO_ACTIVE);
        if (err)
        {
            LOG_ERR("failed to enable button callback: %d", err);
            return 0;
        }
    }

    struct display_capabilities capabilities;
    display_get_capabilities(display, &capabilities);

    const uint16_t x_res = capabilities.x_resolution;
    const uint16_t y_res = capabilities.y_resolution;

    LOG_INF("x_resolution: %d", x_res);
    LOG_INF("y_resolution: %d", y_res);
    LOG_INF("supported pixel formats: %d", capabilities.supported_pixel_formats);
    LOG_INF("screen_info: %d", capabilities.screen_info);
    LOG_INF("current_pixel_format: %d", capabilities.current_pixel_format);
    LOG_INF("current_orientation: %d", capabilities.current_orientation);

    display_blanking_off(display);

    const struct display_buffer_descriptor buf_desc = {
        .width = x_res, .height = y_res, .buf_size = x_res * y_res, .pitch = DISPLAY_BUFFER_PITCH};

    if (display_write(display, 0, 0, &buf_desc, logo_buf) != 0)
    {
        LOG_ERR("could not write to display");
    }

    if (display_set_contrast(display, 0) != 0)
    {
        LOG_ERR("could not set display contrast");
    }

    const size_t ms_sleep = 5;

    // Increase brightness
    for (size_t i = 0; i < 255; i++)
    {
        display_set_contrast(display, i);
        k_sleep(K_MSEC(ms_sleep));
    }

    // Decrease brightness
    for (size_t i = 255; i > 0; i--)
    {
        display_set_contrast(display, i);
        k_sleep(K_MSEC(ms_sleep));
    }

    display_blanking_on(display);

    char count_str[11] = {0};
    lv_obj_t *hello_world_label;
    lv_obj_t *count_label;

    if (IS_ENABLED(CONFIG_LV_Z_POINTER_INPUT))
    {
        lv_obj_t *hello_world_button;

        hello_world_button = lv_btn_create(lv_scr_act());
        lv_obj_align(hello_world_button, LV_ALIGN_CENTER, 0, -15);
        lv_obj_add_event_cb(hello_world_button, lv_btn_click_callback, LV_EVENT_CLICKED, NULL);
        hello_world_label = lv_label_create(hello_world_button);
    }
    else
    {
        hello_world_label = lv_label_create(lv_scr_act());
    }

    lv_label_set_text(hello_world_label, "Hello world!");
    lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

    count_label = lv_label_create(lv_scr_act());
    lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_task_handler();
    display_blanking_off(display);

    while (true)
    {
        if ((count % 100) == 0U)
        {
            sprintf(count_str, "%d", count / 100U);
            lv_label_set_text(count_label, count_str);
        }
        lv_task_handler();
        ++count;
        k_sleep(K_MSEC(10));
    }
}

#else

#endif