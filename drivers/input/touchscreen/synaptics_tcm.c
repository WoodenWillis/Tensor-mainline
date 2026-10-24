// SPDX-License-Identifier: GPL-2.0-only
/*
 * Synaptics TouchComm touchscreen over SPI
 *
 * Copyright 2026 Will
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <linux/unaligned.h>

#define TCM_XFER_SIZE			63
#define TCM_HEADER_SIZE			4
#define TCM_MARKER			0xa5

#define TCM_CMD_IDENTIFY		0x02
#define TCM_CMD_ENABLE_REPORT		0x05
#define TCM_CMD_GET_APPLICATION_INFO	0x20

#define TCM_STATUS_IDLE			0x00
#define TCM_STATUS_OK			0x01
#define TCM_REPORT_IDENTIFY		0x10
#define TCM_REPORT_TOUCH		0x11

#define TCM_IDENTIFY_VERSION		0
#define TCM_IDENTIFY_MODE		1
#define TCM_IDENTIFY_PART		2
#define TCM_IDENTIFY_PART_LEN		16
#define TCM_IDENTIFY_MIN_LEN		(TCM_IDENTIFY_PART + TCM_IDENTIFY_PART_LEN)

#define TCM_APP_INFO_MAX_X		32
#define TCM_APP_INFO_MAX_Y		34
#define TCM_APP_INFO_MIN_LEN		36

#define TCM_TOUCH_PRESENT_LEN		0x17
#define TCM_TOUCH_X			12
#define TCM_TOUCH_Y			14
#define TCM_TOUCH_Z			16

#define TCM_POLL_DELAY_MS		10
#define TCM_RESPONSE_TIMEOUT_MS		3000
#define TCM_POWER_DELAY_MS		200
#define TCM_RESET_ACTIVE_US		2000
#define TCM_RESET_DELAY_MS		50

struct tcm {
	struct spi_device *spi;
	struct input_dev *input;
	struct gpio_desc *reset;
	struct regulator_bulk_data supplies[2];
	struct touchscreen_properties prop;
	u16 max_x;
	u16 max_y;
	u8 tx[TCM_XFER_SIZE] __aligned(ARCH_DMA_MINALIGN);
	u8 rx[TCM_XFER_SIZE] __aligned(ARCH_DMA_MINALIGN);
};

static u8 tcm_code(struct tcm *ts)
{
	return ts->rx[1];
}

static u16 tcm_len(struct tcm *ts)
{
	return get_unaligned_le16(&ts->rx[2]);
}

static u8 *tcm_payload(struct tcm *ts)
{
	return &ts->rx[TCM_HEADER_SIZE];
}

static int tcm_xfer(struct tcm *ts, size_t len)
{
	struct spi_transfer xfer = {
		.tx_buf = ts->tx,
		.rx_buf = ts->rx,
		.len = len,
	};

	return spi_sync_transfer(ts->spi, &xfer, 1);
}

static int tcm_write(struct tcm *ts, u8 cmd, const u8 *payload, u16 len)
{
	size_t size = 3 + len;

	if (size > sizeof(ts->tx))
		return -EINVAL;

	ts->tx[0] = cmd;
	put_unaligned_le16(len, &ts->tx[1]);
	memcpy(&ts->tx[3], payload, len);

	return tcm_xfer(ts, size);
}

static int tcm_read(struct tcm *ts)
{
	int ret;

	memset(ts->tx, 0xff, sizeof(ts->tx));

	ret = tcm_xfer(ts, sizeof(ts->rx));
	if (ret)
		return ret;

	if (ts->rx[0] != TCM_MARKER) {
		dev_dbg(&ts->spi->dev, "bad marker %*ph\n", 4, ts->rx);
		return -EIO;
	}

	if (TCM_HEADER_SIZE + tcm_len(ts) + 1 > sizeof(ts->rx)) {
		dev_dbg(&ts->spi->dev, "message %#04x too long (%u)\n",
			tcm_code(ts), tcm_len(ts));
		return -EMSGSIZE;
	}

	return 0;
}

static bool tcm_is_status(struct tcm *ts)
{
	return tcm_code(ts) != TCM_STATUS_IDLE &&
	       tcm_code(ts) < TCM_REPORT_IDENTIFY;
}

static int tcm_wait_response(struct tcm *ts)
{
	unsigned long deadline = jiffies +
				 msecs_to_jiffies(TCM_RESPONSE_TIMEOUT_MS);

	do {
		msleep(TCM_POLL_DELAY_MS);
		if (!tcm_read(ts) && tcm_is_status(ts))
			return 0;
	} while (time_before(jiffies, deadline));

	return -ETIMEDOUT;
}

static int tcm_command(struct tcm *ts, u8 cmd, const u8 *payload, u16 len)
{
	int ret;

	ret = tcm_write(ts, cmd, payload, len);
	if (ret)
		return ret;

	ret = tcm_wait_response(ts);
	if (ret)
		dev_dbg(&ts->spi->dev, "no response to %#04x\n", cmd);

	return ret;
}

static bool tcm_is_identify(struct tcm *ts)
{
	return (tcm_code(ts) == TCM_REPORT_IDENTIFY ||
		tcm_code(ts) == TCM_STATUS_OK) &&
	       tcm_len(ts) >= TCM_IDENTIFY_MIN_LEN;
}

static int tcm_identify(struct tcm *ts)
{
	struct device *dev = &ts->spi->dev;
	const u8 *p;
	int ret;

	ret = tcm_read(ts);
	if (ret || !tcm_is_identify(ts))
		ret = tcm_command(ts, TCM_CMD_IDENTIFY, NULL, 0);
	if (ret)
		return ret;

	if (!tcm_is_identify(ts)) {
		dev_err(dev, "no identify report: %*ph\n", 8, ts->rx);
		return -ENODEV;
	}

	p = tcm_payload(ts);
	dev_info(dev, "part %.*s, TouchComm v%u, mode %#04x\n",
		 TCM_IDENTIFY_PART_LEN, &p[TCM_IDENTIFY_PART],
		 p[TCM_IDENTIFY_VERSION], p[TCM_IDENTIFY_MODE]);

	return 0;
}

static int tcm_get_app_info(struct tcm *ts)
{
	int ret;

	ret = tcm_command(ts, TCM_CMD_GET_APPLICATION_INFO, NULL, 0);
	if (ret)
		return ret;

	if (tcm_code(ts) != TCM_STATUS_OK || tcm_len(ts) < TCM_APP_INFO_MIN_LEN)
		return -EIO;

	ts->max_x = get_unaligned_le16(&tcm_payload(ts)[TCM_APP_INFO_MAX_X]);
	ts->max_y = get_unaligned_le16(&tcm_payload(ts)[TCM_APP_INFO_MAX_Y]);

	dev_info(&ts->spi->dev, "firmware range %ux%u\n", ts->max_x, ts->max_y);

	return 0;
}

static int tcm_enable_touch_report(struct tcm *ts)
{
	u8 report = TCM_REPORT_TOUCH;
	int ret;

	ret = tcm_command(ts, TCM_CMD_ENABLE_REPORT, &report, 1);
	if (ret)
		return ret;

	if (tcm_code(ts) != TCM_STATUS_OK)
		dev_warn(&ts->spi->dev, "enable report answered %#04x\n",
			 tcm_code(ts));

	return 0;
}

static void tcm_report_touch(struct tcm *ts)
{
	struct input_dev *input = ts->input;
	const u8 *p = tcm_payload(ts);
	bool down = tcm_len(ts) >= TCM_TOUCH_PRESENT_LEN;

	/* TODO(will): decode all active objects, layout not captured yet */
	input_mt_slot(input, 0);
	input_mt_report_slot_state(input, MT_TOOL_FINGER, down);
	if (down) {
		touchscreen_report_pos(input, &ts->prop,
				       get_unaligned_le16(&p[TCM_TOUCH_X]),
				       get_unaligned_le16(&p[TCM_TOUCH_Y]), true);
		input_report_abs(input, ABS_MT_PRESSURE, p[TCM_TOUCH_Z]);
	}

	input_mt_sync_frame(input);
	input_sync(input);
}

