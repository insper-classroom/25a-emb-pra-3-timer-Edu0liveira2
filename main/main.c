/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include <string.h>
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "hardware/timer.h"
 #include "hardware/rtc.h"
 #include "pico/util/datetime.h"
 
 const int echo = 14;
 const int trig = 15;
 volatile int start_flag = 1;
 volatile int timer_f = 0;
 
 volatile uint64_t start_time = 0; 
 volatile uint64_t pulse_duration = 0; 
 
 void echo_callback(uint gpio, uint32_t events) {
     if (events & GPIO_IRQ_EDGE_RISE) { 
         start_time = time_us_64();
     }
     else if (events & GPIO_IRQ_EDGE_FALL) { 
         pulse_duration = time_us_64() - start_time;
     }
 }
 
 int64_t alarm_callback(alarm_id_t id, void *user_data) {
     timer_f = 1;
     return 0; // Retorna 0 para não reagendar o alarme
 }
 
 int main() {
     stdio_init_all(); 
     sleep_ms(2000); 
 
     alarm_id_t alarm = add_alarm_in_ms(300, alarm_callback, NULL, false);
 
     gpio_init(trig);
     gpio_init(echo);
 
     gpio_set_dir(trig, GPIO_OUT);
     gpio_set_dir(echo, GPIO_IN);
 
     gpio_pull_down(echo); 
 
     gpio_set_irq_enabled_with_callback(echo, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_callback);
 
     datetime_t t = {
         .year  = 2025,
         .month = 3,
         .day   = 13,
         .dotw  = 3, // 0 é domingo, então 3 é quarta-feira
         .hour  = 11,
         .min   = 05,
         .sec   = 0
     };
 
     // Inicia o RTC
     rtc_init();
     rtc_set_datetime(&t);
 
     while (true) {
         int caracter = alarm;
 
         if (caracter != PICO_ERROR_TIMEOUT) { 
             if (caracter == 's') {
                 start_flag = 1;
                 printf("INICIANDO\n");
             } else if (caracter == 'p') {
                 start_flag = 0;
                 printf("PAUSADO\n");
             }
         }
 
         if (start_flag) {
             gpio_put(trig, 1);
             sleep_us(10);
             gpio_put(trig, 0);
 
             sleep_ms(50);
 
             if (timer_f == 1 || pulse_duration == 0) {
                 printf("FALHA\n");
                 timer_f = 0;
             } else if (pulse_duration > 0) {
                 float distance = (pulse_duration * 0.0343) / 2;
                 if (distance >= 300) {
                     printf("Muito longe\n");
                 } else {
                     printf("%d/%d/%d %d:%d:%d Distancia: %.2f cm\n",t.year, t.month, t.day, t.hour, t.min, t.sec, distance);
                 }
                 pulse_duration = 0; 
             }
         }
 
         sleep_ms(500);
     }
 }
 