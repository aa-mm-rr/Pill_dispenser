// src/leds.c
#include "leds.h"
#include "board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

static int led2pin(int idx){
    switch(idx){
        case 0: return PIN_LED0;
        case 1: return PIN_LED1;
        default: return PIN_LED2;
    }
}

void leds_init(void){
    for (int i=0;i<3;i++){
        int p = led2pin(i);
        gpio_init(p);
        gpio_set_dir(p,true);
        gpio_put(p,false);
    }
}

void led_on(int idx, bool on){
    gpio_put(led2pin(idx), on);
}

void led_blink(int idx, int times, int on_ms, int off_ms){
    int p = led2pin(idx);
    for (int i=0;i<times;i++){
        gpio_put(p,true);  sleep_ms(on_ms);
        gpio_put(p,false); sleep_ms(off_ms);
    }
}
