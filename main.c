#include <stdint.h>
#include <stm32f10x.h>

// ==================== SPI БИБЛИОТЕКА ====================

/**
 * Инициализация SPI1 в режиме Master
 * Настройка пинов:
 * PA5 - SCK (Serial Clock)
 * PA6 - MISO (Master In Slave Out) 
 * PA7 - MOSI (Master Out Slave In)
 */
void SPI1_Init(void) {
    // 1. Включаем тактирование SPI1 и порта A
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    
    // 2. Настраиваем пины SPI1 на порту A
    
    // PA5 (SCK) - Alternate Function Push-Pull, 10MHz
    GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
    GPIOA->CRL |= (GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5_1 | GPIO_CRL_MODE5_0);
    
    // PA6 (MISO) - Input floating
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOA->CRL |= GPIO_CRL_CNF6_0;
    
    // PA7 (MOSI) - Alternate Function Push-Pull, 10MHz
    GPIOA->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOA->CRL |= (GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7_1 | GPIO_CRL_MODE7_0);
    
    // 3. Конфигурация регистра SPI1_CR1
    SPI1->CR1 = SPI_CR1_MSTR |       // Режим Master
                SPI_CR1_BR_0 |       // Baud rate: Fpclk/4
                SPI_CR1_SSM |        // Software slave management
                SPI_CR1_SSI;         // Internal slave select
    
    // Дополнительные настройки:
    SPI1->CR1 &= ~SPI_CR1_CPOL;      // CPOL = 0 (полярность)
    SPI1->CR1 &= ~SPI_CR1_CPHA;      // CPHA = 0 (фаза)
    SPI1->CR1 &= ~SPI_CR1_DFF;       // 8-bit data format
    SPI1->CR1 &= ~SPI_CR1_LSBFIRST;  // MSB first
    
    // 4. Включаем SPI1
    SPI1->CR1 |= SPI_CR1_SPE;
}

/**
 * Запись одного байта через SPI
 * @param data - байт для отправки
 */
void SPI1_Write(uint8_t data) {
    // Ждем, пока буфер передачи не станет пустым
    while (!(SPI1->SR & SPI_SR_TXE));
    
    // Записываем данные в регистр данных
    SPI1->DR = data;
    
    // Ждем завершения передачи
    while (SPI1->SR & SPI_SR_BSY);
}

/**
 * Чтение одного байта через SPI
 * @return прочитанный байт
 */
uint8_t SPI1_Read(void) {
    // Отправляем "пустой" байт для генерации тактовых импульсов
    SPI1->DR = 0xFF;
    
    // Ждем, пока данные не будут получены
    while (!(SPI1->SR & SPI_SR_RXNE));
    
    // Возвращаем полученные данные
    return SPI1->DR;
}

// ==================== ДИСПЛЕЙ SSD1306 ====================

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64

// Буфер дисплея (1024 байта для 128x64)
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Управляющие пины для дисплея
#define SSD1306_RST_PIN   GPIO_BSRR_BR0  // PA0 - Reset
#define SSD1306_DC_PIN    GPIO_BSRR_BR1  // PA1 - Data/Command
#define SSD1306_CS_PIN    GPIO_BSRR_BR2  // PA2 - Chip Select

// Команды SSD1306
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22

