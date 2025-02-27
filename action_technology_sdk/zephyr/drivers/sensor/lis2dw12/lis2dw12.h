/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_

#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/sensor.h>
#include <stmemsc.h>
#include "lis2dw12_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/* Return ODR reg value based on data rate set */
#define LIS2DW12_ODR_TO_REG(_odr) \
	((_odr <= 1) ? LIS2DW12_XL_ODR_1Hz6_LP_ONLY : \
	 (_odr <= 12) ? LIS2DW12_XL_ODR_12Hz5 : \
	 ((31 - __builtin_clz(_odr / 25))) + 3)

/* FS reg value from Full Scale */
#define LIS2DW12_FS_TO_REG(_fs)	(30 - __builtin_clz(_fs))

/* Acc Gain value in ug/LSB in High Perf mode */
#define LIS2DW12_FS_2G_GAIN		244
#define LIS2DW12_FS_4G_GAIN		488
#define LIS2DW12_FS_8G_GAIN		976
#define LIS2DW12_FS_16G_GAIN		1952

#define LIS2DW12_SHFT_GAIN_NOLP1	2
#define LIS2DW12_ACCEL_GAIN_DEFAULT_VAL LIS2DW12_FS_2G_GAIN
#define LIS2DW12_FS_TO_GAIN(_fs, _lp1) \
		(LIS2DW12_FS_2G_GAIN << ((_fs) + (_lp1)))

/* shift value for power mode */
#define LIS2DW12_SHIFT_PM1		4
#define LIS2DW12_SHIFT_PMOTHER		2

/**
 * struct lis2dw12_device_config - lis2dw12 hw configuration
 * @bus_name: Pointer to bus master identifier.
 * @pm: Power mode (lis2dh_powermode).
 * @int_pin: Sensor int pin (int1/int2).
 */
struct lis2dw12_device_config {
	stmdev_ctx_t ctx;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct stmemsc_cfg_i2c i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct stmemsc_cfg_spi spi;
#endif
	} stmemsc_cfg;
	lis2dw12_mode_t pm;
	uint8_t range;
#ifdef CONFIG_LIS2DW12_TRIGGER
	struct gpio_dt_spec gpio_int;
	uint8_t int_pin;
#ifdef CONFIG_LIS2DW12_TAP
	uint8_t tap_mode;
	uint8_t tap_threshold[3];
	uint8_t tap_shock;
	uint8_t tap_latency;
	uint8_t tap_quiet;
#endif /* CONFIG_LIS2DW12_TAP */
#endif /* CONFIG_LIS2DW12_TRIGGER */
};

/* sensor data */
struct lis2dw12_data {
	int16_t acc[3];

	 /* save sensitivity */
	uint16_t gain;

#ifdef CONFIG_LIS2DW12_TRIGGER
	const struct device *dev;

	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t drdy_handler;
#ifdef CONFIG_LIS2DW12_TAP
	sensor_trigger_handler_t tap_handler;
	sensor_trigger_handler_t double_tap_handler;
#endif /* CONFIG_LIS2DW12_TAP */
#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DW12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_LIS2DW12_TRIGGER */
};

#ifdef CONFIG_LIS2DW12_TRIGGER
int lis2dw12_init_interrupt(const struct device *dev);
int lis2dw12_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_LIS2DW12_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_H_ */
