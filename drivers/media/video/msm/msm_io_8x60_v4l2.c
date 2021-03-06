/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/vreg.h>
#include <mach/camera.h>
#include <mach/clk.h>
#include <mach/msm_bus.h>
#include <mach/msm_bus_board.h>

#define BUFF_SIZE_128 128

static struct clk *camio_cam_clk;
static struct clk *camio_jpeg_clk;
static struct clk *camio_vfe_clk;
static struct clk *camio_jpeg_pclk;
static struct clk *camio_vpe_clk;
static struct clk *camio_vpe_pclk;
static struct regulator *fs_ijpeg;
static struct regulator *fs_vpe;
static int camio_cam_open_cnt = 0;

static struct msm_camera_io_clk camio_clk;
static int vpe_clk_rate;
static struct platform_device *camio_dev;
void msm_io_w(u32 data, void __iomem *addr)
{
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	writel_relaxed((data), (addr));
}

void msm_io_w_mb(u32 data, void __iomem *addr)
{
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	wmb();
	writel_relaxed((data), (addr));
	wmb();
}

u32 msm_io_r(void __iomem *addr)
{
	uint32_t data = readl_relaxed(addr);
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	return data;
}

u32 msm_io_r_mb(void __iomem *addr)
{
	uint32_t data;
	rmb();
	data = readl_relaxed(addr);
	rmb();
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	return data;
}

void msm_io_memcpy_toio(void __iomem *dest_addr,
	void __iomem *src_addr, u32 len)
{
	int i;
	u32 *d = (u32 *) dest_addr;
	u32 *s = (u32 *) src_addr;
	/* memcpy_toio does not work. Use writel for now */
	for (i = 0; i < len; i++)
		writel_relaxed(*s++, d++);
}

void msm_io_dump(void __iomem *addr, int size)
{
	char line_str[BUFF_SIZE_128], *p_str;
	int i;
	u32 *p = (u32 *) addr;
	u32 data;
	CDBG("%s: %p %d\n", __func__, addr, size);
	line_str[0] = '\0';
	p_str = line_str;
	for (i = 0; i < size/4; i++) {
		if (i % 4 == 0) {
			snprintf(p_str, 12, "%08x: ", (u32) p);
			p_str += 10;
		}
		data = readl_relaxed(p++);
		snprintf(p_str, 12, "%08x ", data);
		p_str += 9;
		if ((i + 1) % 4 == 0) {
			CDBG("%s\n", line_str);
			line_str[0] = '\0';
			p_str = line_str;
		}
	}
	if (line_str[0] != '\0')
		CDBG("%s\n", line_str);
}

void msm_io_memcpy(void __iomem *dest_addr, void __iomem *src_addr, u32 len)
{
	CDBG("%s: %p %p %d\n", __func__, dest_addr, src_addr, len);
	msm_io_memcpy_toio(dest_addr, src_addr, len / 4);
	msm_io_dump(dest_addr, len);
}

