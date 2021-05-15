/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* host command customization configuration */

#ifndef __HOST_COMMAND_CUSTOMIZATION_H
#define __HOST_COMMAND_CUSTOMIZATION_H

/*****************************************************************************/
/* Configure the behavior of the flash notify */
#define EC_CMD_FLASH_NOTIFIED 0x3E01

enum ec_flash_notified_flags {
	/* Enable/Disable power button pulses for x86 devices */
	FLASH_ACCESS_SPI	  = 0,
	FLASH_FIRMWARE_START  = BIT(0),
	FLASH_FIRMWARE_DONE   = BIT(1),
	FLASH_ACCESS_SPI_DONE = 3,
	FLASH_FLAG_PD         = BIT(4),
};

struct ec_params_flash_notified {
	/* See enum ec_flash_notified_flags */
	uint8_t flags;
} __ec_align1;

/* Factory will need change Fnkey and power button
 * key scancode to test keyboard.
 */
#define EC_CMD_FACTORY_MODE	0x3E02
#define RESET_FOR_SHIP 0x5A

struct ec_params_factory_notified {
	/* factory mode enable */
	uint8_t flags;
} __ec_align1;

/* Configure the behavior of the charge limit control */
#define EC_CMD_CHARGE_LIMIT_CONTROL 0x3E03
#define NEED_RESTORE 0x7F

enum ec_chg_limit_control_modes {
	/* Disable all setting, charge control by charge_manage */
	CHG_LIMIT_DISABLE	= BIT(0),
	/* Set maximum and minimum percentage */
	CHG_LIMIT_SET_LIMIT	= BIT(1),
	/* Host read current setting */
	CHG_LIMIT_GET_LIMIT	= BIT(3),
	/* Enable override mode, allow charge to full this time */
	CHG_LIMIT_OVERRIDE	= BIT(7),
};

struct ec_params_ec_chg_limit_control {
	/* See enum ec_chg_limit_control_modes */
	uint8_t modes;
	uint8_t max_percentage;
	uint8_t min_percentage;
} __ec_align1;

struct ec_response_chg_limit_control {
	uint8_t max_percentage;
	uint8_t min_percentage;
} __ec_align1;

#define EC_CMD_PWM_GET_FAN_ACTUAL_RPM	0x3E04

struct ec_response_pwm_get_actual_fan_rpm {
	uint32_t rpm;
} __ec_align4;

#define EC_CMD_SET_AP_REBOOT_DELAY	0x3E05

struct ec_response_ap_reboot_delay {
	uint8_t delay;
} __ec_align1;

#define EC_CMD_ME_CONTROL	0x3E06

enum ec_mecontrol_modes {
	ME_LOCK		= BIT(0),
	ME_UNLOCK	= BIT(1),
};

struct ec_params_me_control {
	uint8_t me_mode;
} __ec_align1;

#define EC_CMD_CUSTOM_HELLO	0x3E07

#define EC_CMD_DISABLE_PS2_EMULATION 0x3E08

struct ec_params_ps2_emulation_control {
	uint8_t disable;
} __ec_align1;

#define EC_CMD_CHASSIS_INTRUSION 0x3E09
#define EC_PARAM_CHASSIS_INTRUSION_MAGIC 0xCE
#define EC_PARAM_CHASSIS_BBRAM_MAGIC 0xEC

struct ec_params_chassis_intrusion_control {
	uint8_t clear_magic;
	uint8_t clear_chassis_status;
} __ec_align1;

struct ec_response_chassis_intrusion_control {
	uint8_t chassis_ever_opened;			/* have rtc(VBAT) no battery(VTR) */
	uint8_t coin_batt_ever_remove;
	uint8_t total_open_count;
	uint8_t vtr_open_count;
} __ec_align1;

/* Debug LED for BIOS boot check */
#define EC_CMD_DIAGNOSIS 0x3E0B

/* bit7 as trigger LED behavior or not, bit6-bit0 as error_type */
#define EC_CMD_DIAGNOSIS_LED 0x80
#define EC_CMD_DIAGNOSIS_LED_TYPE 0x7F

enum ec_params_diagnosis_type {
	TYPE_DDR	= 1,
	TYPE_PORT80,
	TYPE_COUNT
};

enum ec_params_diagnosis_subtype_ddr {
	/* type: DDR */
	TYPE_DDR_TRAINING_START	= 1,
	TYPE_DDR_TRAINING_FINISH,
	TYPE_DDR_FAIL,
	TYPE_DDR_COUNT
};

enum ec_params_diagnosis_subtype_port80 {
	/* type: PORT80 */
	TYPE_PORT80_COMPLETE	= 1,
	TYPE_PORT80_COUNT
};

struct ec_params_diagnosis {
	/* See enum ec_params_diagnosis_type_ */
	uint8_t error_type;
	/* See enum ec_params_diagnosis_subtype_ */
	uint8_t error_subtype;
} __ec_align1;

#define EC_CMD_UPDATE_KEYBOARD_MATRIX 0x3E0C
struct keyboard_matrix_map {
	uint8_t row;
	uint8_t col;
	uint16_t scanset;
} __ec_align1;
struct ec_params_update_keyboard_matrix {
	uint32_t num_items;
	uint32_t write;
	struct keyboard_matrix_map scan_update[32];
} __ec_align1;

#define EC_CMD_VPRO_CONTROL	0x3E0D

enum ec_vrpo_control_modes {
	VPRO_OFF	= 0,
	VPRO_ON		= BIT(0),
};

struct ec_params_vpro_control {
	uint8_t vpro_mode;
} __ec_align1;

#endif /* __HOST_COMMAND_CUSTOMIZATION_H */