/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "is31fl3741.h"
#include "usbd_cdc_if.h"
#include "string.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

const uint8_t PWM_Gamma64Length = 64;
const uint8_t PWM_Gamma64[64] =
{
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0b,0x0d,0x0f,0x11,0x13,0x16,
    0x1a,0x1c,0x1d,0x1f,0x22,0x25,0x28,0x2e,
    0x34,0x38,0x3c,0x40,0x44,0x48,0x4b,0x4f,
    0x55,0x5a,0x5f,0x64,0x69,0x6d,0x72,0x77,
    0x7d,0x80,0x88,0x8d,0x94,0x9a,0xa0,0xa7,
    0xac,0xb0,0xb9,0xbf,0xc6,0xcb,0xcf,0xd6,
    0xe1,0xe9,0xed,0xf1,0xf6,0xfa,0xfe,0xff
};

const uint16_t ISSI1_LED_REGLength = IS31FL3741_FULL_SIZE;
uint8_t ISSI1_LED_REG[IS31FL3741_FULL_SIZE] = { 0 };

const uint16_t ISSI2_LED_REGLength = IS31FL3741_FULL_SIZE;
uint8_t ISSI2_LED_REG[IS31FL3741_FULL_SIZE] = { 0 };

uint8_t ISSI_DEFAULT_PWM = 0x03;
uint8_t ISSI_DEFAULT_SCALE = 0xA0;

IS31FL3741_LED led;
IS31FL3741_ADDR d1_addr;

int8_t rot1 = 0;
int8_t rot2 = 0;

int8_t active_row = 0;
const uint8_t NUM_OF_ROWS = 5;
uint16_t ROW_PINS[5] = {
    ROW1_Pin,
    ROW2_Pin,
    ROW3_Pin,
    ROW4_Pin,
    ROW5_Pin
};

GPIO_TypeDef *ROW_PORTS[5] = {
    ROW1_GPIO_Port,
    ROW2_GPIO_Port,
    ROW3_GPIO_Port,
    ROW4_GPIO_Port,
    ROW5_GPIO_Port
};

const uint8_t NUM_OF_COLS = 18;
uint16_t COL_PINS[18] = {
    COL1_Pin,
    COL2_Pin,
    COL3_Pin,
    COL4_Pin,
    COL5_Pin,
    COL6_Pin,
    COL7_Pin,
    COL8_Pin,
    COL9_Pin,
    COL10_Pin,
    COL11_Pin,
    COL12_Pin,
    COL13_Pin,
    COL14_Pin,
    COL15_Pin,
    COL16_Pin,
    COL17_Pin,
    COL18_Pin
};

GPIO_TypeDef *COL_PORTS[18] = {
    COL1_GPIO_Port,
    COL2_GPIO_Port,
    COL3_GPIO_Port,
    COL4_GPIO_Port,
    COL5_GPIO_Port,
    COL6_GPIO_Port,
    COL7_GPIO_Port,
    COL8_GPIO_Port,
    COL9_GPIO_Port,
    COL10_GPIO_Port,
    COL11_GPIO_Port,
    COL12_GPIO_Port,
    COL13_GPIO_Port,
    COL14_GPIO_Port,
    COL15_GPIO_Port,
    COL16_GPIO_Port,
    COL17_GPIO_Port,
    COL18_GPIO_Port
};

char keys[5][18] = {
        "E1234567890-=BDFFR",
        "Tqwertyuiop[]BIFFR",
        "Casdfghjkl;'EEFF",
        "Szxcvbnm,./SAFFF",
        "CWASKAWLDRFF"
};


// represents the entire matrix, each row is a single 32bit int each bit represents one col
uint8_t active_keys[5][18] = { 0 };

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t One_By_One_LED_Mode(uint8_t address, char *msgBuffer);

void Scale_ISSI1_LEDs( uint8_t *reg );
void Scale_ISSI2_LEDs( uint8_t *reg );
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // double detents per pulse so we need to keep track of raising AND falling edges
    if ( GPIO_Pin == ROT1_A_Pin ) {
        GPIO_PinState A_State = HAL_GPIO_ReadPin(ROT1_A_GPIO_Port, ROT1_A_Pin);
        GPIO_PinState B_State = HAL_GPIO_ReadPin(ROT1_B_GPIO_Port, ROT1_B_Pin);

        if ( A_State == B_State ) {
            rot1--;
        } else {
            rot1++;
        }

        if ( rot1 < 0 ) rot1 = 0;
        else if ( rot1 > 64 ) rot1 = 64;
    } else if ( GPIO_Pin == ROT2_A_Pin ) {
        GPIO_PinState A_State = HAL_GPIO_ReadPin(ROT2_A_GPIO_Port, ROT2_A_Pin);
        GPIO_PinState B_State = HAL_GPIO_ReadPin(ROT2_B_GPIO_Port, ROT2_B_Pin);

        if ( A_State == B_State ) {
            rot2--;
        } else {
            rot2++;
        }

        if ( rot2 < 0 ) rot2 = 0;
        else if ( rot2 > 64 ) rot2 = 64;
    }
}