static irqreturn_t tcm_irq(int irq, void *dev_id)
{
	struct tcm *ts = dev_id;

	if (tcm_read(ts))
		return IRQ_HANDLED;

	print_hex_dump_debug("tcm: ", DUMP_PREFIX_OFFSET, 16, 1,
			     ts->rx, sizeof(ts->rx), false);

	if (tcm_code(ts) == TCM_REPORT_TOUCH)
		tcm_report_touch(ts);

	return IRQ_HANDLED;
}

static void tcm_power_off(void *data)
{
	struct tcm *ts = data;

	gpiod_set_value_cansleep(ts->reset, 1);
	regulator_bulk_disable(ARRAY_SIZE(ts->supplies), ts->supplies);
}

static int tcm_power_on(struct tcm *ts)
{
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(ts->supplies), ts->supplies);
	if (ret)
		return ret;

	msleep(TCM_POWER_DELAY_MS);

	if (ts->reset) {
		gpiod_set_value_cansleep(ts->reset, 1);
		fsleep(TCM_RESET_ACTIVE_US);
		gpiod_set_value_cansleep(ts->reset, 0);
		msleep(TCM_RESET_DELAY_MS);
	}

	return 0;
}

static int tcm_setup_input(struct tcm *ts)
{
	struct device *dev = &ts->spi->dev;
	struct input_dev *input;
	int ret;

	input = devm_input_allocate_device(dev);
	if (!input)
		return -ENOMEM;

	ts->input = input;
	input->name = "Synaptics TouchComm touchscreen";
	input->id.bustype = BUS_SPI;

	input_set_abs_params(input, ABS_MT_POSITION_X, 0, ts->max_x, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, ts->max_y, 0, 0);
	input_set_abs_params(input, ABS_MT_PRESSURE, 0, U8_MAX, 0, 0);
	touchscreen_parse_properties(input, true, &ts->prop);

	if (!input_abs_get_max(input, ABS_MT_POSITION_X) ||
	    !input_abs_get_max(input, ABS_MT_POSITION_Y))
		return dev_err_probe(dev, -EINVAL, "no touchscreen size\n");

	ret = input_mt_init_slots(input, 1, INPUT_MT_DIRECT);
	if (ret)
		return ret;

	return input_register_device(input);
}

