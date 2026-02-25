#include "driver.h"
#include "trinamic/common.h"
#include "trinamic/tmc2209.h"

#if defined(BOARD_SOVOL_SV06_PLUS)

#define CPU_CLOCK_HZ 72000000UL
#define UART_BAUD_RATE 19200
#define BIT_CYCLES      ((CPU_CLOCK_HZ + (UART_BAUD_RATE / 2)) / UART_BAUD_RATE)

__attribute__((always_inline)) static inline void delay_cycles(uint32_t cycles) {
    volatile uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles);
}

// Mode: 0x4 = Input Floating, 0x1 = Output Push-Pull 10MHz
static inline void set_pin_mode(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t mode) {
    // bit-to-index conversion
    uint32_t pin_index = __builtin_ctz(GPIO_Pin);

    // register selection
    // Treat &GPIOx->CRL as start of an array, CRL is at index 0, CRH is at index 1
    // (pin_index >> 3) gives 0 for pins 0-7 (CRL) and 1 for pins 8-15 (CRH)
    volatile uint32_t *cr_reg = ((volatile uint32_t *)&GPIOx->CRL) + (pin_index >> 3);

    // calculate shift
    uint32_t shift = (pin_index & 0x7) * 4;

    // read-modify-write
    uint32_t temp = *cr_reg;
    temp &= ~(0xF << shift);      // Clear 4 bits
    temp |= (mode << shift);      // Set new mode
    *cr_reg = temp;
}

static inline void set_pin_level(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t level) {
    if (level) GPIOx->BSRR = GPIO_Pin; // Set pin high
    else       GPIOx->BSRR  = GPIO_Pin << 16; // Set pin low
}

static void sw_uart_write_byte(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t byte) {
    //1. Start bit (Low)
    set_pin_level(GPIOx, GPIO_Pin, 0); // TX Low

    uint32_t next_time = DWT->CYCCNT + BIT_CYCLES;

    // 2. Data bits
    for (int i = 0; i < 8; i++) {
        while ((int32_t)(DWT->CYCCNT - next_time) < 0);

        // write bit
        set_pin_level(GPIOx, GPIO_Pin, (byte >> i) & 0x01);
        next_time += BIT_CYCLES;
    }

    // 3. Stop bit (High)
    while ((int32_t)(DWT->CYCCNT - next_time) < 0);
    set_pin_level(GPIOx, GPIO_Pin, 1); // TX High

    // hold stop bit for full duration
    next_time += BIT_CYCLES;
    while ((int32_t)(DWT->CYCCNT - next_time) < 0);
}

static int sw_uart_read_byte(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t *byte_out) {
    uint8_t data = 0;

    uint32_t timeout = 20000;
    // Wait for start bit
    while (GPIOx->IDR & GPIO_Pin) {
        if (--timeout == 0) {
            *byte_out = 0xFF; // Indicate timeout
            return -1;
        }
    }

    // Capture when start bit was detected
    uint32_t start_time = DWT->CYCCNT;

    // Calculate sample point (start + 1.5 bit times)
    uint32_t next_sample_time = start_time + (BIT_CYCLES + (BIT_CYCLES / 2));

    for (int i = 0; i < 8; i++) {
        // Wait until next sample point
        while ((int32_t)(DWT->CYCCNT - next_sample_time) < 0);
        if (GPIOx->IDR & GPIO_Pin) data |= (1 << i);
        next_sample_time += BIT_CYCLES;
    }

    // Wait for stop bit
    while ((int32_t) (DWT->CYCCNT - next_sample_time) < 0);

    // wait for more to clear the stop bit completely
    delay_cycles(BIT_CYCLES / 2);

    *byte_out = data;
    return 0;
}

TMC_uart_write_datagram_t *tmc_uart_read(trinamic_motor_t driver, TMC_uart_read_datagram_t *dgr)
{
    dgr->msg.slave = 3; // Slave address is fixed to 3 in this board
    tmc_crc8(dgr->data, sizeof(TMC_uart_read_datagram_t));

    static TMC_uart_write_datagram_t wdgr = {0};
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;

    switch (driver.axis) {
        case 0: GPIOx = GPIOC; GPIO_Pin = GPIO_PIN_1; break; // X Axis
        case 1: GPIOx = GPIOC; GPIO_Pin = GPIO_PIN_0; break; // Y Axis
        case 2: GPIOx = GPIOA; GPIO_Pin = GPIO_PIN_15; break; // Z Axis
        default: return NULL; // Unsupported axis
    }

    __disable_irq();

    set_pin_mode(GPIOx, GPIO_Pin, 0x1); // Set as Output Push-Pull 10MHz
    set_pin_level(GPIOx, GPIO_Pin, 1); // TX High

    for (int i = 0; i < sizeof(TMC_uart_read_datagram_t); i++) {
        sw_uart_write_byte(GPIOx, GPIO_Pin, dgr->data[i]);
    }

    set_pin_mode(GPIOx, GPIO_Pin, 0x8); // Set as Input Floating

    for (int i = 0; i < sizeof(TMC_uart_write_datagram_t); i++) {
        if (sw_uart_read_byte(GPIOx, GPIO_Pin, &wdgr.data[i]) != 0) {
            wdgr.msg.addr.value = 0xFF; // Indicate timeout
            break;
        }
    }

    set_pin_mode(GPIOx, GPIO_Pin, 0x1); // Set as Output Push-Pull 10MHz
    set_pin_level(GPIOx, GPIO_Pin, 1); // TX High

    delay_cycles(BIT_CYCLES * 4);

    __enable_irq();

    return &wdgr;
}

void tmc_uart_write (trinamic_motor_t driver, TMC_uart_write_datagram_t *dgr)
{
    dgr->msg.slave = 3; // Slave address is fixed to 3 in this board
    tmc_crc8(dgr->data, sizeof(TMC_uart_write_datagram_t));

    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;

    switch (driver.axis) {
        case 0: GPIOx = GPIOC; GPIO_Pin = GPIO_PIN_1; break; // X Axis
        case 1: GPIOx = GPIOC; GPIO_Pin = GPIO_PIN_0; break; // Y Axis
        case 2: GPIOx = GPIOA; GPIO_Pin = GPIO_PIN_15; break; // Z Axis
        default: return NULL; // Unsupported axis
    }

    __disable_irq();
    set_pin_mode(GPIOx, GPIO_Pin, 0x1); // Set as Output Push-Pull 10MHz
    set_pin_level(GPIOx, GPIO_Pin, 1); // TX High

    for (int i = 0; i < sizeof(TMC_uart_write_datagram_t); i++) {
        sw_uart_write_byte(GPIOx, GPIO_Pin, dgr->data[i]);
    }

    // guard time
    delay_cycles(BIT_CYCLES * 4);

    __enable_irq();
}

void board_init(void)
{
    // Initialize DWT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

#endif // BOARD_SOVOL_SV06_PLUS