void Scale_ISSI1_LEDs( uint8_t *reg ){
    uint8_t fullOnRange = (uint8_t) (IS31FL3741_REG_PG0_SIZE + 0x60);
    for ( uint8_t i = 0; i < fullOnRange; i++ ){
        reg[i] = ISSI_DEFAULT_SCALE;
    }

    // except CS25-27, SW2
    // which is 1E offset (SW2) + 24 (0x18)
    uint8_t offset = 0x1E + 0x18;
    reg[ offset ]       = 0x00;
    reg[ offset + 1 ]   = 0x00;
    reg[ offset + 2 ]   = 0x00;

    // add underglow
    uint8_t upperOffset = fullOnRange;
    uint8_t swSteps = 0x09;
    for ( uint8_t i = upperOffset+3; i+2 < IS31FL3741_FULL_SIZE; i += swSteps ) {
        reg[i]   = ISSI_DEFAULT_SCALE;
        reg[i+1] = ISSI_DEFAULT_SCALE;
        reg[i+2] = ISSI_DEFAULT_SCALE;
    }

    // activate window
    // CS31-33 SW3,6,7,8
    // SW3 + 18, SW6 + 0x2D
    reg[ fullOnRange+18 ]    = ISSI_DEFAULT_SCALE;
    reg[ fullOnRange+18+1 ]  = ISSI_DEFAULT_SCALE;
    reg[ fullOnRange+18+2 ]  = ISSI_DEFAULT_SCALE;

    for ( uint8_t i = upperOffset + 0x2D; i + 2 < fullOnRange + 0x48; i += swSteps ) {
        reg[i]   = ISSI_DEFAULT_SCALE;
        reg[i+1] = ISSI_DEFAULT_SCALE;
        reg[i+2] = ISSI_DEFAULT_SCALE;
    }
}

void Scale_ISSI2_LEDs( uint8_t *reg ){
    for ( uint8_t i = 0; i <= 0xB4 + 0x3D; i += 0x1E ) {
        for ( uint8_t j = 0; j < 21; j++ ) {
            reg[ i+j ] = ISSI_DEFAULT_SCALE;
        }
    }

    // except CS19-21, SW2
    // which is 1E offset (SW2) + 18 (0x12)
    reg[ 0x1E + 0x12 ]      = 0x00;
    reg[ 0x1E + 0x12 + 1 ]  = 0x00;
    reg[ 0x1E + 0x12 + 2 ]  = 0x00;

    // CS34-39 SW1-9
    // add underglow
    uint8_t upperOffset = (uint8_t) (IS31FL3741_REG_PG0_SIZE + 0x60);
    uint8_t swSteps = 0x09;
    for ( uint8_t i = upperOffset + 3; i+5 <= IS31FL3741_FULL_SIZE; i += swSteps ) {
        for ( uint8_t j = 0; j < 6; j++ ) {
            reg[i]   = ISSI_DEFAULT_SCALE;
        }
    }
}