static int msm_camera_vreg_enable(struct platform_device *pdev)
{
#if 0 /* HTC_START Hayden Huang 20111005 */
	if (mipi_csi_vdd == NULL) {
		mipi_csi_vdd = regulator_get(&pdev->dev, "mipi_csi_vdd");
		if (IS_ERR(mipi_csi_vdd)) {
			CDBG("%s: VREG MIPI CSI VDD get failed\n", __func__);
			mipi_csi_vdd = NULL;
			return -ENODEV;
		}
		if (regulator_set_voltage(mipi_csi_vdd, CAM_CSI_VDD_MINUV,
			CAM_CSI_VDD_MAXUV)) {
			CDBG("%s: VREG MIPI CSI VDD set voltage failed\n",
				__func__);
			goto mipi_csi_vdd_put;
		}
		if (regulator_set_optimum_mode(mipi_csi_vdd,
			CAM_CSI_LOAD_UA) < 0) {
			CDBG("%s: VREG MIPI CSI set optimum mode failed\n",
				__func__);
			goto mipi_csi_vdd_release;
		}
		if (regulator_enable(mipi_csi_vdd)) {
			CDBG("%s: VREG MIPI CSI VDD enable failed\n",
				__func__);
			goto mipi_csi_vdd_disable;
		}
	}
	if (cam_vana == NULL) {
		cam_vana = regulator_get(&pdev->dev, "cam_vana");
		if (IS_ERR(cam_vana)) {
			CDBG("%s: VREG CAM VANA get failed\n", __func__);
			cam_vana = NULL;
			goto mipi_csi_vdd_disable;
		}
		if (regulator_set_voltage(cam_vana, CAM_VANA_MINUV,
			CAM_VANA_MAXUV)) {
			CDBG("%s: VREG CAM VANA set voltage failed\n",
				__func__);
			goto cam_vana_put;
		}
		if (regulator_set_optimum_mode(cam_vana,
			CAM_VANA_LOAD_UA) < 0) {
			CDBG("%s: VREG CAM VANA set optimum mode failed\n",
				__func__);
			goto cam_vana_release;
		}
		if (regulator_enable(cam_vana)) {
			CDBG("%s: VREG CAM VANA enable failed\n", __func__);
			goto cam_vana_disable;
		}
	}
	if (cam_vio == NULL) {
		cam_vio = regulator_get(&pdev->dev, "cam_vio");
		if (IS_ERR(cam_vio)) {
			CDBG("%s: VREG VIO get failed\n", __func__);
			cam_vio = NULL;
			goto cam_vana_disable;
		}
		if (regulator_enable(cam_vio)) {
			CDBG("%s: VREG VIO enable failed\n", __func__);
			goto cam_vio_put;
		}
	}
	if (cam_vdig == NULL) {
		cam_vdig = regulator_get(&pdev->dev, "cam_vdig");
		if (IS_ERR(cam_vdig)) {
			CDBG("%s: VREG CAM VDIG get failed\n", __func__);
			cam_vdig = NULL;
			goto cam_vio_disable;
		}
		if (regulator_set_voltage(cam_vdig, CAM_VDIG_MINUV,
			CAM_VDIG_MAXUV)) {
			CDBG("%s: VREG CAM VDIG set voltage failed\n",
				__func__);
			goto cam_vdig_put;
		}
		if (regulator_set_optimum_mode(cam_vdig,
			CAM_VDIG_LOAD_UA) < 0) {
			CDBG("%s: VREG CAM VDIG set optimum mode failed\n",
				__func__);
			goto cam_vdig_release;
		}
		if (regulator_enable(cam_vdig)) {
			CDBG("%s: VREG CAM VDIG enable failed\n", __func__);
			goto cam_vdig_disable;
		}
	}
	if (cam_vaf == NULL) {
		cam_vaf = regulator_get(&pdev->dev, "cam_vaf");
		if (IS_ERR(cam_vaf)) {
			CDBG("%s: VREG CAM VAF get failed\n", __func__);
			cam_vaf = NULL;
			goto cam_vdig_disable;
		}
		if (regulator_set_voltage(cam_vaf, CAM_VAF_MINUV,
			CAM_VAF_MAXUV)) {
			CDBG("%s: VREG CAM VAF set voltage failed\n",
				__func__);
			goto cam_vaf_put;
		}
		if (regulator_set_optimum_mode(cam_vaf,
			CAM_VAF_LOAD_UA) < 0) {
			CDBG("%s: VREG CAM VAF set optimum mode failed\n",
				__func__);
			goto cam_vaf_release;
		}
		if (regulator_enable(cam_vaf)) {
			CDBG("%s: VREG CAM VAF enable failed\n", __func__);
			goto cam_vaf_disable;
		}
	}
#endif /* HTC_END Hayden Huang 20111005 */
	return 0;

#if 0 /* HTC_START Hayden Huang 20111005 */
cam_vaf_disable:
	regulator_set_optimum_mode(cam_vaf, 0);
cam_vaf_release:
	regulator_set_voltage(cam_vaf, 0, CAM_VAF_MAXUV);
	regulator_disable(cam_vaf);
cam_vaf_put:
	regulator_put(cam_vaf);
	cam_vaf = NULL;
cam_vdig_disable:
	regulator_set_optimum_mode(cam_vdig, 0);
cam_vdig_release:
	regulator_set_voltage(cam_vdig, 0, CAM_VDIG_MAXUV);
	regulator_disable(cam_vdig);
cam_vdig_put:
	regulator_put(cam_vdig);
	cam_vdig = NULL;
cam_vio_disable:
	regulator_disable(cam_vio);
cam_vio_put:
	regulator_put(cam_vio);
	cam_vio = NULL;
cam_vana_disable:
	regulator_set_optimum_mode(cam_vana, 0);
cam_vana_release:
	regulator_set_voltage(cam_vana, 0, CAM_VANA_MAXUV);
	regulator_disable(cam_vana);
cam_vana_put:
	regulator_put(cam_vana);
	cam_vana = NULL;
mipi_csi_vdd_disable:
	regulator_set_optimum_mode(mipi_csi_vdd, 0);
mipi_csi_vdd_release:
	regulator_set_voltage(mipi_csi_vdd, 0, CAM_CSI_VDD_MAXUV);
	regulator_disable(mipi_csi_vdd);

mipi_csi_vdd_put:
	regulator_put(mipi_csi_vdd);
	mipi_csi_vdd = NULL;
	return -ENODEV;
#endif /* HTC_END Hayden Huang 20111005 */
}

