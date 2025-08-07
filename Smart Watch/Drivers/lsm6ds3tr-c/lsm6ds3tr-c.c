/*
 * lsm6ds3tr-c.c
 *
 *  Created on: Aug 1, 2025
 *      Author: P. Seixas
 */
#include "lsm6ds3tr-c.h"

void lsm6ds3tr_c_write_register(I2C_HandleTypeDef *hi2c, uint8_t reg,
		uint8_t* value) {
	HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, reg, I2C_MEMADD_SIZE_8BIT, value, 1,
			HAL_MAX_DELAY);
}

void lsm6ds3tr_c_init(I2C_HandleTypeDef *hi2c) {
	uint8_t ctrl1_xl = 0x60;
	uint8_t ctrl2_g = 0x40;

	lsm6ds3tr_c_write_register(hi2c, CTRL1_XL, &ctrl1_xl);

	lsm6ds3tr_c_write_register(hi2c, CTRL2_G, &ctrl2_g);
}

void lsm6ds3tr_c_ctrl10_set(I2C_HandleTypeDef *hi2c, uint8_t bits_to_set) {
	uint8_t ctrl10;

	// Read bits so we don't overwrite the register
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, CTRL10_C, I2C_MEMADD_SIZE_8BIT,
			&ctrl10, 1, HAL_MAX_DELAY);

	ctrl10 |= bits_to_set;

	lsm6ds3tr_c_write_register(hi2c, CTRL10_C, &ctrl10);
}

void lsm6ds3tr_c_read_accel(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay,
		int16_t *az) {
	uint8_t accelData[6];

	// Read 6 bytes from OUTX_L_XL to OUTZ_H_XL (Accelerometer)
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, OUTX_L_XL, I2C_MEMADD_SIZE_8BIT,
			accelData, 6, HAL_MAX_DELAY);

	// Combine high and low bytes
	*ax = (int16_t) (accelData[1] << 8 | accelData[0]);
	*ay = (int16_t) (accelData[3] << 8 | accelData[2]);
	*az = (int16_t) (accelData[5] << 8 | accelData[4]);
}

void lsm6ds3tr_c_read_gyro(I2C_HandleTypeDef *hi2c, int16_t *gx, int16_t *gy,
		int16_t *gz) {
	uint8_t gyroData[6];

	// Read 6 bytes from OUTX_L_G (Gyroscope)
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, OUTX_L_G, I2C_MEMADD_SIZE_8BIT,
			gyroData, 6, HAL_MAX_DELAY);

	// Combine high and low bytes
	*gx = (int16_t) (gyroData[1] << 8 | gyroData[0]);
	*gy = (int16_t) (gyroData[3] << 8 | gyroData[2]);
	*gz = (int16_t) (gyroData[5] << 8 | gyroData[4]);
}

void lsm6ds3tr_c_read_temp(I2C_HandleTypeDef *hi2c, int16_t *temp) {
	uint8_t tempData[2];

	// Read 2 bytes from OUTX_TEMP_L (Temperature Sensor)
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, OUT_TEMP_L, I2C_MEMADD_SIZE_8BIT,
			tempData, 2, HAL_MAX_DELAY);

	// Combine high and low bytes
	*temp = (int16_t) (tempData[1] << 8 | tempData[0]);
}

void lsm6ds3tr_c_read_step_count(I2C_HandleTypeDef *hi2c, uint16_t *steps) {
	uint8_t stepsData[2];

	// Read 2 bytes from STEP_COUNTER_L (Pedometer Sensor)
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, STEP_COUNTER_L, I2C_MEMADD_SIZE_8BIT,
			stepsData, 2, HAL_MAX_DELAY);

	// Combine high and low bytes
	*steps = (stepsData[1] << 8 | stepsData[0]);
}

void lsm6ds3tr_c_pedometer_init(I2C_HandleTypeDef *hi2c) {
	lsm6ds3tr_c_ctrl10_t ctrl10;
	ctrl10.pedo_en = 1;
	ctrl10.func_en = 1;

	lsm6ds3tr_c_write_register(hi2c, CTRL10_C, &ctrl10);
}

