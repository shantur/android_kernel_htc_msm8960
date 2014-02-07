/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/mach-types.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>
#include <asm/setup.h>
#include "devices-msm8x60.h"
#include "devices.h"
#include "board-pyramid.h"
#include <media/msm_camera.h>

#include <linux/spi/spi.h>
#include <mach/rpm-regulator.h>

#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#include <linux/leds.h>
#endif

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[GPIO] %s: gpio_tlmm_config(%#x)=%d\n",
					__func__, table[n], rc);
			break;
		}
	}
}

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 566231040,
		.ib  = 905969664,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 706199040,
		.ib  = 1129918464,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 320864256,
		.ib  = 513382810,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 320864256,
		.ib  = 513382810,
	},
};

static struct msm_bus_vectors cam_stereo_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 212336640,
		.ib  = 339738624,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 25090560,
		.ib  = 40144896,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 239708160,
		.ib  = 383533056,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 79902720,
		.ib  = 127844352,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_stereo_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 300902400,
		.ib  = 481443840,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 230307840,
		.ib  = 368492544,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 245113344,
		.ib  = 392181351,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 106536960,
		.ib  = 170459136,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 106536960,
		.ib  = 170459136,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_video_vectors),
		cam_stereo_video_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_snapshot_vectors),
		cam_stereo_snapshot_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

