/*
 * lsm6ds3tr-c.c
 *
 *  Created on: Aug 1, 2025
 *      Author: P. Seixas
 */
#include "lsm6ds3tr-c.h"

void lsm6ds3tr_c_write_register(uint8_t reg,
		uint8_t* value) {
	HAL_I2C_Mem_Write(&LSM6DS3TR_C_I2C_PORT, LSM6DS3_ADDR, reg, I2C_MEMADD_SIZE_8BIT, value, 1,
			HAL_MAX_DELAY);
}

void lsm6ds3tr_c_read_register(uint8_t reg, uint8_t* data, size_t size){
	HAL_I2C_Mem_Read(&LSM6DS3TR_C_I2C_PORT, LSM6DS3_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
				data, size, HAL_MAX_DELAY);
}

void lsm6ds3tr_c_init() {
	uint8_t ctrl1_xl = 0x60;
	uint8_t ctrl2_g = 0x40;

	lsm6ds3tr_c_write_register(CTRL1_XL, &ctrl1_xl);

	lsm6ds3tr_c_write_register(CTRL2_G, &ctrl2_g);
}

void lsm6ds3tr_c_ctrl10_set(uint8_t bits_to_set) {
	uint8_t ctrl10;

	// Read bits so we don't overwrite the register
	lsm6ds3tr_c_read_register(CTRL10_C, &ctrl10, sizeof(ctrl10));

	ctrl10 |= bits_to_set;

	lsm6ds3tr_c_write_register(CTRL10_C, &ctrl10);
}

void lsm6ds3tr_c_read_accel(int16_t *ax, int16_t *ay,
		int16_t *az) {
	uint8_t accelData[6];

	// Read 6 bytes from OUTX_L_XL to OUTZ_H_XL (Accelerometer)
	lsm6ds3tr_c_read_register(OUTX_L_XL, accelData, sizeof(accelData));

	// Combine high and low bytes
	*ax = (int16_t) (accelData[1] << 8 | accelData[0]);
	*ay = (int16_t) (accelData[3] << 8 | accelData[2]);
	*az = (int16_t) (accelData[5] << 8 | accelData[4]);
}

void lsm6ds3tr_c_read_gyro(int16_t *gx, int16_t *gy,
		int16_t *gz) {
	uint8_t gyroData[6];

	// Read 6 bytes from OUTX_L_G (Gyroscope)
	lsm6ds3tr_c_read_register(OUTX_L_G, gyroData, sizeof(gyroData));

	// Combine high and low bytes
	*gx = (int16_t) (gyroData[1] << 8 | gyroData[0]);
	*gy = (int16_t) (gyroData[3] << 8 | gyroData[2]);
	*gz = (int16_t) (gyroData[5] << 8 | gyroData[4]);
}

void lsm6ds3tr_c_read_temp(int16_t *temp) {
	uint8_t tempData[2];

	// Read 2 bytes from OUTX_TEMP_L (Temperature Sensor)
	lsm6ds3tr_c_read_register(OUT_TEMP_L, tempData, sizeof(tempData));

	// Combine high and low bytes
	*temp = (int16_t) (tempData[1] << 8 | tempData[0]);
}

void lsm6ds3tr_c_read_step_count(uint16_t *steps) {
	uint8_t stepsData[2];

	// Read 2 bytes from STEP_COUNTER_L (Pedometer Sensor)
	lsm6ds3tr_c_read_register(STEP_COUNTER_L, stepsData, sizeof(stepsData));

	// Combine high and low bytes
	*steps = (stepsData[1] << 8 | stepsData[0]);
}

void lsm6ds3tr_c_pedometer_init() {
	lsm6ds3tr_c_ctrl10_t ctrl10;
	ctrl10.pedo_en = 1;
	ctrl10.func_en = 1;

	lsm6ds3tr_c_write_register(CTRL10_C, &ctrl10.value);
}

void lsm6ds3tr_c_wrist_tilt_init() {
	lsm6ds3tr_c_ctrl10_t ctrl10;
	ctrl10.wrist_tilt_en = 1;

	lsm6ds3tr_c_ctrl10_set(ctrl10.value);

	// Configure a latency of 200ms
	uint8_t A_WRIST_TILT_LAT_C = 0x50;

	// Latency of 200ms for tilting
	uint8_t a_wrist_tilt_lat = 0x05;

	lsm6ds3tr_c_write_register(A_WRIST_TILT_LAT_C, &a_wrist_tilt_lat);
}

void lsm6ds3tr_c_tap_cfg() {
	// Enable tap detection
	uint8_t tap_cfg = 0x8E;
	uint8_t tap_ths_6d = 0x8F;
	uint8_t int_dur2 = 0x7D;
	uint8_t wake_up_ths = 0x80;
	uint8_t md1_cfg = 0x08;

	// Tap Init
	lsm6ds3tr_c_write_register(TAP_CFG, &tap_cfg);

	// Tap intensity threshold
	lsm6ds3tr_c_write_register(TAP_THS_6D, &tap_ths_6d);

	// Tap duration / quiet time
	lsm6ds3tr_c_write_register(INT_DUR2, &int_dur2);

	// Allow double taps
	lsm6ds3tr_c_write_register(WAKE_UP_THS, &wake_up_ths);

	// Mapping double taps interrupt to INT1
	lsm6ds3tr_c_write_register(MD1_CFG, &md1_cfg);
}

uint8_t lsm6ds3tr_c_who_am_i() {
	uint8_t who_am_i = 0;

	lsm6ds3tr_c_read_register(WHO_AM_I, &who_am_i, sizeof(who_am_i));

	/*
	 char msg[32];
	 int len = snprintf(msg, sizeof(msg), "WHO_AM_I: 0x%02X\r\n", who_am_i);
	 CDC_Transmit_FS((uint8_t*)msg, len);
	 */

	return who_am_i;
}

uint8_t lsm6ds3tr_c_read_wrist() {
	uint8_t wrist_tilt_detected;
	char msg[64];

	// Read Wrist Tilt
	lsm6ds3tr_c_read_register(FUNC_SRC2, &wrist_tilt_detected, sizeof(wrist_tilt_detected));

	if (wrist_tilt_detected & 0x01) {
		int len = snprintf(msg, sizeof(msg), "Wrist Detected!\r\n");
		CDC_Transmit_FS((uint8_t*) msg, len);
	} else {
		wrist_tilt_detected = 0;
	}

	return wrist_tilt_detected;
}

uint8_t lsm6ds3tr_c_get_tap() {
	uint8_t tap_detected;

	// Read Tap
	lsm6ds3tr_c_read_register(TAP_SRC, &tap_detected, sizeof(tap_detected));

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

int8_t map_to_range(int16_t value) {
    // Map from int16_t to -90,90
    if (value >= 0)
        return (int8_t)(((int32_t)value * 90) / 32767);
    else
        return (int8_t)(((int32_t)value * 90) / 32768);
}

int convert_to_fahrenheit(int16_t rawTemp) {
    float tempC = 25.0f + ((float)rawTemp / 16.0f);
    // float tempF = tempC * 1.8f + 32.0f;
    return (int)(tempC);
}