void lsm6ds3tr_c_wrist_tilt_init(I2C_HandleTypeDef *hi2c) {
	lsm6ds3tr_c_ctrl10_t ctrl10;
	ctrl10.wrist_tilt_en = 1;

	lsm6ds3tr_c_ctrl10_set(hi2c, (uint8_t*) &ctrl10);

	// Configure a latency of 200ms
	uint8_t A_WRIST_TILT_LAT_C = 0x50;

	// Latency of 200ms for tilting
	uint8_t a_wrist_tilt_lat = 0x05;

	lsm6ds3tr_c_write_register(hi2c, A_WRIST_TILT_LAT_C, &a_wrist_tilt_lat);
}

void lsm6ds3tr_c_tap_cfg(I2C_HandleTypeDef *hi2c) {
	// Enable tap detection
	uint8_t ctrl1_xl = 0x60;
	uint8_t tap_cfg = 0x8E;
	uint8_t tap_ths_6d = 0x8F;
	uint8_t int_dur2 = 0x7D;
	uint8_t wake_up_ths = 0x80;
	uint8_t md1_cfg = 0x08;

	// Accel Adjustment
	lsm6ds3tr_c_write_register(hi2c, CTRL1_XL, &ctrl1_xl);

	// Tap Init
	lsm6ds3tr_c_write_register(hi2c, TAP_CFG, &tap_cfg);

	// Tap intensity threshold
	lsm6ds3tr_c_write_register(hi2c, TAP_THS_6D, &tap_ths_6d);

	// Tap duration / quiet time
	lsm6ds3tr_c_write_register(hi2c, INT_DUR2, &int_dur2);

	// Allow double taps
	lsm6ds3tr_c_write_register(hi2c, WAKE_UP_THS, &wake_up_ths);

	// Mapping interrupt to INT1
	lsm6ds3tr_c_write_register(hi2c, MD1_CFG, &md1_cfg);
}

uint8_t lsm6ds3tr_c_who_am_i(I2C_HandleTypeDef *hi2c) {
	uint8_t who_am_i = 0;
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, 0x0F, I2C_MEMADD_SIZE_8BIT, &who_am_i,
			1, HAL_MAX_DELAY);

	/*
	 char msg[32];
	 int len = snprintf(msg, sizeof(msg), "WHO_AM_I: 0x%02X\r\n", who_am_i);
	 CDC_Transmit_FS((uint8_t*)msg, len);
	 */

	return who_am_i;
}

uint8_t lsm6ds3tr_c_read_wrist(I2C_HandleTypeDef *hi2c) {
	uint8_t wrist_tilt_detected;
	char msg[64];

	// Read Wrist Tilt
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, FUNC_SRC2, I2C_MEMADD_SIZE_8BIT,
			&wrist_tilt_detected, 1, HAL_MAX_DELAY);

	if (wrist_tilt_detected & 0x01) {
		int len = snprintf(msg, sizeof(msg), "Wrist Detected!\r\n");
		CDC_Transmit_FS((uint8_t*) msg, len);
	} else {
		wrist_tilt_detected = 0;
	}

	return wrist_tilt_detected;
}

uint8_t lsm6ds3tr_c_get_tap(I2C_HandleTypeDef *hi2c) {
	uint8_t tap_detected;

	// Read Tap
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, TAP_SRC, I2C_MEMADD_SIZE_8BIT,
			&tap_detected, 1, HAL_MAX_DELAY);

	/*
	 // Any Tap = 0x40
	 if(tap_detected & 0x40 || tap_detected & 0x20){
	 tap_detected = 1;
	 }else{
	 tap_detected = 0;
	 }
	 */
	char msg[32];
	int len = snprintf(msg, sizeof(msg), "WHO_AM_I: %d\r\n", tap_detected);
	CDC_Transmit_FS((uint8_t*) msg, len);
	return tap_detected;
}
