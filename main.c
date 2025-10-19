#include <stdint.h>
#include <stm32f10x.h>

volatile uint32_t current_delay = 100000;
volatile uint8_t button_a_pressed = 0;
volatile uint8_t button_c_pressed = 0;


void delay(uint32_t ticks) {
    for (uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}


uint8_t read_joystick_button(char button) {
    // Кнопка A - PB0
    // Кнопка C - PB2
    switch(button) {
        case 'A': return (GPIOB->IDR & GPIO_IDR_IDR0) == 0;  
        case 'C': return (GPIOB->IDR & GPIO_IDR_IDR2) == 0; 
        default: return 0;
    }
}


void change_frequency(char button) {
    if (button == 'A' && !button_a_pressed) {
        // Увеличиваем частоту в 2 раза (уменьшаем задержку)
        if (current_delay > 100) {  // Минимальная задержка
            current_delay /= 2;
            button_a_pressed = 1;  // Больше не реагируем на кнопку A
        }
    }
    else if (button == 'C' && !button_c_pressed) {
    
        if (current_delay < 6400000) {  
            current_delay *= 2;
            button_c_pressed = 1;  
        }
    }
}


int main(void) {

    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;  // Для светодиода (PC13)
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;  // Для кнопок джойстика (PB0, PB2)
    
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_0;
    
    // Настраиваем PB0 и PB2 как входы для кнопок
    // CNF = 01 (Input floating), MODE = 00 (Input)
    GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);  // PB0
    GPIOB->CRL |= GPIO_CRL_CNF0_0;
    
    GPIOB->CRL &= ~(GPIO_CRL_CNF2 | GPIO_CRL_MODE2);  // PB2  
    GPIOB->CRL |= GPIO_CRL_CNF2_0; 

    while (1) {
        if (read_joystick_button('A')) {
            change_frequency('A');
            while (read_joystick_button('A')) {
                delay(1000);
            }
        }
        
        if (read_joystick_button('C')) {
            change_frequency('C');
            while (read_joystick_button('C')) {
                delay(1000);
            }
        }
        
        GPIOC->ODR |= (1U << 13U);  
        delay(current_delay);
        GPIOC->ODR &= ~(1U << 13U); 
        delay(current_delay);
    }
}