idf_component_register(
    SRCS
        "main.c"
        "audio_capture.c"
        "battery_monitor.c"
        "ble_manager.c"
        "button_handler.c"
        "haptic.c"
        "led_controller.c"
        "power_manager.c"
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash driver esp_pm esp_adc esp_event
    PRIV_REQUIRES led_strip_encoder
)