// SPDX-License-Identifier: GPL-2.0-only

#include <linux/aperture.h>
#include <linux/bits.h>
#include <linux/io.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>

#include <drm/clients/drm_client_setup.h>
#include <drm/drm_atomic.h>
#include <drm/drm_connector.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_fbdev_shmem.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_managed.h>
#include <drm/drm_modeset_helper.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>

#include "drm_sysfb_helper.h"

#define DRIVER_NAME	"zumapro-decon"
#define DRIVER_DESC	"DRM driver for the ABL-initialised zumapro DECON0 pipeline"
#define DRIVER_MAJOR	1
#define DRIVER_MINOR	0

#define DECON_GLOBAL_CON		0x0020
#define DECON_GLOBAL_CON_RUN_STATUS	BIT(4)
#define DECON_TRIG_CON			0x0030
#define DECON_TRIG_CON_HW_TRIG_EN	BIT(0)
#define DECON_TRIG_CON_SW_TRIG_DET_EN	BIT(1)
#define DECON_TRIG_CON_HW_TRIG_MASK	BIT(4)
#define DECON_TRIG_CON_SW_TRIG_EN	BIT(8)
#define DECON_SHD_REG_UP_REQ		0x0050
#define DECON_SHD_REG_UP_REQ_GLOBAL	BIT(31)

#define RDMA_DMA_SHD_OFFSET		0x0400
#define RDMA_IMG_SIZE			0x001c
#define RDMA_BASEADDR_P0		0x0040
#define RDMA_SRC_STRIDE_0		0x0050
#define RDMA_SRC_STRIDE_0_MASK		GENMASK(23, 0)

struct zumapro_decon {
	struct drm_sysfb_device sysfb;
	void __iomem *decon;
	void __iomem *rdma;
	u32 formats[DRM_SYSFB_PLANE_NFORMATS(1)];
	struct drm_plane primary_plane;
	struct drm_crtc crtc;
	struct drm_encoder encoder;
	struct drm_connector connector;
};

static struct zumapro_decon *to_zumapro_decon(struct drm_device *dev)
{
	return container_of(dev, struct zumapro_decon, sysfb.dev);
}

static u32 rdma_shadow_read(struct zumapro_decon *zd, u32 reg)
{
	return readl(zd->rdma + RDMA_DMA_SHD_OFFSET + reg);
}

static bool decon_is_running(struct zumapro_decon *zd)
{
	return readl(zd->decon + DECON_GLOBAL_CON) & DECON_GLOBAL_CON_RUN_STATUS;
}

/* TODO(will): pace triggers on the DECON frame-done interrupt. */
static void decon_sw_trigger(struct zumapro_decon *zd)
{
	u32 trig = readl(zd->decon + DECON_TRIG_CON);

	trig &= ~(DECON_TRIG_CON_HW_TRIG_EN | DECON_TRIG_CON_HW_TRIG_MASK);
	trig |= DECON_TRIG_CON_SW_TRIG_EN | DECON_TRIG_CON_SW_TRIG_DET_EN;
	writel(trig, zd->decon + DECON_TRIG_CON);
	writel(DECON_SHD_REG_UP_REQ_GLOBAL, zd->decon + DECON_SHD_REG_UP_REQ);
}

static void zumapro_decon_plane_atomic_update(struct drm_plane *plane,
					      struct drm_atomic_commit *state)
{
	drm_sysfb_plane_helper_atomic_update(plane, state);
	decon_sw_trigger(to_zumapro_decon(plane->dev));
}

static void zumapro_decon_plane_atomic_disable(struct drm_plane *plane,
					       struct drm_atomic_commit *state)
{
	drm_sysfb_plane_helper_atomic_disable(plane, state);
	decon_sw_trigger(to_zumapro_decon(plane->dev));
}

static const u64 zumapro_decon_plane_format_modifiers[] = {
	DRM_SYSFB_PLANE_FORMAT_MODIFIERS,
};

static const struct drm_plane_helper_funcs zumapro_decon_plane_helper_funcs = {
	.begin_fb_access = drm_sysfb_plane_helper_begin_fb_access,
	.end_fb_access = drm_gem_end_shadow_fb_access,
	.atomic_check = drm_sysfb_plane_helper_atomic_check,
	.atomic_update = zumapro_decon_plane_atomic_update,
	.atomic_disable = zumapro_decon_plane_atomic_disable,
	.get_scanout_buffer = drm_sysfb_plane_helper_get_scanout_buffer,
};