struct resource msm_camera_resources[] = {
	{
		.start	= 0x04500000,
		.end	= 0x04500000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= VFE_IRQ,
		.end	= VFE_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(137, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM1_RST# */
	GPIO_CFG(138, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM2_RST# */
	GPIO_CFG(140, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM2_PWDN */
	GPIO_CFG(32, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),	/* CAM_MCLK */
	GPIO_CFG(PYRAMID_CAM_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA),		/* CAM_I2C_SDA */
	GPIO_CFG(PYRAMID_CAM_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),		/* CAM_I2C_SCL */
	GPIO_CFG(141, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM_SEL */
	GPIO_CFG(PYRAMID_CAM_CAM1_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM_CAM1_ID */
};

static uint32_t camera_on_gpio_table_workaround[] = {
	GPIO_CFG(PYRAMID_CAM_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA),		/* CAM_I2C_SDA */
	GPIO_CFG(PYRAMID_CAM_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA),		/* CAM_I2C_SCL */
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(PYRAMID_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA),		/* CAM_I2C_SDA */
	GPIO_CFG(PYRAMID_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),		/* CAM_I2C_SCL */
	GPIO_CFG(32, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),	/* CAM_MCLK */
	GPIO_CFG(137, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM1_RST# */
	GPIO_CFG(138, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM2_RST# */
	GPIO_CFG(140, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM2_PWDN */
	GPIO_CFG(141, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM_SEL */
	GPIO_CFG(PYRAMID_CAM_CAM1_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CAM_CAM1_ID */
};

static int pyramid_config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
	return 0;
}

static void pyramid_config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
	.camera_gpio_on  = pyramid_config_camera_on_gpios,
	.camera_gpio_off = pyramid_config_camera_off_gpios,
	.ioext.csiphy = 0x04800000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = CSI_0_IRQ,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 228570000,
	.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
	.camera_gpio_on  = pyramid_config_camera_on_gpios,
	.camera_gpio_off = pyramid_config_camera_off_gpios,
	.ioext.csiphy = 0x04900000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = CSI_1_IRQ,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 228570000,
	.cam_bus_scale_table = &cam_bus_client_pdata,

	},
};

static int camera_sensor_power_enable(char *power, unsigned volt)
{
	struct regulator *sensor_power;
	int rc;
	if (power == NULL)
		return -ENODEV;

	sensor_power = regulator_get(NULL, power);
	if (IS_ERR(sensor_power)) {
		pr_err("[CAM] %s: Unable to get %s\n", __func__, power);
		return -ENODEV;
	}
	rc = regulator_set_voltage(sensor_power, volt, volt);
	if (rc) {
		pr_err("[CAM] %s: unable to set %s voltage to %d rc:%d\n",
			__func__, power, volt, rc);
	}
	rc = regulator_enable(sensor_power);
	if (rc)
		pr_err("[CAM] %s: Enable regulator %s failed\n", __func__, power);

	regulator_put(sensor_power);
	return rc;
}


static int camera_sensor_power_disable(char *power)
{
	struct regulator *sensor_power;
	int rc;
	if (power == NULL)
		return -ENODEV;

	sensor_power = regulator_get(NULL, power);
	if (IS_ERR(sensor_power)) {
		pr_err("[CAM] %s: Unable to get %s\n", __func__, power);
		return -ENODEV;
	}
	rc = regulator_disable(sensor_power);
	if (rc)
		pr_err("[CAM] %s: Enable regulator %s failed\n", __func__, power);

	regulator_put(sensor_power);
	return rc;

}


static int Pyramid_sensor_vreg_off(void)
{
	int rc;
	pr_info("[CAM] %s\n", __func__);
	/* main / 2nd camera digital power */
	rc = camera_sensor_power_disable("8058_l9");
	/*pr_info("[CAM] sensor_power_disable(\"8058_l9\") == %d\n", rc);*/

	/* main / 2nd camera analog power */
	rc = camera_sensor_power_disable("8058_l15");
	/*pr_info("[CAM] sensor_power_disable(\"8058_l15\") == %d\n", rc);*/

	/* IO power off */
	rc = camera_sensor_power_disable("8058_l12");
	/*pr_info("[CAM] sensor_power_disable(\"8058_l12\") == %d\n", rc);*/

	/* main camera VCM power */
	rc = camera_sensor_power_disable("8058_l10");
	/*pr_info("[CAM] sensor_power_disable(\"8058_l10\") == %d\n", rc);*/

	mdelay(20);

	return rc;
}


static int Pyramid_sensor_vreg_on(void)
{
	static int first_run = 1;
	int rc;
	pr_info("[CAM] %s\n", __func__);
	/* Work-around for PYD power issue */
	if (first_run == 1) {
		first_run = 0;

		config_gpio_table(camera_on_gpio_table_workaround,
			ARRAY_SIZE(camera_on_gpio_table_workaround));

		mdelay(10);

		/* main camera VCM power */
		rc = camera_sensor_power_enable("8058_l10", 2850000);
		/*pr_info("[CAM] sensor_power_enable(\"8058_l10\", 2850) == %d\n", rc);*/
		/*IO*/
		rc = camera_sensor_power_enable("8058_l12", 1800000);
		/*pr_info("[CAM] sensor_power_enable(\"8058_l12\", 1800) == %d\n", rc);*/
		udelay(50);
		/* main / 2nd camera analog power */
		rc = camera_sensor_power_enable("8058_l15", 2800000);
		/*pr_info("[CAM] sensor_power_enable(\"8058_l15\", 2850) == %d\n", rc);*/
		udelay(50);
		/* main / 2nd camera digital power */
		rc = camera_sensor_power_enable("8058_l9", 1800000);
		/*pr_info("[CAM] sensor_power_enable(\"8058_l9\", 1800) == %d\n", rc);*/

		mdelay(20);
		pr_info("[CAM] call Pyramid_sensor_vreg_off() at first boot up !!!\n");
		Pyramid_sensor_vreg_off();
	}

	/* main camera VCM power */
	rc = camera_sensor_power_enable("8058_l10", 2850000);
	/*pr_info("[CAM] sensor_power_enable(\"8058_l10\", 2850) == %d\n", rc);*/
	/*IO*/
	rc = camera_sensor_power_enable("8058_l12", 1800000);
	/*pr_info("[CAM] sensor_power_enable(\"8058_l12\", 1800) == %d\n", rc);*/
	udelay(50);
	/* main / 2nd camera analog power */
	rc = camera_sensor_power_enable("8058_l15", 2800000);
	/*pr_info("[CAM] sensor_power_enable(\"8058_l15\", 2850) == %d\n", rc);*/
	udelay(50);
	/* main / 2nd camera digital power */
	rc = camera_sensor_power_enable("8058_l9", 1800000);
	/*pr_info("[CAM] sensor_power_enable(\"8058_l9\", 1800) == %d\n", rc);*/

	mdelay(1);

	return rc;
}


#define CLK_SWITCH 141
static void Pyramid_maincam_clk_switch(void)
{
	int rc = 0;
	pr_info("[CAM] Doing clk switch (Main Cam)\n");
	rc = gpio_request(CLK_SWITCH, "s5k3h1gx");
	if (rc < 0)
		pr_err("[CAM] GPIO (%d) request fail\n", CLK_SWITCH);
	else
		gpio_direction_output(CLK_SWITCH, 0);
	gpio_free(CLK_SWITCH);
	return;
}

static void Pyramid_seccam_clk_switch(void)
{
	int rc = 0;
	pr_info("[CAM] Doing clk switch (2nd Cam)\n");
	rc = gpio_request(CLK_SWITCH, "mt9v113");

	if (rc < 0)
		pr_err("[CAM] GPIO (%d) request fail\n", CLK_SWITCH);
	else
		gpio_direction_output(CLK_SWITCH, 1);

	gpio_free(CLK_SWITCH);
	return;
}

int aat_flashlight_control(int mode);
int flashlight_control(int mode)
{
#ifdef CONFIG_FLASHLIGHT_AAT
	return aat_flashlight_control(mode);
#else
	return 0;
#endif
}

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type				= MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash				= flashlight_control,
};

static struct msm_camera_sensor_flash_data flash_s5k3h1gx = {
	.flash_type		= MSM_CAMERA_FLASH_LED,
	.flash_src		= &msm_flash_src
};

static struct camera_flash_cfg msm_camera_sensor_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 30,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h1gx_data = {
	.sensor_name	= "s5k3h1gx",
	.sensor_reset	= 137,/*Main Cam RST*/
	.sensor_pwd		= 137,/*139,*/ /*Main Cam PWD*/
	.vcm_pwd		= 58,/*VCM_PD*/
	.vcm_enable		= 0,
	.camera_power_on = Pyramid_sensor_vreg_on,
	.camera_power_off = Pyramid_sensor_vreg_off,
	.camera_clk_switch = Pyramid_maincam_clk_switch,
	.pdata			= &msm_camera_csi_device_data[0],
	.resource		= msm_camera_resources,
	.num_resources	= ARRAY_SIZE(msm_camera_resources),
	.flash_data		= &flash_s5k3h1gx,
	.flash_cfg = &msm_camera_sensor_flash_cfg,
	.power_down_disable = false, /* true: disable pwd down function */
	.mirror_mode = 1,
	.cam_select_pin = CLK_SWITCH,
	.csi_if			= 1,
	.dev_node		= 0
};

static struct platform_device msm_camera_sensor_s5k3h1gx = {
	.name	= "msm_camera_s5k3h1gx",
	.dev	= {
		.platform_data = &msm_camera_sensor_s5k3h1gx_data,
	},
};

static struct msm_camera_sensor_flash_data flash_mt9v113 = {
	.flash_type		= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9v113_data = {
	.sensor_name	= "mt9v113",
	.sensor_reset	= 138,/*2nd Cam RST*/
	.sensor_pwd		= 140,/*2nd Cam PWD*/
	.camera_clk_switch = Pyramid_seccam_clk_switch,
	.camera_power_on = Pyramid_sensor_vreg_on,
	.camera_power_off = Pyramid_sensor_vreg_off,
	.pdata			= &msm_camera_csi_device_data[1],
	.resource		= msm_camera_resources,
	.num_resources	= ARRAY_SIZE(msm_camera_resources),
	.flash_data             = &flash_mt9v113,
	.power_down_disable = false, /* true: disable pwd down function */
	.mirror_mode = 1,
	.cam_select_pin = CLK_SWITCH,
	.csi_if			= 1,
	.dev_node		= 1
};

static struct platform_device msm_camera_sensor_mt9v113 = {
	.name	= "msm_camera_mt9v113",
	.dev	= {
		.platform_data = &msm_camera_sensor_mt9v113_data,
	},
};

#ifdef CONFIG_I2C
static struct i2c_board_info msm_camera_boardinfo[] = {
#ifdef CONFIG_S5K3H1GX
	{
          I2C_BOARD_INFO("s5k3h1gx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9V113
	{
          I2C_BOARD_INFO("mt9v113", 0x3C),
	},
#endif
};

#endif 

void __init pyramid_init_cam(void)
{
	pr_info("%s", __func__);

        i2c_register_board_info(MSM_GSBI4_QUP_I2C_BUS_ID,
        		msm_camera_boardinfo,
        		ARRAY_SIZE(msm_camera_boardinfo));

        platform_device_register(&msm_camera_sensor_s5k3h1gx);
        platform_device_register(&msm_camera_sensor_mt9v113);
	platform_device_register(&msm_device_csic0);
	platform_device_register(&msm_device_csic1);
	platform_device_register(&msm_device_vfe);
	platform_device_register(&msm_device_vpe);
}


