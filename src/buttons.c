// src/buttons.c
#include "buttons.h"
#include "board.h"
#include "hardware/gpio.h"

static int btn2pin(int idx){
    switch(idx){
        case 1: return PIN_BTN1;
        case 2: return PIN_BTN2;
        default: return PIN_BTN3;
    }
}

void buttons_init(void){
    for (int i=1;i<=4;i++){
        int p = btn2pin(i);
        gpio_init(p);
        gpio_set_dir(p,false);
        gpio_pull_up(p);
    }
}

bool button_read(int idx){
    int p = btn2pin(idx);
    return gpio_get(p) == 0; // active low
}
