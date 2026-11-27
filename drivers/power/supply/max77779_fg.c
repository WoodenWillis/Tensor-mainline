// SPDX-License-Identifier: GPL-2.0-only

#include <linux/i2c.h>
#include <linux/math64.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/property.h>
#include <linux/regmap.h>

#define MAX77779_FG_RepCap	0x06
#define MAX77779_FG_RepSOC	0x07
#define MAX77779_FG_FullCapRep	0x10
#define MAX77779_FG_AvgVCell	0x19
#define MAX77779_FG_VCell	0x1a
#define MAX77779_FG_Temp	0x1b
#define MAX77779_FG_Current	0x1c
#define MAX77779_FG_AvgCurrent	0x1d
#define MAX77779_FG_DevName	0x21

struct max77779_fg {
	struct regmap *regmap;
	u32 rsense_uohm;
};

static bool max77779_fg_writeable_reg(struct device *dev, unsigned int reg)
{
	return false;
}

static const struct regmap_config max77779_fg_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
	.val_format_endian = REGMAP_ENDIAN_LITTLE,
	.max_register = 0xff,
	.writeable_reg = max77779_fg_writeable_reg,
};

static int max77779_fg_read(struct max77779_fg *fg, u8 reg, u16 *val)
{
	unsigned int raw;
	int ret;

	ret = regmap_read(fg->regmap, reg, &raw);
	if (ret)
		return ret;

	*val = raw;
	return 0;
}

static int max77779_fg_capacity(struct max77779_fg *fg, int *pct)
{
	u16 val;
	int ret;

	ret = max77779_fg_read(fg, MAX77779_FG_RepSOC, &val);
	if (ret)
		return ret;

	*pct = val >> 8;
	return 0;
}

static int max77779_fg_voltage(struct max77779_fg *fg, u8 reg, int *uv)
{
	u16 val;
	int ret;

	ret = max77779_fg_read(fg, reg, &val);
	if (ret)
		return ret;

	*uv = div_u64((u64)val * 78125, 1000);
	return 0;
}

static int max77779_fg_current(struct max77779_fg *fg, u8 reg, int *ua)
{
	u16 val;
	int ret;

	ret = max77779_fg_read(fg, reg, &val);
	if (ret)
		return ret;

	*ua = div_s64((s64)(s16)val * 1562500, fg->rsense_uohm);
	return 0;
}

static int max77779_fg_charge(struct max77779_fg *fg, u8 reg, int *uah)
{
	u16 val;
	int ret;

	ret = max77779_fg_read(fg, reg, &val);
	if (ret)
		return ret;

	*uah = div_u64((u64)val * 5000000, fg->rsense_uohm);
	return 0;
}

static int max77779_fg_temp(struct max77779_fg *fg, int *decidegc)
{
	u16 val;
	int ret;

	ret = max77779_fg_read(fg, MAX77779_FG_Temp, &val);
	if (ret)
		return ret;

	*decidegc = div_s64((s64)(s16)val * 10, 256);
	return 0;
}

static int max77779_fg_status(struct max77779_fg *fg, int *status)
{
	int ua, pct, ret;

	ret = max77779_fg_current(fg, MAX77779_FG_Current, &ua);
	if (ret)
		return ret;

	ret = max77779_fg_capacity(fg, &pct);
	if (ret)
		return ret;

	if (ua > 0)
		*status = POWER_SUPPLY_STATUS_CHARGING;
	else if (pct >= 100)
		*status = POWER_SUPPLY_STATUS_FULL;
	else
		*status = POWER_SUPPLY_STATUS_DISCHARGING;

	return 0;
}

static const enum power_supply_property max77779_fg_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_TEMP,
};

static int max77779_fg_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct max77779_fg *fg = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		return max77779_fg_status(fg, &val->intval);
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		return 0;
	case POWER_SUPPLY_PROP_CAPACITY:
		return max77779_fg_capacity(fg, &val->intval);
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		return max77779_fg_voltage(fg, MAX77779_FG_VCell, &val->intval);
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		return max77779_fg_voltage(fg, MAX77779_FG_AvgVCell, &val->intval);
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		return max77779_fg_current(fg, MAX77779_FG_Current, &val->intval);
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		return max77779_fg_current(fg, MAX77779_FG_AvgCurrent, &val->intval);
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		return max77779_fg_charge(fg, MAX77779_FG_RepCap, &val->intval);
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		return max77779_fg_charge(fg, MAX77779_FG_FullCapRep, &val->intval);
	case POWER_SUPPLY_PROP_TEMP:
		return max77779_fg_temp(fg, &val->intval);
	default:
		return -EINVAL;
	}
}

static const struct power_supply_desc max77779_fg_desc = {
	.name = "max77779-fg",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = max77779_fg_props,
	.num_properties = ARRAY_SIZE(max77779_fg_props),
	.get_property = max77779_fg_get_property,
};

static int max77779_fg_probe(struct i2c_client *client)
{
	struct power_supply_config psy_cfg = {};
	struct device *dev = &client->dev;
	struct power_supply *psy;
	struct max77779_fg *fg;
	u16 devname;
	int ret;

	fg = devm_kzalloc(dev, sizeof(*fg), GFP_KERNEL);
	if (!fg)
		return -ENOMEM;

	fg->regmap = devm_regmap_init_i2c(client, &max77779_fg_regmap_config);
	if (IS_ERR(fg->regmap))
		return dev_err_probe(dev, PTR_ERR(fg->regmap),
				     "failed to init regmap\n");

	ret = device_property_read_u32(dev, "shunt-resistor-micro-ohms",
				       &fg->rsense_uohm);
	if (ret || !fg->rsense_uohm)
		return dev_err_probe(dev, -EINVAL,
				     "missing shunt-resistor-micro-ohms\n");

	ret = max77779_fg_read(fg, MAX77779_FG_DevName, &devname);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read DevName\n");

	dev_info(dev, "DevName 0x%04x, Rsense %u uOhm\n", devname,
		 fg->rsense_uohm);

	psy_cfg.drv_data = fg;
	psy_cfg.fwnode = dev_fwnode(dev);

	psy = devm_power_supply_register(dev, &max77779_fg_desc, &psy_cfg);
	if (IS_ERR(psy))
		return dev_err_probe(dev, PTR_ERR(psy),
				     "failed to register power supply\n");

	return 0;
}

static const struct of_device_id max77779_fg_of_match[] = {
	{ .compatible = "maxim,max77779-fg" },
	{ }
};
MODULE_DEVICE_TABLE(of, max77779_fg_of_match);

static struct i2c_driver max77779_fg_driver = {
	.driver = {
		.name = "max77779-fg",
		.of_match_table = max77779_fg_of_match,
	},
	.probe = max77779_fg_probe,
};
module_i2c_driver(max77779_fg_driver);

MODULE_DESCRIPTION("Maxim MAX77779 fuel gauge driver");
MODULE_LICENSE("GPL");