static void msm_camera_vreg_disable(void)
{
#if 0 /* HTC_START Hayden Huang 20111005 */
	if (mipi_csi_vdd) {
		regulator_set_voltage(mipi_csi_vdd, 0, CAM_CSI_VDD_MAXUV);
		regulator_set_optimum_mode(mipi_csi_vdd, 0);
		regulator_disable(mipi_csi_vdd);
		regulator_put(mipi_csi_vdd);
		mipi_csi_vdd = NULL;
	}

	if (cam_vana) {
		regulator_set_voltage(cam_vana, 0, CAM_VANA_MAXUV);
		regulator_set_optimum_mode(cam_vana, 0);
		regulator_disable(cam_vana);
		regulator_put(cam_vana);
		cam_vana = NULL;
	}

	if (cam_vio) {
		regulator_disable(cam_vio);
		regulator_put(cam_vio);
		cam_vio = NULL;
	}

	if (cam_vdig) {
		regulator_set_voltage(cam_vdig, 0, CAM_VDIG_MAXUV);
		regulator_set_optimum_mode(cam_vdig, 0);
		regulator_disable(cam_vdig);
		regulator_put(cam_vdig);
		cam_vdig = NULL;
	}

	if (cam_vaf) {
		regulator_set_voltage(cam_vaf, 0, CAM_VAF_MAXUV);
		regulator_set_optimum_mode(cam_vaf, 0);
		regulator_disable(cam_vaf);
		regulator_put(cam_vaf);
		cam_vaf = NULL;
	}
#endif /* HTC_END Hayden Huang 20111005 */
}