static const struct drm_plane_funcs zumapro_decon_plane_funcs = {
	DRM_SYSFB_PLANE_FUNCS,
	.destroy = drm_plane_cleanup,
};

static const struct drm_crtc_helper_funcs zumapro_decon_crtc_helper_funcs = {
	DRM_SYSFB_CRTC_HELPER_FUNCS,
};

static const struct drm_crtc_funcs zumapro_decon_crtc_funcs = {
	DRM_SYSFB_CRTC_FUNCS,
	.destroy = drm_crtc_cleanup,
};

static const struct drm_encoder_funcs zumapro_decon_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static const struct drm_connector_helper_funcs zumapro_decon_connector_helper_funcs = {
	DRM_SYSFB_CONNECTOR_HELPER_FUNCS,
};

static const struct drm_connector_funcs zumapro_decon_connector_funcs = {
	DRM_SYSFB_CONNECTOR_FUNCS,
	.destroy = drm_connector_cleanup,
};

static const struct drm_mode_config_funcs zumapro_decon_mode_config_funcs = {
	DRM_SYSFB_MODE_CONFIG_FUNCS,
};

static int zumapro_decon_map_regs(struct zumapro_decon *zd, struct platform_device *pdev)
{
	zd->decon = devm_platform_ioremap_resource_byname(pdev, "decon");
	if (IS_ERR(zd->decon))
		return PTR_ERR(zd->decon);

	zd->rdma = devm_platform_ioremap_resource_byname(pdev, "rdma");
	return PTR_ERR_OR_ZERO(zd->rdma);
}

static int zumapro_decon_read_scanout(struct zumapro_decon *zd, struct resource *fb,
				      const struct drm_format_info *format)
{
	struct drm_device *dev = &zd->sysfb.dev;
	u32 size = rdma_shadow_read(zd, RDMA_IMG_SIZE);
	u32 base = rdma_shadow_read(zd, RDMA_BASEADDR_P0);
	u32 stride = rdma_shadow_read(zd, RDMA_SRC_STRIDE_0) & RDMA_SRC_STRIDE_0_MASK;
	u32 width = size & 0xffff;
	u32 height = size >> 16;

	if (!width || !height) {
		drm_err(dev, "RDMA0 IMG_SIZE is empty (0x%08x)\n", size);
		return -ENODEV;
	}
	if (base != fb->start) {
		drm_err(dev, "RDMA0 scans 0x%08x, memory-region is %pR\n", base, fb);
		return -EINVAL;
	}
	if (!stride)
		stride = width * format->cpp[0];
	if ((u64)stride * height > resource_size(fb)) {
		drm_err(dev, "%ux%u stride %u does not fit %pR\n", width, height, stride, fb);
		return -EINVAL;
	}

	zd->sysfb.fb_mode = drm_sysfb_mode(width, height, 0, 0);
	zd->sysfb.fb_format = format;
	zd->sysfb.fb_pitch = stride;
	drm_info(dev, "inherited DECON0 scanout %ux%u stride %u at %pR\n",
		 width, height, stride, fb);
	return 0;
}

static int zumapro_decon_map_fb(struct zumapro_decon *zd, struct platform_device *pdev,
				struct resource *fb)
{
	struct drm_device *dev = &zd->sysfb.dev;
	resource_size_t len = (resource_size_t)zd->sysfb.fb_pitch *
			      zd->sysfb.fb_mode.vdisplay;
	void __iomem *screen;
	int ret;

	ret = devm_aperture_acquire_for_platform_device(pdev, fb->start, len);
	if (ret) {
		drm_err(dev, "could not acquire framebuffer %pR: %d\n", fb, ret);
		return ret;
	}

	screen = devm_ioremap_wc(&pdev->dev, fb->start, len);
	if (!screen)
		return -ENOMEM;

	iosys_map_set_vaddr_iomem(&zd->sysfb.fb_addr, screen);
	return 0;
}

