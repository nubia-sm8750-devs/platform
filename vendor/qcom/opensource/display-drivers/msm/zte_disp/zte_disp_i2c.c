#include "zte_disp_i2c.h"

struct i2c_client *i2c_lp8556_client = NULL;
struct i2c_client *i2c_sgm62110s_client = NULL;

bool tilp8556_probe = false;
bool sgm62110s_probe = false;

int zte_disp_read_reg(struct i2c_client *client, u8 reg, u8 *data)
{
	struct i2c_msg msgs[2];
	int ret;
	u8 retries = 0;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 1;
	msgs[0].buf   = &reg;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = 1;
	msgs[1].buf   = data;

	while (retries < 3) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret == 2)
			break;
		retries++;
		msleep_interruptible(5);
	}
	pr_info("[MSM_LCD] i2caddr=0x%x read reg retries=%d,ret=%d\n", client->addr, retries, ret);
	if (ret != 2) {
		pr_err("msm_lcd i2c read transfer error\n");
		ret = -1;
	}

	return ret;
}

int zte_disp_write_reg(struct i2c_client *client, u8 *buf, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = len + 1,
			.buf = buf,
		},
	};

	do {
		err = i2c_transfer(client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(5);
	} while ((err != 1) && (++tries < 3));

	pr_info("[MSM_LCD] i2caddr=0x%x write reg tries=%d,ret=%d\n", client->addr, tries, err);
	if (err != 1) {
		pr_err("i2c write transfer error\n");
		err = -1;
	}

	return err;
}

int lp8556_read(u16 offset)
{
	u8 data = 0x0;
	if(zte_disp_read_reg(i2c_lp8556_client, offset, &data)) {
		pr_info("[MSM_LCD] lp8556 read offset = %x, data = %x\n", offset, data);
	} else {
		pr_info("[MSM_LCD] lp8556 read fail\n");
	}
	return data;
}

int sgm62110s_read(u16 offset)
{
	u8 data = 0x0;
	if(zte_disp_read_reg(i2c_sgm62110s_client, offset, &data)) {
		pr_info("[MSM_LCD] sgm62110s read offset = %x, data = %x\n", offset, data);
	} else {
		pr_info("[MSM_LCD] sgm62110s read fail\n");
	}
	return data;
}

void zte_disp_set_cmds(int offset, int data, int i2c_type)
{
	u8 buf[2] = {offset, data};

	switch (i2c_type)
	{
	case lp8556_device:
		if (tilp8556_probe) {
			pr_info("[MSM_LCD] lp8556 write reg offset = %x, data = %x\n", buf[0], buf[1]);
			zte_disp_write_reg(i2c_lp8556_client, buf, 1);
			usleep_range(1000, 1100);    // Delay for 1000us to 1100us
		}
		break;
	case sgm62110s_device:
		if (sgm62110s_probe) {
			pr_info("[MSM_LCD] sgm62110s write reg offset = %x, data = %x\n", buf[0], buf[1]);
			zte_disp_write_reg(i2c_sgm62110s_client, buf, 1);
			usleep_range(1000, 1100);    // Delay for 1000us to 1100us
		}
		break;
	default:
		break;
	}
}

static int disp_probe(struct i2c_client *client)
{
	const struct i2c_device_id *id = i2c_client_get_device_id(client);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("tilp8556_probe,client not i2c capable\n");
		return -EIO;
	}

	switch (id->driver_data)
	{
	case lp8556_device:
		i2c_lp8556_client = client;
		tilp8556_probe = true;
		pr_info("[MSM_LCD] tilp8556_probe ok\n");
		break;
	case sgm62110s_device:
		i2c_sgm62110s_client = client;
		sgm62110s_probe = true;
		pr_info("[MSM_LCD] sgm62110s_probe ok\n");
		//usleep_range(10000, 10100);
		sgm62110s_read(0x03);
		break;
	default:
		break;
	}
	return 0;
}

static void disp_remove(struct i2c_client *client)
{
	kfree(i2c_get_clientdata(client));
	return;
}

static const struct i2c_device_id disp_i2c_id_table[] = {
	//{"lp8556",  lp8556_device},
	{"sgm62110s", sgm62110s_device},
};

static const struct of_device_id disp_of_id_table[] = {
	//{.compatible = "ti,lp8556"},
	{.compatible = "sgm,sgm62110s"},
	{ },
};

static struct i2c_driver disp_i2c_driver = {
	.probe = disp_probe,
	.remove = disp_remove,
	.id_table = disp_i2c_id_table,
	.driver = {
		.name = "zte_disp_i2c",
		.owner = THIS_MODULE,
		.of_match_table = disp_of_id_table,
	},
};

int disp_i2c_driver_init(void)
{
	return i2c_add_driver(&disp_i2c_driver);
}
EXPORT_SYMBOL(disp_i2c_driver_init);

void disp_i2c_driver_exit(void)
{
	i2c_del_driver(&disp_i2c_driver);
}
EXPORT_SYMBOL(disp_i2c_driver_exit);

