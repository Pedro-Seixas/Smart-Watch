/*
 * lsm6ds3tr-c.h
 *
 *  Created on: Aug 1, 2025
 *      Author: P. Seixas
 */

#ifndef LSM6DS3TR_C_LSM6DS3TR_C_H_
#define LSM6DS3TR_C_LSM6DS3TR_C_H_

#include <stdint.h>
#include <stdio.h>
#include "stm32l0xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
#define LSM6DS3TR_C_I2C_PORT	hi2c1

// Registers

#define CTRL1_XL 		0x10
typedef struct{
	uint8_t bw0_xl 			: 1;
	uint8_t lpf1_bw_sel	 	: 1;
	uint8_t fs_xl			: 2;
	uint8_t odr,xl 			: 4;
} lsm6ds3tr_c_ctrl1_xl_t;

#define CTRL10_C 		0x19
typedef union{
	struct{
		uint8_t sign_motion_en	: 1;
		uint8_t pedo_rst_step	: 1;
		uint8_t func_en			: 1;
		uint8_t tilt_en			: 1;
		uint8_t pedo_en 		: 1;
		uint8_t timer_en		: 1;
		uint8_t not_used_01		: 1;
		uint8_t wrist_tilt_en	: 1;
	};
	uint8_t value;
} lsm6ds3tr_c_ctrl10_t;;

#define FUNC_SRC2		0x54
typedef union{
	struct{
		uint8_t wrist_tilt_ia	: 1;
		uint8_t not_used_01		: 2;
		uint8_t slave0_nack		: 1;
		uint8_t slave1_nack		: 1;
		uint8_t slave2_nack		: 1;
		uint8_t slave3_nack		: 1;
		uint8_t not_used02		: 1;
	};
	uint8_t value;
} lsm6ds3tr_c_func_src2_t;

#define LSM6DS3_ADDR  (0x6B << 1)
#define WHO_AM_I 		0x0F
#define WHO_AM_I_VALUE	0x6A
#define OUTX_L_XL		0x28
#define OUTX_L_G 		0x22
#define OUT_TEMP_L  	0x20
#define STEP_COUNTER_L	0x4B
#define TAP_CFG			0x58
#define TAP_SRC			0x1C
#define TAP_THS_6D		0x59
#define INT_DUR2		0x5A
#define WAKE_UP_THS		0x5B
#define MD1_CFG			0x5E
#define CTRL2_G			0x11

// Functions
void lsm6ds3tr_c_write_register(uint8_t reg, uint8_t* value);
void lsm6ds3tr_c_init(); // TODO separate into accel and gyro init
void lsm6ds3tr_c_ctrl10_set(uint8_t bits_to_set);
void lsm6ds3tr_c_read_accel(int16_t *ax, int16_t *ay, int16_t *az);
void lsm6ds3tr_c_read_gyro(int16_t *gx, int16_t *gy, int16_t *gz);
void lsm6ds3tr_c_read_temp(int16_t *temp);
void lsm6ds3tr_c_read_step_count(uint16_t *steps);
void lsm6ds3tr_c_pedometer_init();
void lsm6ds3tr_c_wrist_tilt_init();
void lsm6ds3tr_c_tap_cfg();
uint8_t lsm6ds3tr_c_who_am_i();
uint8_t lsm6ds3tr_c_read_wrist();
uint8_t lsm6ds3tr_c_get_tap();

// Helper Functions
int8_t map_to_range(int16_t value);
int convert_to_fahrenheit(int16_t rawTemp);
#endif /* LSM6DS3TR_C_LSM6DS3TR_C_H_ */