static int tcm_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct tcm *ts;
	int ret;

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->spi = spi;

	ts->supplies[0].supply = "vdd";
	ts->supplies[1].supply = "avdd";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ts->supplies), ts->supplies);
	if (ret)
		return dev_err_probe(dev, ret, "failed to get supplies\n");

	ts->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ts->reset))
		return dev_err_probe(dev, PTR_ERR(ts->reset), "failed to get reset\n");

	ret = tcm_power_on(ts);
	if (ret)
		return dev_err_probe(dev, ret, "failed to power on\n");

	ret = devm_add_action_or_reset(dev, tcm_power_off, ts);
	if (ret)
		return ret;

	ret = tcm_identify(ts);
	if (ret)
		return dev_err_probe(dev, ret, "identify failed\n");

	ret = tcm_get_app_info(ts);
	if (ret)
		dev_warn(dev, "no application info (%d)\n", ret);

	ret = tcm_enable_touch_report(ts);
	if (ret)
		return dev_err_probe(dev, ret, "failed to enable touch reports\n");

	ret = tcm_setup_input(ts);
	if (ret)
		return ret;

	return devm_request_threaded_irq(dev, spi->irq, NULL, tcm_irq,
					 IRQF_ONESHOT, dev_name(dev), ts);
}

static const struct of_device_id tcm_of_match[] = {
	{ .compatible = "synaptics,s3908" },
	{ }
};
MODULE_DEVICE_TABLE(of, tcm_of_match);

static const struct spi_device_id tcm_spi_ids[] = {
	{ "s3908" },
	{ }
};
MODULE_DEVICE_TABLE(spi, tcm_spi_ids);

static struct spi_driver tcm_driver = {
	.driver = {
		.name = "synaptics-tcm",
		.of_match_table = tcm_of_match,
	},
	.id_table = tcm_spi_ids,
	.probe = tcm_probe,
};
module_spi_driver(tcm_driver);

MODULE_DESCRIPTION("Synaptics TouchComm SPI touchscreen driver");
MODULE_LICENSE("GPL");