int msm_camio_clk_enable(enum msm_camio_clk_type clktype)
{
	int rc = 0;
	struct clk *clk = NULL;
	pr_info("[CAM] %s clktype:%d", __func__, clktype);
	switch (clktype) {
	case CAMIO_CAM_MCLK_CLK:
		camio_cam_open_cnt++;
		if (camio_cam_open_cnt > 1)
			return 0;
		camio_cam_clk =
		clk = clk_get(NULL, "cam_clk");
		pr_info("[CAM] %s clk:0x%x", __func__, (unsigned int)clk);
		msm_camio_clk_rate_set_2(clk, camio_clk.mclk_clk_rate);
		break;

	case CAMIO_JPEG_CLK:
		camio_jpeg_clk =
		clk = clk_get(NULL, "ijpeg_clk");
		msm_camio_clk_rate_set_2(clk, 228571000);
		break;

	case CAMIO_JPEG_PCLK:
		camio_jpeg_pclk =
		clk = clk_get(NULL, "ijpeg_pclk");
		break;

	case CAMIO_VPE_CLK:
		camio_vpe_clk =
		clk = clk_get(NULL, "vpe_clk");
		vpe_clk_rate = clk_round_rate(camio_vpe_clk, vpe_clk_rate);
		clk_set_rate(camio_vpe_clk, vpe_clk_rate);
		break;

	case CAMIO_VPE_PCLK:
		camio_vpe_pclk =
		clk = clk_get(NULL, "vpe_pclk");
		break;

	default:
		break;
	}

	if (!IS_ERR(clk)) {
		clk_prepare(clk);
		clk_enable(clk);
	} else {
		rc = -1;
	}
	return rc;
}

int msm_camio_clk_disable(enum msm_camio_clk_type clktype)
{
	int rc = 0;
	struct clk *clk = NULL;
	pr_info("[CAM] %s clktype:%d", __func__, clktype);

	switch (clktype) {
	case CAMIO_CAM_MCLK_CLK:
		camio_cam_open_cnt--;
		if (camio_cam_open_cnt > 0)
			return 0;
		if (camio_cam_open_cnt < 0){
			camio_cam_open_cnt = 0;
			return 0;
		}
		clk = camio_cam_clk;
		break;

	case CAMIO_JPEG_CLK:
		clk = camio_jpeg_clk;
		break;

	case CAMIO_JPEG_PCLK:
		clk = camio_jpeg_pclk;
		break;

	case CAMIO_VPE_CLK:
		clk = camio_vpe_clk;
		break;

	case CAMIO_VPE_PCLK:
		clk = camio_vpe_pclk;
		break;

	default:
		break;
	}

	if (!IS_ERR(clk)) {
		clk_disable(clk);
		clk_put(clk);
	} else
		rc = -1;
	return rc;
}

int msm_camio_vfe_clk_rate_set(int rate)
{
	int rc = 0;
	struct clk *clk = camio_vfe_clk;
	if (rate > clk_get_rate(clk))
		rc = clk_set_rate(clk, rate);
	return rc;
}

void msm_camio_clk_rate_set(int rate)
{
	struct clk *clk = camio_cam_clk;
	clk_set_rate(clk, rate);
}

void msm_camio_clk_rate_set_2(struct clk *clk, int rate)
{
	clk_set_rate(clk, rate);
}

int msm_camio_jpeg_clk_disable(void)
{
	int rc = 0;
	if (fs_ijpeg) {
		rc = regulator_disable(fs_ijpeg);
		if (rc < 0) {
			CDBG("%s: Regulator disable failed %d\n", __func__, rc);
			return rc;
		}
		regulator_put(fs_ijpeg);
	}
	rc = msm_camio_clk_disable(CAMIO_JPEG_PCLK);
	if (rc < 0)
		return rc;
	rc = msm_camio_clk_disable(CAMIO_JPEG_CLK);
	CDBG("%s: exit %d\n", __func__, rc);
	return rc;
}

int msm_camio_jpeg_clk_enable(void)
{
	int rc = 0;
	rc = msm_camio_clk_enable(CAMIO_JPEG_CLK);
	if (rc < 0)
		return rc;
	rc = msm_camio_clk_enable(CAMIO_JPEG_PCLK);
	if (rc < 0)
		return rc;
	fs_ijpeg = regulator_get(NULL, "fs_ijpeg");
	if (IS_ERR(fs_ijpeg)) {
		CDBG("%s: Regulator FS_IJPEG get failed %ld\n", __func__,
			PTR_ERR(fs_ijpeg));
		fs_ijpeg = NULL;
	} else if (regulator_enable(fs_ijpeg)) {
		CDBG("%s: Regulator FS_IJPEG enable failed\n", __func__);
		regulator_put(fs_ijpeg);
	}
	CDBG("%s: exit %d\n", __func__, rc);
	return rc;
}