uint8_t One_By_One_LED_Mode(uint8_t address, char *msgBuffer) {
    if ( IS31FL3741_SetLED( &led, address ) != HAL_OK ) {
        sprintf(msgBuffer, "Unable to light up LED 0x%02X on page 0x%02X.\r\n", d1_addr.addr, d1_addr.page);
        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
    }

    HAL_Delay(500);

    led.value[0] = 0x00;
    led.value[1] = 0x00;
    led.value[2] = 0x00;

    if ( IS31FL3741_SetLED( &led, address ) != HAL_OK ) {
        sprintf(msgBuffer, "Unable to turn off LED 0x%02X on page 0x%02X.\r\n", d1_addr.addr, d1_addr.page);
        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
    }

    d1_addr.addr = d1_addr.addr + 3;
    if ( d1_addr.page == 0 && d1_addr.addr >= IS31FL3741_REG_PG0_SIZE ) {
        d1_addr.page = 0x01;
        d1_addr.addr = 0x00;
    } else if ( d1_addr.page == 1 && d1_addr.addr >= IS31FL3741_REG_PG1_SIZE ) {
        d1_addr.page = 0x00;
        d1_addr.addr = 0x00;
        if ( address == IS31FL3741_I2C_ADDR_1 )
            address = IS31FL3741_I2C_ADDR_2;
        else
            address = IS31FL3741_I2C_ADDR_1;
    }

    led.value[0] = 0xFF;
    led.value[1] = 0xFF;
    led.value[2] = 0xFF;

    if ( IS31FL3741_SetLED( &led, address ) != HAL_OK ) {
        sprintf(msgBuffer, "Unable to light up LED 0x%02X on page 0x%02X.\r\n", d1_addr.addr, d1_addr.page);
        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
    }

    return address;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  led.i2cHandle = &hi2c1;
  led.setting = PWM;
  led.addr = &d1_addr;
  led.value[0] = 0xFF;
  led.value[1] = 0xFF;
  led.value[2] = 0xFF;

  d1_addr.page = 0x00;
  d1_addr.addr = 0x00;

    HAL_Delay(5000);
    char msgBuffer[256];
    uint8_t id[3];

    // testing ISSI-1 by reading its I2C address from memory
    if( IS31FL3741_ReadID( &hi2c1, IS31FL3741_I2C_ADDR_1, id ) != HAL_OK ) {
        sprintf(msgBuffer, "Error during transmitting I2C data to ISSI-1\r\n");
    } else if ( id[0] != IS31FL3741_I2C_ADDR_1 ) {
        sprintf(msgBuffer, "ISSI-1 responded with wrong ID: 0x%02X; expected: 0x%02X\r\n", id[0], IS31FL3741_I2C_ADDR_1);
    } else {
        sprintf(msgBuffer, "ISSI-1 address check successful\r\n");
    }

    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
    HAL_Delay(500);

    // testing ISSI-2 by reading its I2C address from memory
    if( IS31FL3741_ReadID( &hi2c1, IS31FL3741_I2C_ADDR_2, id ) != HAL_OK ) {
        sprintf(msgBuffer, "Error during transmitting I2C data to ISSI-2\r\n");
    } else if ( id[0] != IS31FL3741_I2C_ADDR_2 ) {
        sprintf(msgBuffer, "ISSI-2 responded with wrong ID: 0x%02X; expected: 0x%02X\r\n", id[0], IS31FL3741_I2C_ADDR_2);
    } else {
        sprintf(msgBuffer, "ISSI-2 address check successful\r\n");
    }

    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
    HAL_Delay(500);

    sprintf(msgBuffer, "Init LED registers\r\n");
    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

//    Scale_ISSI1_LEDs( ISSI1_LED_REG );

    for ( uint16_t i = 0; i < IS31FL3741_FULL_SIZE; i++ ){
        ISSI1_LED_REG[i] = 0xFF;
    }

    for ( uint16_t i = 0; i < IS31FL3741_FULL_SIZE; i++ ){
        ISSI2_LED_REG[i] = 0xFF;
    }

//    Scale_ISSI2_LEDs( ISSI2_LED_REG );
    HAL_Delay(500);

    sprintf(msgBuffer, "Finish setting scale values.\r\n");
    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

    HAL_Delay(500);
    sprintf(msgBuffer, "Init ISSI 1.\r\n");
    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

    IS31FL3741_Init( &hi2c1, IS31FL3741_I2C_ADDR_1, ISSI1_LED_REG, ISSI1_LED_REGLength, &ISSI_DEFAULT_PWM );

    HAL_Delay(500);
    sprintf(msgBuffer, "Init ISSI 2.\r\n");
    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

    IS31FL3741_Init( &hi2c1, IS31FL3741_I2C_ADDR_2, ISSI2_LED_REG, ISSI2_LED_REGLength, &ISSI_DEFAULT_PWM );
//
//    HAL_Delay(2000);
//    sprintf(msgBuffer, "Init ISSI 2 (init).\r\n");
//    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//    IS31FL3741_Init( &hi2c1, IS31FL3741_I2C_ADDR_2, ISSI2_LED_REG, ISSI2_LED_REGLength, &ISSI_DEFAULT_PWM );
//
//    sprintf(msgBuffer, "All setup done. Try lighting up a single LED. Confirm values are 0.\r\n");
//    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));



    // ==================================== CHECK CAPSLOCK VALUES =================================
//    d1_addr.page = 0x00;
//    d1_addr.addr = 0x78; // CS1, SW5 (capslock blue)
//
//    IS31FL3741_Unlock( &hi2c1, IS31FL3741_I2C_ADDR_1 );
//    IS31FL3741_OpenPage(&hi2c1, d1_addr.page, IS31FL3741_I2C_ADDR_1 );
//    if ( HAL_I2C_Mem_Read(&hi2c1, IS31FL3741_I2C_ADDR_1, d1_addr.addr, I2C_MEMADD_SIZE_8BIT, id, I2C_MEMADD_SIZE_8BIT*3, IS31FL3741_I2C_TIMEOUT) != HAL_OK ){
//        sprintf(msgBuffer, "Unable to read PWM LED CapsLock values.\r\n");
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//    } else {
//        sprintf(msgBuffer, "CapsLock LED PWM is set: 0x%02X, 0x%02X, 0x%02X\r\n", id[0], id[1], id[2]);
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//    }
//
//    d1_addr.page = 0x02;
//    IS31FL3741_Unlock( &hi2c1, IS31FL3741_I2C_ADDR_1 );
//    IS31FL3741_OpenPage(&hi2c1, d1_addr.page, IS31FL3741_I2C_ADDR_1 );
//    if ( HAL_I2C_Mem_Read(&hi2c1, IS31FL3741_I2C_ADDR_1, d1_addr.addr, I2C_MEMADD_SIZE_8BIT, id, I2C_MEMADD_SIZE_8BIT*3, IS31FL3741_I2C_TIMEOUT) != HAL_OK ){
//        sprintf(msgBuffer, "Unable to read SCALE LED CapsLock values.\r\n");
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//    } else {
//        sprintf(msgBuffer, "CapsLock LED SCALE is set: 0x%02X, 0x%02X, 0x%02X\r\n", id[0], id[1], id[2]);
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//    }
//
//    HAL_Delay(500);
//    sprintf(msgBuffer, "Expected Scale Values: 0x%02X, 0x%02X, 0x%02X\r\n", ISSI1_LED_REG[d1_addr.addr], ISSI1_LED_REG[d1_addr.addr+1], ISSI1_LED_REG[d1_addr.addr+2]);
//    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
    // ==================================== CHECK CAPSLOCK VALUES =================================

//
//    IS31FL3741_OpenPage(&hi2c1, 0x02, IS31FL3741_I2C_ADDR_1 );
//    if ( HAL_I2C_Mem_Read(&hi2c1, IS31FL3741_I2C_ADDR_1, 0x02, I2C_MEMADD_SIZE_8BIT, &id[0], I2C_MEMADD_SIZE_8BIT, IS31FL3741_I2C_TIMEOUT) != HAL_OK ){
//        sprintf(msgBuffer, "Unable to read SCALE LED 0 values.\r\n");
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//    } else {
//        sprintf(msgBuffer, "LED 1 SCALE is set: 0x%02X\r\n", id[0]);
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//    }
//
//    sprintf(msgBuffer, "Start lighting up LEDs one by one (complete white)\r\n");
//    CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    char buffer[NUM_OF_ROWS][NUM_OF_COLS+1];

    for ( uint8_t i = 0; i < NUM_OF_ROWS; i++ ){
        buffer[i][NUM_OF_COLS] = '\0';
    }

    while (1)
    {
        // ================================ ACTIVATE ROWS =============================
        // pull row high

        for ( uint8_t col = 0; col < NUM_OF_COLS; col++ ) {
            HAL_GPIO_WritePin(COL_PORTS[col], COL_PINS[col], GPIO_PIN_SET);

            for ( uint8_t row = 0; row < NUM_OF_ROWS; row++ ) {
                active_keys[row][col] = HAL_GPIO_ReadPin(ROW_PORTS[row], ROW_PINS[row]);
                if ( active_keys[row][col] )
                    buffer[row][col] = keys[row][col];
                else buffer[row][col] = ' ';
            }

            HAL_GPIO_WritePin(COL_PORTS[col], COL_PINS[col], GPIO_PIN_RESET);
            HAL_Delay(10);
        }

//        for ( uint8_t row = 0; row < NUM_OF_ROWS; row++ ) {
//            snprintf(intBuffer, 20, "%d", (int) active_keys[row]);
//        }

        sprintf(msgBuffer, "Keyboard Matrix:\r\n%19s | %d\r\n%19s | %d\r\n%19s\r\n%19s\r\n%19s\r\n\r\n",
                buffer[0], rot1,
                buffer[1], rot2,
                buffer[2],
                buffer[3],
                buffer[4]);
        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

//        if ( active_row == 0 ) {
//            sprintf(msgBuffer, "Keyboard Matrix:\r\n"BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" | %02d\r\n"BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" | %02d\r\n"BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\r\n"BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\r\n"BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\r\n\r\n",
//                    BYTE_TO_BINARY(active_cols[0]>>24), BYTE_TO_BINARY(active_cols[0]>>16), BYTE_TO_BINARY(active_cols[0]>>8), BYTE_TO_BINARY(active_cols[0]), rot1,
//                    BYTE_TO_BINARY(active_cols[1]>>24), BYTE_TO_BINARY(active_cols[1]>>16), BYTE_TO_BINARY(active_cols[1]>>8), BYTE_TO_BINARY(active_cols[1]), rot2,
//                    BYTE_TO_BINARY(active_cols[2]>>24), BYTE_TO_BINARY(active_cols[2]>>16), BYTE_TO_BINARY(active_cols[2]>>8), BYTE_TO_BINARY(active_cols[2]),
//                    BYTE_TO_BINARY(active_cols[3]>>24), BYTE_TO_BINARY(active_cols[3]>>16), BYTE_TO_BINARY(active_cols[3]>>8), BYTE_TO_BINARY(active_cols[3]),
//                    BYTE_TO_BINARY(active_cols[4]>>24), BYTE_TO_BINARY(active_cols[4]>>16), BYTE_TO_BINARY(active_cols[4]>>8), BYTE_TO_BINARY(active_cols[4])
//                    );
//            CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));
//        }

//        address = One_By_One_LED_Mode(address, msgBuffer);
//
//        sprintf(msgBuffer, "Light LED 0x%02X on page 0x%02X (ISSI: 0x%02X).\r\n", d1_addr.addr, d1_addr.page, address);
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

//        sprintf(msgBuffer, "Idle.\r\n");
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

        // ================================== ENCODER HARDCODED =================================
//        if ( HAL_GPIO_ReadPin(ROT1_A_GPIO_Port, ROT1_A_Pin) == GPIO_PIN_RESET ) {
//            // means A was pulled low
//            if ( HAL_GPIO_ReadPin(ROT1_B_GPIO_Port, ROT1_B_Pin) == GPIO_PIN_RESET ) {
//                // B is already pulled low, so B was pulled before A, hence CCW
//                while ( HAL_GPIO_ReadPin(ROT1_B_GPIO_Port, ROT1_B_Pin) == GPIO_PIN_RESET ) {}
//                counter--;
//                while ( HAL_GPIO_ReadPin(ROT1_A_GPIO_Port, ROT1_A_Pin) == GPIO_PIN_RESET ) {}
//                HAL_Delay(10);
//            } else {
//                // means A is low but B is still low. Hence, CW, wait for B to be pulled LOW too
//                while ( HAL_GPIO_ReadPin(ROT1_B_GPIO_Port, ROT1_B_Pin) == GPIO_PIN_SET ) {}
//                // now we are essentially in the top case and wait for A to be pulled high again
//
//                counter++;
//                while ( HAL_GPIO_ReadPin(ROT1_A_GPIO_Port, ROT1_A_Pin) == GPIO_PIN_RESET ) {}
//                // and lastly B being back to normal too
//                while ( HAL_GPIO_ReadPin(ROT1_B_GPIO_Port, ROT1_B_Pin) == GPIO_PIN_RESET ) {}
//                HAL_Delay(10);
//            }
//
//
//
//            // some normalization so we dont get out of bounds
//            if ( counter < 0  ) counter = 0;
//            if ( counter > 64 ) counter = 64;
//
//        }


        // ===================== FIND ISSUE OF NOT WORKING I2C ================================
        for ( uint16_t i = 0; i < IS31FL3741_FULL_SIZE; i++ ){
            ISSI1_LED_REG[i] = PWM_Gamma64[rot1];
        }

        IS31FL3741_WriteAllLEDs( &hi2c1, IS31FL3741_I2C_ADDR_1, ISSI1_LED_REG );
        IS31FL3741_WriteAllLEDs( &hi2c1, IS31FL3741_I2C_ADDR_2, ISSI1_LED_REG );

//        sprintf(msgBuffer, "Counter %02d LEDs PWM to 0x%02X.\r\n", rot1, PWM_Gamma64[rot1]);
//        CDC_Transmit_FS((uint8_t *) msgBuffer, strlen(msgBuffer));

//        HAL_Delay(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