static int zumapro_decon_init_modeset(struct zumapro_decon *zd)
{
	struct drm_device *dev = &zd->sysfb.dev;
	const struct drm_format_info *format = zd->sysfb.fb_format;
	struct drm_display_mode *mode = &zd->sysfb.fb_mode;
	size_t nformats;
	int ret;

	ret = drmm_mode_config_init(dev);
	if (ret)
		return ret;

	dev->mode_config.min_width = mode->hdisplay;
	dev->mode_config.max_width = max_t(unsigned long, mode->hdisplay,
					   DRM_SHADOW_PLANE_MAX_WIDTH);
	dev->mode_config.min_height = mode->vdisplay;
	dev->mode_config.max_height = max_t(unsigned long, mode->vdisplay,
					    DRM_SHADOW_PLANE_MAX_HEIGHT);
	dev->mode_config.preferred_depth = format->depth;
	dev->mode_config.funcs = &zumapro_decon_mode_config_funcs;

	nformats = drm_sysfb_build_fourcc_list(dev, &format->format, 1,
					       zd->formats, ARRAY_SIZE(zd->formats));

	ret = drm_universal_plane_init(dev, &zd->primary_plane, 0, &zumapro_decon_plane_funcs,
				       zd->formats, nformats,
				       zumapro_decon_plane_format_modifiers,
				       DRM_PLANE_TYPE_PRIMARY, NULL);
	if (ret)
		return ret;
	drm_plane_helper_add(&zd->primary_plane, &zumapro_decon_plane_helper_funcs);
	drm_plane_enable_fb_damage_clips(&zd->primary_plane);

	ret = drm_crtc_init_with_planes(dev, &zd->crtc, &zd->primary_plane, NULL,
					&zumapro_decon_crtc_funcs, NULL);
	if (ret)
		return ret;
	drm_crtc_helper_add(&zd->crtc, &zumapro_decon_crtc_helper_funcs);

	ret = drm_encoder_init(dev, &zd->encoder, &zumapro_decon_encoder_funcs,
			       DRM_MODE_ENCODER_DSI, NULL);
	if (ret)
		return ret;
	zd->encoder.possible_crtcs = drm_crtc_mask(&zd->crtc);

	ret = drm_connector_init(dev, &zd->connector, &zumapro_decon_connector_funcs,
				 DRM_MODE_CONNECTOR_DSI);
	if (ret)
		return ret;
	drm_connector_helper_add(&zd->connector, &zumapro_decon_connector_helper_funcs);

	ret = drm_connector_attach_encoder(&zd->connector, &zd->encoder);
	if (ret)
		return ret;

	drm_mode_config_reset(dev);
	return 0;
}

DEFINE_DRM_GEM_FOPS(zumapro_decon_fops);

static const struct drm_driver zumapro_decon_driver = {
	DRM_GEM_SHMEM_DRIVER_OPS,
	DRM_FBDEV_SHMEM_DRIVER_OPS,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.driver_features = DRIVER_ATOMIC | DRIVER_GEM | DRIVER_MODESET,
	.fops = &zumapro_decon_fops,
};

static int zumapro_decon_probe(struct platform_device *pdev)
{
	/* TODO(will): read the pixel format from RDMA0 instead of assuming it. */
	const struct drm_format_info *format = drm_format_info(DRM_FORMAT_ARGB8888);
	struct zumapro_decon *zd;
	struct drm_device *dev;
	struct resource fb;
	int ret;

	zd = devm_drm_dev_alloc(&pdev->dev, &zumapro_decon_driver, struct zumapro_decon,
				sysfb.dev);
	if (IS_ERR(zd))
		return PTR_ERR(zd);
	dev = &zd->sysfb.dev;
	platform_set_drvdata(pdev, dev);

	ret = zumapro_decon_map_regs(zd, pdev);
	if (ret)
		return ret;

	/* TODO(will): own the DECON0 clocks and power domain, not clk_ignore_unused. */
	if (!decon_is_running(zd))
		return dev_err_probe(&pdev->dev, -ENODEV, "DECON0 is not running\n");

	ret = of_reserved_mem_region_to_resource(pdev->dev.of_node, 0, &fb);
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "no framebuffer memory-region\n");

	ret = zumapro_decon_read_scanout(zd, &fb, format);
	if (ret)
		return ret;

	ret = zumapro_decon_map_fb(zd, pdev, &fb);
	if (ret)
		return ret;

	ret = zumapro_decon_init_modeset(zd);
	if (ret)
		return ret;

	ret = drm_dev_register(dev, 0);
	if (ret)
		return ret;

	drm_client_setup(dev, format);
	return 0;
}

static void zumapro_decon_remove(struct platform_device *pdev)
{
	drm_dev_unplug(platform_get_drvdata(pdev));
}

static const struct of_device_id zumapro_decon_of_match[] = {
	{ .compatible = "google,zumapro-decon" },
	{ }
};
MODULE_DEVICE_TABLE(of, zumapro_decon_of_match);

static struct platform_driver zumapro_decon_platform_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = zumapro_decon_of_match,
	},
	.probe = zumapro_decon_probe,
	.remove = zumapro_decon_remove,
};
module_platform_driver(zumapro_decon_platform_driver);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
