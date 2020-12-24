/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "adc.h"
#include "adc_chip.h"
#include "board.h"
#include "battery.h"
#include "battery_smart.h"
#include "charge_state.h"
#include "console.h"
#include "ec_commands.h"
#include "host_command.h"
#include "host_command_customization.h"
#include "system.h"
#include "i2c.h"
#include "string.h"
#include "system.h"
#include "util.h"
#include "hooks.h"

/* Shutdown mode parameter to write to manufacturer access register */
#define PARAM_CUT_OFF_LOW  0x10
#define PARAM_CUT_OFF_HIGH 0x00

/* Battery info for BQ40Z50 4-cell */
static const struct battery_info info = {
	.voltage_max = 17600,        /* mV */
	.voltage_normal = 15400,
	.voltage_min = 12000,
	.precharge_current = 72,   /* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 47,
	.charging_min_c = 0,
	.charging_max_c = 52,
	.discharging_min_c = 0,
	.discharging_max_c = 62,
};

static uint8_t charging_maximum_level = NEED_RESTORE;
static int old_btp;

const struct battery_info *battery_get_info(void)
{
	return &info;
}

int board_cut_off_battery(void)
{
	int rv;
	uint8_t buf[3];

	/* Ship mode command must be sent twice to take effect */
	buf[0] = SB_MANUFACTURER_ACCESS & 0xff;
	buf[1] = PARAM_CUT_OFF_LOW;
	buf[2] = PARAM_CUT_OFF_HIGH;

	i2c_lock(I2C_PORT_BATTERY, 1);
	rv = i2c_xfer_unlocked(I2C_PORT_BATTERY, BATTERY_ADDR_FLAGS,
			       buf, 3, NULL, 0, I2C_XFER_SINGLE);
	rv |= i2c_xfer_unlocked(I2C_PORT_BATTERY, BATTERY_ADDR_FLAGS,
				buf, 3, NULL, 0, I2C_XFER_SINGLE);
	i2c_lock(I2C_PORT_BATTERY, 0);

	return rv;
}

enum battery_present battery_is_present(void)
{
	enum battery_present bp;
	int mv;

	mv = adc_read_channel(ADC_VCIN1_BATT_TEMP);

	if (mv == ADC_READ_ERROR)
		return -1;

	bp = (mv < 3000 ? BP_YES : BP_NO);

	return bp;
}

#ifdef CONFIG_EMI_REGION1

void battery_customize(struct charge_state_data *emi_info)
{
	char text[32];
	char *str = "LION";
	int value;
	int new_btp;
	static int batt_state;

	*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_AVER_TEMP) = 
							(emi_info->batt.temperature - 2731)/10;
	*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_PERCENTAGE) = emi_info->batt.display_charge/10;
	
	if (emi_info->batt.status & STATUS_FULLY_CHARGED)
		*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_STATUS) |= EC_BATT_FLAG_FULL;
	else
		*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_STATUS) &= ~EC_BATT_FLAG_FULL;

	battery_device_chemistry(text, sizeof(text));
	if (!strncmp(text, str, 4))
		*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_STATUS) |= EC_BATT_TYPE;
	else
		*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_STATUS) &= ~EC_BATT_TYPE;

	battery_get_mode(&value);
	if (value & MODE_CAPACITY)
		*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_STATUS) |= EC_BATT_MODE;
	else
		*host_get_customer_memmap(EC_MEMMAP_ER1_BATT_STATUS) &= ~EC_BATT_MODE;
	
	/* BTP: Notify AP update battery */
	new_btp = *host_get_customer_memmap(0x08) + (*host_get_customer_memmap(0x09) << 8);
	if (new_btp > old_btp && !battery_is_cut_off())
	{
		if (emi_info->batt.remaining_capacity > new_btp)
		{
			old_btp = new_btp;
			host_set_single_event(EC_HOST_EVENT_BATT_BTP);
			ccprintf ("trigger higher BTP: %d", old_btp);
		}
	} else if (new_btp < old_btp && !battery_is_cut_off())
	{
		if (emi_info->batt.remaining_capacity < new_btp)
		{
			old_btp = new_btp;
			host_set_single_event(EC_HOST_EVENT_BATT_BTP);
			ccprintf ("trigger lower BTP: %d", old_btp);
		}
	}

	/*
	 * When the battery present have change notify AP
	 */
	if (batt_state != emi_info->batt.is_present) {
		host_set_single_event(EC_HOST_EVENT_BATTERY_STATUS);
		batt_state = emi_info->batt.is_present;
	}
}

#endif

static void bettery_percentage_control(void)
{
	static enum ec_charge_control_mode before_mode;
	enum ec_charge_control_mode new_mode;
	int rv;


	if(charging_maximum_level == NEED_RESTORE)
		charging_maximum_level = 0;
		// TODO
		// system_get_bbram(SYSTEM_BBRAM_IDX_CHG_MAX, &charging_maximum_level);

	if( charging_maximum_level < 20 )
		new_mode = CHARGE_CONTROL_NORMAL;
	else if( charge_get_percent() > charging_maximum_level){
		new_mode = CHARGE_CONTROL_DISCHARGE;
	}
	else if( charge_get_percent() == charging_maximum_level)
		new_mode = CHARGE_CONTROL_IDLE;
	else
		new_mode = CHARGE_CONTROL_NORMAL;

	if( before_mode != new_mode ){
		before_mode = new_mode;
		set_chg_ctrl_mode(before_mode);
#ifdef CONFIG_CHARGER_DISCHARGE_ON_AC
		rv = charger_discharge_on_ac(before_mode == CHARGE_CONTROL_DISCHARGE);
#endif
		if (rv != EC_SUCCESS)
			ccprintf("fail to discharge.");
	}
}
DECLARE_HOOK(HOOK_AC_CHANGE, bettery_percentage_control, HOOK_PRIO_DEFAULT);
DECLARE_HOOK(HOOK_BATTERY_SOC_CHANGE, bettery_percentage_control, HOOK_PRIO_DEFAULT);

/*****************************************************************************/
/* Customize host command */

/*
 * Charging limit control.
 */
static enum ec_status cmd_charging_limit_control(struct host_cmd_handler_args *args)
{

	const struct ec_params_ec_chg_limit_control *p = args->params;
	// TDDO: read limit
	// struct ec_response_chg_limit_control *r = args->response;
	// TODO: bbram
	// system_get_bbram(SYSTEM_BBRAM_IDX_CHG_MAX, &charging_maximum_level);

	if (p->modes & CHG_LIMIT_DISABLE){
		charging_maximum_level = 0;
	}

	if (p->modes & CHG_LIMIT_SET_LIMIT){
		if( p->max_percentage < 20 )
			return EC_RES_ERROR;

		charging_maximum_level = p->max_percentage;
	}

	if (p->modes & CHG_LIMIT_OVERRIDE)
		charging_maximum_level = charging_maximum_level | CHG_LIMIT_OVERRIDE;

	// TODO: bbran
	// system_set_bbram(SYSTEM_BBRAM_IDX_CHG_MAX, charging_maximum_level);

	if (p->modes & CHG_LIMIT_GET_LIMIT){
		// TODO: read limit
		/*
		system_get_bbram(SYSTEM_BBRAM_IDX_CHG_MAX, &charging_maximum_level);
		r->max_percentage = charging_maximum_level;
		args->response_size = sizeof(*r);
		*/
	}

	bettery_percentage_control();

	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_CHARGE_LIMIT_CONTROL, cmd_charging_limit_control,
			EC_VER_MASK(0));