// Функция задержки
void delay(uint32_t ticks) {
    for (uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

/**
 * Отправка команды на дисплей
 * @param cmd - команда для отправки
 */
void SSD1306_WriteCommand(uint8_t cmd) {
    GPIOA->BSRR = SSD1306_DC_PIN;     // DC = 0 (command mode)
    GPIOA->BSRR = SSD1306_CS_PIN;     // CS = 0 (select display)
    
    SPI1_Write(cmd);                  // Отправляем команду
    
    GPIOA->BSRR = SSD1306_CS_PIN << 16; // CS = 1 (deselect display)
}

/**
 * Отправка данных на дисплей
 * @param data - данные для отправки
 */
void SSD1306_WriteData(uint8_t data) {
    GPIOA->BSRR = SSD1306_DC_PIN << 16; // DC = 1 (data mode)
    GPIOA->BSRR = SSD1306_CS_PIN;       // CS = 0 (select display)
    
    SPI1_Write(data);                  // Отправляем данные
    
    GPIOA->BSRR = SSD1306_CS_PIN << 16; // CS = 1 (deselect display)
}

/**
 * Инициализация дисплея SSD1306
 */
void SSD1306_Init(void) {
    // Настройка управляющих пинов дисплея
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0 |
                    GPIO_CRL_MODE1 | GPIO_CRL_CNF1 |
                    GPIO_CRL_MODE2 | GPIO_CRL_CNF2);
    GPIOA->CRL |= (GPIO_CRL_MODE0_0 |  // PA0: Output, 2MHz (RST)
                   GPIO_CRL_MODE1_0 |  // PA1: Output, 2MHz (DC)
                   GPIO_CRL_MODE2_0);  // PA2: Output, 2MHz (CS)
    
    // Устанавливаем начальные состояния
    GPIOA->BSRR = SSD1306_RST_PIN << 16; // RST = 1
    GPIOA->BSRR = SSD1306_CS_PIN << 16;  // CS = 1
    
    // Инициализация SPI
    SPI1_Init();
    
    // Процедура сброса дисплея
    GPIOA->BSRR = SSD1306_RST_PIN;      // RST = 0 (активный сброс)
    delay(100000);                      // Задержка 10ms
    GPIOA->BSRR = SSD1306_RST_PIN << 16; // RST = 1 (завершить сброс)
    delay(100000);                      // Задержка 10ms
    
    // Последовательность инициализации дисплея
    SSD1306_WriteCommand(SSD1306_DISPLAYOFF);
    SSD1306_WriteCommand(SSD1306_SETDISPLAYCLOCKDIV);
    SSD1306_WriteCommand(0x80);
    SSD1306_WriteCommand(SSD1306_SETMULTIPLEX);
    SSD1306_WriteCommand(0x3F);
    SSD1306_WriteCommand(SSD1306_SETDISPLAYOFFSET);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(SSD1306_SETSTARTLINE | 0x00);
    SSD1306_WriteCommand(SSD1306_CHARGEPUMP);
    SSD1306_WriteCommand(0x14);
    SSD1306_WriteCommand(SSD1306_MEMORYMODE);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(SSD1306_SEGREMAP | 0x01);
    SSD1306_WriteCommand(SSD1306_COMSCANDEC);
    SSD1306_WriteCommand(SSD1306_SETCOMPINS);
    SSD1306_WriteCommand(0x12);
    SSD1306_WriteCommand(SSD1306_SETCONTRAST);
    SSD1306_WriteCommand(0xCF);
    SSD1306_WriteCommand(SSD1306_SETPRECHARGE);
    SSD1306_WriteCommand(0xF1);
    SSD1306_WriteCommand(SSD1306_SETVCOMDETECT);
    SSD1306_WriteCommand(0x40);
    SSD1306_WriteCommand(SSD1306_DISPLAYALLON_RESUME);
    SSD1306_WriteCommand(SSD1306_NORMALDISPLAY);
    SSD1306_WriteCommand(SSD1306_DISPLAYON);
    
    // Очищаем буфер дисплея
    for (int i = 0; i < sizeof(ssd1306_buffer); i++) {
        ssd1306_buffer[i] = 0x00;
    }
}

/**
 * Обновление дисплея (отправка всего буфера на дисплей)
 */
void SSD1306_Update(void) {
    // Устанавливаем область отображения (весь экран)
    SSD1306_WriteCommand(SSD1306_COLUMNADDR);
    SSD1306_WriteCommand(0);   // Начальный столбец
    SSD1306_WriteCommand(127); // Конечный столбец
    
    SSD1306_WriteCommand(SSD1306_PAGEADDR);
    SSD1306_WriteCommand(0);   // Начальная страница
    SSD1306_WriteCommand(7);   // Конечная страница
    
    // Отправляем весь буфер на дисплей
    for (int i = 0; i < sizeof(ssd1306_buffer); i++) {
        SSD1306_WriteData(ssd1306_buffer[i]);
    }
}

/**
 * Рисование пикселя в буфере дисплея
 * @param x - координата X (0-127)
 * @param y - координата Y (0-63)
 * @param color - цвет (0 - выкл, 1 - вкл)
 */
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    
    if (color) {
        // Включаем пиксель
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    } else {
        // Выключаем пиксель
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/**
 * Отображение шахматной доски 8x8 на дисплее
 */
void SSD1306_ChessBoard(void) {
    // Размер клетки (дисплей 128x64, доска 8x8)
    uint8_t cell_width = SSD1306_WIDTH / 8;   // 16 пикселей
    uint8_t cell_height = SSD1306_HEIGHT / 8; // 8 пикселей
    
    // Рисуем шахматную доску
    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            // Определяем цвет клетки (чередование черных и белых)
            uint8_t color = ((row + col) % 2 == 0) ? 0 : 1;
            
            // Рисуем клетку заданного размера
            for (uint8_t y = 0; y < cell_height; y++) {
                for (uint8_t x = 0; x < cell_width; x++) {
                    SSD1306_DrawPixel(col * cell_width + x, 
                                     row * cell_height + y, 
                                     color);
                }
            }
        }
    }
    
    // Обновляем дисплей
    SSD1306_Update();
}

// ==================== ГЛАВНАЯ ПРОГРАММА ====================

int main(void) {
    // Инициализация дисплея
    SSD1306_Init();
    
    // Отображение шахматной доски
    SSD1306_ChessBoard();
    
    // Бесконечный цикл
    while (1) {
        // Программа завершена, дисплей продолжает отображать шахматную доску
    }
}