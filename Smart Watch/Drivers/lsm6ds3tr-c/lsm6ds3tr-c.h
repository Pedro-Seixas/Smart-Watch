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
	uint8_t bw0_xl 		: 1;
	uint8_t lpf1_bw_sel : 1;
	uint8_t fs_xl		: 2;
	uint8_t odr,xl 		: 4;
} lsm6ds3tr_c_ctrl1_xl_t;

#define CTRL10_C 		0x19
#define OUTX_L_XL		0x28
#define OUTX_L_G 		0x22
#define OUT_TEMP_L  	0x20
#define STEP_COUNTER_L	0x4B
#define FUNC_SRC2		0x54

#endif /* LSM6DS3TR_C_LSM6DS3TR_C_H_ */