static int config_gpio_table(int gpio_en)
{
	struct msm_camera_sensor_info *sinfo = camio_dev->dev.platform_data;
	struct msm_camera_gpio_conf *gpio_conf = sinfo->gpio_conf;
	int rc = 0, i = 0;
	return 0; //HTC RMDBG FIXME!!! 
#if 1 /* HTC_START Hayden Huang 20111005 */
	if (gpio_conf == NULL || gpio_conf->cam_gpio_tbl == NULL) {
		pr_err("%s: Invalid NULL cam gpio config table\n", __func__);
		return -EFAULT;
	}

	if (gpio_en) {
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++) {
			rc = gpio_request(gpio_conf->cam_gpio_tbl[i],
				 "CAM_GPIO");
			if (rc < 0) {
				pr_err("%s not able to get gpio\n", __func__);
				for (i--; i >= 0; i--)
					gpio_free(gpio_conf->cam_gpio_tbl[i]);
					break;
			}
		}
	} else {
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++)
			gpio_free(gpio_conf->cam_gpio_tbl[i]);
	}
#else /* QCT gpio function */
	if (gpio_conf->cam_gpio_tbl == NULL || gpio_conf->cam_gpiomux_conf_tbl
		== NULL) {
		pr_err("%s: Invalid NULL cam gpio config table\n", __func__);
		return -EFAULT;
	}

	if (gpio_en) {
		msm_gpiomux_install((struct msm_gpiomux_config *)gpio_conf->
			cam_gpiomux_conf_tbl,
			gpio_conf->cam_gpiomux_conf_tbl_size);
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++) {
			rc = gpio_request(gpio_conf->cam_gpio_tbl[i],
				 "CAM_GPIO");
			if (rc < 0) {
				pr_err("%s not able to get gpio\n", __func__);
				for (i--; i >= 0; i--)
					gpio_free(gpio_conf->cam_gpio_tbl[i]);
					break;
			}
		}
	} else {
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++)
			gpio_free(gpio_conf->cam_gpio_tbl[i]);
	}
#endif /* HTC_END Hayden Huang 20111005 */
	return rc;
}

int msm_camio_vpe_clk_disable(void)
{
	int rc = 0;
	if (fs_vpe) {
		regulator_disable(fs_vpe);
		regulator_put(fs_vpe);
	}

	rc = msm_camio_clk_disable(CAMIO_VPE_CLK);
	if (rc < 0)
		return rc;
	rc = msm_camio_clk_disable(CAMIO_VPE_PCLK);
	return rc;
}


int msm_camio_sensor_clk_on(struct platform_device *pdev)
{
	int rc = 0;
	struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;
	camio_dev = pdev;
	camio_clk = camdev->ioclk;

	msm_camera_vreg_enable(pdev);

//	rc = camdev->camera_csi_on();
	if (rc < 0)
		pr_info("%s camera_csi_on failed\n", __func__);
#if 0 //HTC
	if (sinfo->sensor_platform_info->privacy_light) {
		struct msm8960_privacy_light_cfg *privacy_light_config =
			sinfo->sensor_platform_info->privacy_light_info;
		pm8xxx_mpp_config(privacy_light_config->mpp,
						  &privacy_light_on_config);
	}
#endif
	msleep(20);
	rc = config_gpio_table(1);

	return rc;
}

int msm_camio_sensor_clk_off(struct platform_device *pdev)
{
	int rc = 0;
//	struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
//	struct msm_camera_device_platform_data *camdev = sinfo->pdata;

	msm_camera_vreg_disable();

//	rc = camdev->camera_csi_off();
	if (rc < 0)
		pr_info("%s camera_csi_off failed\n", __func__);
#if 0 //HTC
	if (sinfo->sensor_platform_info->privacy_light) {
		struct msm8960_privacy_light_cfg *privacy_light_config =
			sinfo->sensor_platform_info->privacy_light_info;
		pm8xxx_mpp_config(privacy_light_config->mpp,
						  &privacy_light_off_config);
	}
#endif
	rc = config_gpio_table(0);

	return rc;
}

