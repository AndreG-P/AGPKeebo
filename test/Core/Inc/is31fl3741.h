/*
 * is31fl3741.h
 * I2C Driver for IS31FL3741
 *
 * Note that write lock is enabled on startup. One has to
 *
 *  Created on: 24.02.2023
 *      Author: Andre G-P
 */

#ifndef IS31FL3741_I2C_DRIVER_H_
#define IS31FL3741_I2C_DRIVER_H_

// for I2C communication, we use the HAL interface of STM32F4
#include "stm32f4xx_hal.h"

/* DEFINITIONS */
#define IS31FL3741_I2C_ADDR_1         (0x30 << 1) // 01100 ADDR (0/1), pulled to GND is 00 -> 0110 000(0/1)
#define IS31FL3741_I2C_ADDR_2         (0x33 << 1) // 01100 ADDR (0/1), pulled to GND is 00 -> 0110 011(0/1)

#define IS31FL3741_I2C_TIMEOUT      HAL_MAX_DELAY

/* REGISTERS */
#define IS31FL3741_REG_CMD          0xFD
#define IS31FL3741_REG_WRITE_LOCK   0xFE
#define IS31FL3741_REG_INT_MASK     0xF0
#define IS31FL3741_REG_INT_STAT     0xF1
#define IS31FL3741_REG_ID           0xFC

#define IS31FL3741_REG_PAGE_0       0x00
#define IS31FL3741_REG_PAGE_1       0x01
#define IS31FL3741_REG_PAGE_2       0x02
#define IS31FL3741_REG_PAGE_3       0x03
#define IS31FL3741_REG_PAGE_4       0x04

#define IS31FL3741_REG_PWM_PG0      (0x00 | IS31FL3741_REG_PAGE_0)
#define IS31FL3741_REG_PWM_PG1      (0x00 | IS31FL3741_REG_PAGE_1)

#define IS31FL3741_REG_SCALE_PG0    (0x02 | IS31FL3741_REG_PAGE_0)
#define IS31FL3741_REG_SCALE_PG1    (0x02 | IS31FL3741_REG_PAGE_1)

#define IS31FL3741_REG_PAGE_4_FUNC      IS31FL3741_REG_PAGE_4
#define IS31FL3741_REG_CONFIG           0x00
#define IS31FL3741_REG_GLOBAL_CURR      0x01
#define IS31FL3741_REG_RESISTORS        0x02
#define IS31FL3741_REG_SHORT_REG_BASE   0x03
#define IS31FL3741_REG_PWM_FREQ         0x36
#define IS31FL3741_REG_RESET            0x3F

#define IS31FL3741_REG_PG0_SIZE     0xB4
#define IS31FL3741_REG_PG1_SIZE     0xAB
#define IS31FL3741_REG_PG2_SIZE     IS31FL3741_REG_PG0_SIZE
#define IS31FL3741_REG_PG3_SIZE     IS31FL3741_REG_PG1_SIZE
#define IS31FL3741_REG_PG4_SIZE     0x3E

#define IS31FL3741_FULL_SIZE        (uint16_t) (IS31FL3741_REG_PG0_SIZE + IS31FL3741_REG_PG1_SIZE)

/* COMMAND VALUES */
#define IS31FL3741_CMD_LOCK         0x00
#define IS31FL3741_CMD_UNLOCK       0xC5

// Ideally Scaling is full for all installed LEDs only (otherwise 0)
// only PWM gets updated to control brightness
#define IS31FL3741_DEFAULT_SCALING  0xFF
#define IS31FL3741_DEFAULT_PWM      0x00

typedef struct {
    uint8_t page;
    uint8_t addr;
} IS31FL3741_ADDR;

typedef enum {
    PWM = 0,
    SCALE = 2
} IS31FL3741_SETTING;

/*
 * LED STRUCT
 */
typedef struct {
    I2C_HandleTypeDef *i2cHandle;

    IS31FL3741_ADDR *addr;

    IS31FL3741_SETTING setting;

    uint8_t value[3];
} IS31FL3741_LED;

HAL_StatusTypeDef IS31FL3741_Init( I2C_HandleTypeDef *i2cHandle, uint8_t addr, uint8_t *scales, uint8_t scalesLength, uint8_t * default_pwm );

HAL_StatusTypeDef IS31FL3741_Unlock( I2C_HandleTypeDef *i2cHandle, uint8_t addr );

HAL_StatusTypeDef IS31FL3741_OpenPage( I2C_HandleTypeDef *i2cHandle, uint8_t page, uint8_t addr );

HAL_StatusTypeDef IS31FL3741_ReadID( I2C_HandleTypeDef *i2cHandle, uint8_t address, uint8_t *data );

HAL_StatusTypeDef IS31FL3741_WriteAllLEDs( I2C_HandleTypeDef *i2cHandle, uint8_t address, uint8_t *values );

HAL_StatusTypeDef IS31FL3741_SetLED( IS31FL3741_LED *led, uint8_t address );

#endif /* IS31FL3741_I2C_DRIVER_H_ */
