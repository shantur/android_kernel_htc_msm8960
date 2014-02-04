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

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#include <linux/leds.h>
#endif

#define PYRAMID_CAM_I2C_SDA           (47)
#define PYRAMID_CAM_I2C_SCL           (48)
#define PYRAMID_CAM_ID           (135)
#define PYRAMID_GPIO_V_CAM_D1V2_EN     (7)
#define PYRAMID_GPIO_CAM_MCLK     	(32)
#define PYRAMID_GPIO_RAW_RSTN         (49)
#define PYRAMID_GPIO_RAW_INTR0       (106)
#define PYRAMID_GPIO_RAW_INTR1       (105)
#define PYRAMID_GPIO_MCLK_SWITCH     (141)

#define PYRAMID_GPIO_CAM2_RSTz       (138)
#define PYRAMID_GPIO_CAM_PWDN        (107)
#define PYRAMID_GPIO_CAM2_PWDN       (140)
#define PYRAMID_GPIO_CAM_VCM_PD       (58)

#define PYRAMID_GPIO_MCAM_SPI_DO	(33)
#define PYRAMID_GPIO_MCAM_SPI_DI	(34)
#define PYRAMID_GPIO_MCAM_SPI_CLK	(36)
#define PYRAMID_GPIO_MCAM_SPI_CS	(35)

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

#ifdef CONFIG_MSM_CAMERA
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

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
	.ioext.csiphy = 0x04800000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = CSI_0_IRQ,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 266667000,
	.csid_core = 0,
	.is_vpe = 1,
	.cam_bus_scale_table = &cam_bus_client_pdata,

	},
	{
	.ioext.csiphy = 0x04900000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = CSI_1_IRQ,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 266667000,
	.csid_core = 1,
	.is_vpe = 1,
	.cam_bus_scale_table = &cam_bus_client_pdata,

	},
};

//static struct msm_camera_sensor_flash_src msm_flash_src = {
//	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
        //	.camera_flash = flashlight_control,
//};
#endif

#ifdef CONFIG_MT9V113
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(PYRAMID_CAM_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA),
	GPIO_CFG(PYRAMID_CAM_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA),
	GPIO_CFG(PYRAMID_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PYRAMID_GPIO_CAM_MCLK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), 
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(PYRAMID_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(PYRAMID_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(PYRAMID_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(PYRAMID_GPIO_CAM_MCLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
};

int aat1271_flashlight_control(int mode);
int flashlight_control(int mode)
{
#ifdef CONFIG_FLASHLIGHT_AAT1271
	return aat1271_flashlight_control(mode);
#else
	return 0;
#endif
}

static struct gpio msm_cam_gpio_tbl[] = {
  {PYRAMID_GPIO_CAM_MCLK}, 
  {137},
  //  {138},
  //  {140},
  //  {141},
  {PYRAMID_CAM_CAM1_ID},
  {PYRAMID_CAM_I2C_SDA},
  {PYRAMID_CAM_I2C_SCL},
};

static struct msm_gpio_set_tbl mt9v113_gpio_set_tbl[] = {
	{PYRAMID_GPIO_CAM2_RSTz, GPIOF_OUT_INIT_LOW, 10000},
	{PYRAMID_GPIO_CAM2_RSTz, GPIOF_OUT_INIT_HIGH, 5000},
	{PYRAMID_GPIO_MCLK_SWITCH, GPIOF_OUT_INIT_LOW, 10000},
	{PYRAMID_GPIO_MCLK_SWITCH, GPIOF_OUT_INIT_HIGH, 4000},
};

static struct msm_camera_gpio_conf mt9v113_gpio_conf = {
        .camera_on_table = camera_on_gpio_table,
        .camera_off_table = camera_off_gpio_table,
        .camera_on_table_size = ARRAY_SIZE(camera_on_gpio_table),
        .gpio_no_mux = 1,
	.cam_gpio_req_tbl = msm_cam_gpio_tbl,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
	.cam_gpio_set_tbl = mt9v113_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(mt9v113_gpio_set_tbl),
};

static struct camera_vreg_t mt9v113_cam_vreg[] = {
	{"8058_l10", REG_LDO, 2850000, 2850000, -1},
	{"8058_l12", REG_LDO, 1800000, 1800000, -1},
	{"8058_l15", REG_LDO, 2800000, 2800000, -1},
	{"8058_l9", REG_LDO, 1800000, 1800000, -1},
};

static struct msm_camera_sensor_platform_info sensor_mt9v113_board_info = {
	.mount_angle = 270,
        //	.sensor_reset_enable = 1,
        //	.vcm_pwd	= 0,
	.cam_vreg = mt9v113_cam_vreg,
	.num_vreg = ARRAY_SIZE(mt9v113_cam_vreg),
        .gpio_conf = &mt9v113_gpio_conf,
};

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type				= MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
        //	.camera_flash				= flashlight_control,
};

static struct msm_camera_sensor_flash_data flash_mt9v113 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
	.flash_src		= &msm_flash_src
};

//static struct camera_flash_cfg msm_camera_sensor_flash_cfg = {
//	.low_temp_limit		= 5,
//	.low_cap_limit		= 30,
//};

static struct msm_camera_sensor_info msm_camera_sensor_mt9v113_data = {
	.sensor_name	= "mt9v113",
	.sensor_reset	= PYRAMID_GPIO_CAM2_RSTz,
        //        .camera_power_on = pyramid_mt9v113_vreg_on,
        //        .camera_power_off = pyramid_mt9v113_vreg_off,
        .sensor_pwd = PYRAMID_GPIO_CAM2_PWDN,
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_mt9v113,
	.sensor_platform_info = &sensor_mt9v113_board_info,
	.csi_if	= 1,
        .camera_type = FRONT_CAMERA_2D,
        //	.flash_cfg = &msm_camera_sensor_flash_cfg, 
};
#endif  

#ifdef CONFIG_I2C
static struct i2c_board_info msm_camera_boardinfo[] = {
#ifdef CONFIG_S5K3H1GX
	{
          I2C_BOARD_INFO("s5k3h1gx", 0x20 >> 1),
          .platform_data = &msm_camera_sensor_s5k3h1gx_data,
	},
#endif
#ifdef CONFIG_MT9V113
	{
	I2C_BOARD_INFO("mt9v113", 0x3C),
        .platform_data = &msm_camera_sensor_mt9v113_data,
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

        platform_device_register(&msm_camera_server);
	platform_device_register(&msm_device_csic0);
	platform_device_register(&msm_device_csic1);
	platform_device_register(&msm_device_vfe);
	platform_device_register(&msm_device_vpe);
}