int msm_camio_vpe_clk_enable(uint32_t clk_rate)
{
	int rc = 0;
	fs_vpe = regulator_get(NULL, "fs_vpe");
	if (IS_ERR(fs_vpe)) {
		CDBG("%s: Regulator FS_VPE get failed %ld\n", __func__,
			PTR_ERR(fs_vpe));
		fs_vpe = NULL;
	} else if (regulator_enable(fs_vpe)) {
		CDBG("%s: Regulator FS_VPE enable failed\n", __func__);
		regulator_put(fs_vpe);
	}

	vpe_clk_rate = clk_rate;
	rc = msm_camio_clk_enable(CAMIO_VPE_CLK);
	if (rc < 0)
		return rc;

	rc = msm_camio_clk_enable(CAMIO_VPE_PCLK);
	return rc;
}

void msm_camio_vfe_blk_reset(void)
{
	return;
}

int msm_camio_probe_on(struct platform_device *pdev)
{
	int rc = 0;
	struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;
	camio_dev = pdev;
	camio_clk = camdev->ioclk;

	rc = config_gpio_table(1);
	if (rc < 0)
		return rc;

	msm_camera_vreg_enable(pdev);
//	rc = camdev->camera_csi_on();
	if (rc < 0)
		pr_info("%s camera_csi_on failed\n", __func__);

	return rc;
}

int msm_camio_probe_off(struct platform_device *pdev)
{
	int rc = 0;
//	struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
//	struct msm_camera_device_platform_data *camdev = sinfo->pdata;

	msm_camera_vreg_disable();

//	rc = camdev->camera_csi_off();
	if (rc < 0)
		pr_info("%s camera_csi_off failed\n", __func__);

	rc = config_gpio_table(0);

	return rc;
}
void msm_camio_bus_scale_cfg(struct msm_bus_scale_pdata *cam_bus_scale_table,
		enum msm_bus_perf_setting perf_setting)
{
	static uint32_t bus_perf_client;
	int rc = 0;
	switch (perf_setting) {
	case S_INIT:
		bus_perf_client =
			msm_bus_scale_register_client(cam_bus_scale_table);
		if (!bus_perf_client) {
			pr_err("%s: Registration Failed!!!\n", __func__);
			bus_perf_client = 0;
			return;
		}
		CDBG("%s: S_INIT rc = %u\n", __func__, bus_perf_client);
		break;
	case S_EXIT:
		if (bus_perf_client) {
			CDBG("%s: S_EXIT\n", __func__);
			msm_bus_scale_unregister_client(bus_perf_client);
		} else
			pr_err("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_PREVIEW:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 1);
			CDBG("%s: S_PREVIEW rc = %d\n", __func__, rc);
		} else
			pr_err("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_VIDEO:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 2);
			CDBG("%s: S_VIDEO rc = %d\n", __func__, rc);
		} else
			pr_err("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_CAPTURE:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 3);
			CDBG("%s: S_CAPTURE rc = %d\n", __func__, rc);
		} else
			pr_err("%s: Bus Client NOT Registered!!!\n", __func__);
		break;

	case S_ZSL:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 4);
			CDBG("%s: S_ZSL rc = %d\n", __func__, rc);
		} else
			pr_err("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_STEREO_VIDEO:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 5);
			CDBG("%s: S_STEREO_VIDEO rc = %d\n", __func__, rc);
		} else
			pr_err("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_STEREO_CAPTURE:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 6);
			CDBG("%s: S_STEREO_VIDEO rc = %d\n", __func__, rc);
		} else
			pr_err("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_DEFAULT:
		break;
	default:
		pr_warning("%s: INVALID CASE\n", __func__);
	}
}
