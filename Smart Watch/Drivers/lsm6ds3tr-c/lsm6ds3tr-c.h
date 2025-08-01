/*
 * lsm6ds3tr-c.h
 *
 *  Created on: Aug 1, 2025
 *      Author: P. Seixas
 */

#ifndef LSM6DS3TR_C_LSM6DS3TR_C_H_
#define LSM6DS3TR_C_LSM6DS3TR_C_H_

// Registers
#define WHO_AM_I 		0x0F

#define CTRL1_XL 		0x10U
typedef struct{
	uint8_t bw0_xl 			: 1;
	uint8_t lpf1_bw_sel	 	: 1;
	uint8_t fs_xl			: 2;
	uint8_t odr,xl 			: 4;
} lsm6ds3tr_c_ctrl1_xl_t;

#define CTRL10_C 		0x19
typedef struct{
	uint8_t sign_motion_en		: 1;
	uint8_t pedo_rst_step		: 1;
	uint8_t func_en			: 1;
	uint8_t tilt_en			: 1;
	uint8_t pedo_en 		: 1;
	uint8_t timer_en		: 1;
	uint8_t not_used_01		: 1;
	uint8_t wrist_tilt_en	: 1;

} lsm6ds3tr_c_ctrl10_t;

#define FUNC_SRC2		0x54
typedef struct{
	uint8_t wrist_tilt_ia		: 1;
	uint8_t not_used_01		: 2;
	uint8_t slave0_nack		: 1;
	uint8_t slave1_nack		: 1;
	uint8_t slave2_nack		: 1;
	uint8_t slave3_nack		: 1;
	uint8_t not_used02		: 1;
} lsm6ds3tr_c_func_src2_t;

#define OUTX_L_XL		0x28
#define OUTX_L_G 		0x22
#define OUT_TEMP_L  	0x20
#define STEP_COUNTER_L	0x4B

// Functions
void lsm6ds3tr_c_init(I2C_HandleTypeDef *hi2c); // TODO separate into accel and gyro init
void lsm6ds3tr_c_write_register(uint8_t reg, uint8_t value);
void lsm6ds3tr_c_read_accel(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay, int16_t *az);
void lsm6ds3tr_c_read_gyro(I2C_HandleTypeDef *hi2c, int16_t *gx, int16_t *gy, int16_t *gz);
void lsm6ds3tr_c_read_temp(I2C_HandleTypeDef *hi2c, int16_t *temp);
void lsm6ds3tr_c_read_step_count(I2C_HandleTypeDef *hi2c, uint16_t *steps);
void lsm6ds3tr_c_pedometer_init(I2C_HandleTypeDef *hi2c);
void lsm6ds3tr_c_wrist_tilt_init(I2C_HandleTypeDef *hi2c);
void lsm6ds3tr_c_who_am_i(I2C_HandleTypeDef *hi2c);
uint8_t lsm6ds3tr_c_read_wrist(I2C_HandleTypeDef *hi2c);
#endif /* LSM6DS3TR_C_LSM6DS3TR_C_H_ */
