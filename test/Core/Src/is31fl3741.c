/*
 * is31fl3741.c
 *
 *  Created on: 24.02.2023
 *      Author: andre
 */

#include "is31fl3741.h"

uint8_t INIT_VAL = 0x50;
uint8_t INIT_CURR = 0x7F;
uint8_t INIT_PWMF = 0xA0;
uint8_t NORMAL_OP = 0x01;
uint8_t DEFAULT_SCALE = 0xFF;

/*
 * Starts communicating with IS31FL3741 and requests device ID for confirmation.
 * Returns OK if the communication setup worked. Otherwise returns ERROR.
 *
 * If everything goes well it sets PWM to 0, SCALE to 0, and enters normal operation
 */
HAL_StatusTypeDef IS31FL3741_Init( I2C_HandleTypeDef *i2cHandle, uint8_t addr, uint8_t *scales, uint8_t scalesLength, uint8_t * default_pwm ) {
    // unlock
    if ( IS31FL3741_Unlock( i2cHandle, addr ) != HAL_OK ) return HAL_ERROR;

    // set all PWM regs to 0 on page 0
    IS31FL3741_OpenPage(i2cHandle, IS31FL3741_REG_PAGE_0, addr);
    for ( uint8_t i = 0; i <= IS31FL3741_REG_PG0_SIZE; i++ ){
        HAL_I2C_Mem_Write(i2cHandle, addr, i, I2C_MEMADD_SIZE_8BIT, default_pwm, 1, IS31FL3741_I2C_TIMEOUT);
    }

    // set all PWM regs to 0 on page 1
    IS31FL3741_Unlock( i2cHandle, addr );
    IS31FL3741_OpenPage(i2cHandle, IS31FL3741_REG_PAGE_1, addr);
    for ( uint8_t i = 0; i <= IS31FL3741_REG_PG1_SIZE; i++ ){
        HAL_I2C_Mem_Write(i2cHandle, addr, i, I2C_MEMADD_SIZE_8BIT, default_pwm, 1, IS31FL3741_I2C_TIMEOUT);
    }

    // set all SCALE regs to 0 on page 2
    IS31FL3741_Unlock( i2cHandle, addr );
    IS31FL3741_OpenPage(i2cHandle, IS31FL3741_REG_PAGE_2, addr);
    HAL_I2C_Mem_Write(i2cHandle, addr, 0x00, I2C_MEMADD_SIZE_8BIT, scales, IS31FL3741_REG_PG0_SIZE, IS31FL3741_I2C_TIMEOUT);

    // set all SCALE regs to 0 on page 3
    IS31FL3741_Unlock( i2cHandle, addr );
    IS31FL3741_OpenPage(i2cHandle, IS31FL3741_REG_PAGE_3, addr);
    HAL_I2C_Mem_Write(i2cHandle, addr, 0x00, I2C_MEMADD_SIZE_8BIT, &scales[IS31FL3741_REG_PG0_SIZE], IS31FL3741_REG_PG1_SIZE, IS31FL3741_I2C_TIMEOUT);

    // page 4 - settings
    IS31FL3741_Unlock( i2cHandle, addr );
    IS31FL3741_OpenPage(i2cHandle, IS31FL3741_REG_PAGE_4_FUNC, addr);

    // set global current
    HAL_I2C_Mem_Write(i2cHandle, addr, IS31FL3741_REG_GLOBAL_CURR, I2C_MEMADD_SIZE_8BIT, &INIT_CURR, I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);

    // set PWM frequency
    HAL_I2C_Mem_Write(i2cHandle, addr, IS31FL3741_REG_PWM_FREQ, I2C_MEMADD_SIZE_8BIT, &INIT_PWMF, I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);

    // set normal operation
    return HAL_I2C_Mem_Write(i2cHandle, addr, IS31FL3741_REG_CONFIG, I2C_MEMADD_SIZE_8BIT, &NORMAL_OP, I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);
}

HAL_StatusTypeDef IS31FL3741_Unlock( I2C_HandleTypeDef *i2cHandle, uint8_t addr ) {
    uint8_t CMD = IS31FL3741_CMD_UNLOCK;
    return HAL_I2C_Mem_Write(i2cHandle, addr, IS31FL3741_REG_WRITE_LOCK, I2C_MEMADD_SIZE_8BIT, &CMD, I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);
}

HAL_StatusTypeDef IS31FL3741_OpenPage( I2C_HandleTypeDef *i2cHandle, uint8_t page, uint8_t addr ) {
    uint8_t pointer = page;
    return HAL_I2C_Mem_Write(i2cHandle, addr, IS31FL3741_REG_CMD, I2C_MEMADD_SIZE_8BIT, &pointer, I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);
}

/*
 * Reads the ID from is31fl3741. The result must be the device address. This can be used to confirm
 * a working communication with is31fl3741.
 */
HAL_StatusTypeDef IS31FL3741_ReadID( I2C_HandleTypeDef *i2cHandle, uint8_t address, uint8_t *data ) {
    return HAL_I2C_Mem_Read(i2cHandle, address, IS31FL3741_REG_ID, I2C_MEMADD_SIZE_8BIT, data, I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);
}

HAL_StatusTypeDef IS31FL3741_WriteAllLEDs( I2C_HandleTypeDef *i2cHandle, uint8_t address, uint8_t *values ) {
    if ( IS31FL3741_Unlock( i2cHandle, address ) != HAL_OK ) return HAL_ERROR;
    IS31FL3741_OpenPage(i2cHandle, IS31FL3741_REG_PAGE_0, address);
    HAL_I2C_Mem_Write(i2cHandle, address, 0x00, I2C_MEMADD_SIZE_8BIT, values, IS31FL3741_REG_PG0_SIZE, IS31FL3741_I2C_TIMEOUT);

    IS31FL3741_Unlock( i2cHandle, address );
    IS31FL3741_OpenPage(i2cHandle, IS31FL3741_REG_PAGE_1, address);
    HAL_I2C_Mem_Write(i2cHandle, address, 0x00, I2C_MEMADD_SIZE_8BIT, &values[IS31FL3741_REG_PG0_SIZE], IS31FL3741_REG_PG1_SIZE, IS31FL3741_I2C_TIMEOUT);

    return HAL_ERROR;
}

HAL_StatusTypeDef IS31FL3741_SetLED( IS31FL3741_LED *led, uint8_t address ) {
    // request an unlock
//    if ( IS31FL3741_Unlock( led->i2cHandle, address ) != HAL_OK ) return HAL_ERROR;

    // send page register
    // calculates the page we want to write to, PWM mode pages are 0 and 1, SCALE are 2 and 3
    uint8_t page = ((uint8_t)led->setting) + led->addr->page;
    if ( IS31FL3741_OpenPage(led->i2cHandle, page, address) != HAL_OK ) return HAL_ERROR;

    return HAL_I2C_Mem_Write(led->i2cHandle, address, led->addr->addr, I2C_MEMADD_SIZE_8BIT, led->value, I2C_MEMADD_SIZE_8BIT*3, IS31FL3741_I2C_TIMEOUT);

    // sets LED values for RGB
//    HAL_I2C_Mem_Write(led->i2cHandle, address, led->addr->addr, I2C_MEMADD_SIZE_8BIT, &led->value[0], I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);
//    HAL_I2C_Mem_Write(led->i2cHandle, address, led->addr->addr+1, I2C_MEMADD_SIZE_8BIT, &led->value[1], I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);
//    return HAL_I2C_Mem_Write(led->i2cHandle, address, led->addr->addr+2, I2C_MEMADD_SIZE_8BIT, &led->value[2], I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT);
}
