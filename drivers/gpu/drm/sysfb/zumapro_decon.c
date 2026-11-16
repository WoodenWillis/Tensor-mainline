// SPDX-License-Identifier: GPL-2.0-only

#include <linux/aperture.h>
#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>

#include <drm/clients/drm_client_setup.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_fbdev_dma.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_managed.h>
#include <drm/drm_modeset_helper.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_vblank_helper.h>

#include <video/mipi_display.h>

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
#define DECON_TRIG_CON_HW_TRIG_SEL	GENMASK(25, 24)
#define DECON_SHD_REG_UP_REQ		0x0050
#define DECON_SHD_REG_UP_REQ_GLOBAL	BIT(31)
#define DECON_SHD_REG_UP_REQ_WINDOWS	GENMASK(13, 0)
#define DECON_INT_EN			0x0060
#define DECON_INT_EN_INT_EN		BIT(0)
#define DECON_INT_EN_FRAME_START	BIT(12)
#define DECON_INT_PEND			0x0070
#define DECON_INT_PEND_FRAME_START	BIT(12)

#define RDMA_DMA_SHD_OFFSET		0x0400
#define RDMA_IN_CTRL_0			0x0008
#define RDMA_IN_CTRL_0_IMG_FORMAT	GENMASK(13, 8)
#define RDMA_IN_CTRL_0_ALPHA		GENMASK(31, 24)
/* IDMA_IMG_FORMAT_BGRX8888 */
#define RDMA_IMG_FORMAT_XRGB8888	4
#define RDMA_IMG_SIZE			0x001c
#define RDMA_BASEADDR_P0		0x0040
#define RDMA_SRC_STRIDE_0		0x0050
#define RDMA_SRC_STRIDE_0_MASK		GENMASK(23, 0)

#define DSIM_INTSRC			0x0050
#define DSIM_INTSRC_SFR_PH_FIFO_EMPTY	BIT(28)
#define DSIM_INTSRC_SFR_PL_FIFO_EMPTY	BIT(29)
#define DSIM_PKTHDR			0x0058
#define DSIM_PAYLOAD			0x005c
#define DSIM_FIFOCTRL			0x0068
#define DSIM_FIFOCTRL_EMPTY_PH_SFR	BIT(10)

#define CM4_HDISPLAY			1280
#define CM4_VDISPLAY			2856

struct cm4_cmd {
	u8 len;
	u8 data[4];
};

struct zumapro_decon {
	struct drm_sysfb_device sysfb;
	void __iomem *decon;
	void __iomem *rdma;
	void __iomem *dsim;
	u32 abl_base;
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

/* TODO(will): re-mask the trigger at frame start so idle TEs don't refresh from memory. */
static void decon_te_trigger(struct zumapro_decon *zd)
{
	u32 trig = readl(zd->decon + DECON_TRIG_CON);

	trig &= ~(DECON_TRIG_CON_SW_TRIG_EN | DECON_TRIG_CON_SW_TRIG_DET_EN |
		  DECON_TRIG_CON_HW_TRIG_MASK | DECON_TRIG_CON_HW_TRIG_SEL);
	trig |= DECON_TRIG_CON_HW_TRIG_EN;
	writel(trig, zd->decon + DECON_TRIG_CON);
	writel(DECON_SHD_REG_UP_REQ_GLOBAL | DECON_SHD_REG_UP_REQ_WINDOWS,
	       zd->decon + DECON_SHD_REG_UP_REQ);
}

static bool decon_update_is_pending(struct zumapro_decon *zd)
{
	return readl(zd->decon + DECON_SHD_REG_UP_REQ) &
	       (DECON_SHD_REG_UP_REQ_GLOBAL | DECON_SHD_REG_UP_REQ_WINDOWS);
}

static void decon_scan_from(struct zumapro_decon *zd, u32 base)
{
	writel(base, zd->rdma + RDMA_BASEADDR_P0);
	decon_te_trigger(zd);
}

static void rdma_set_xrgb8888(struct zumapro_decon *zd)
{
	u32 ctrl = readl(zd->rdma + RDMA_IN_CTRL_0);

	ctrl &= ~RDMA_IN_CTRL_0_IMG_FORMAT;
	ctrl |= FIELD_PREP(RDMA_IN_CTRL_0_IMG_FORMAT, RDMA_IMG_FORMAT_XRGB8888) |
		RDMA_IN_CTRL_0_ALPHA;
	writel(ctrl, zd->rdma + RDMA_IN_CTRL_0);
}

static void decon_frame_start_irq_mask(struct zumapro_decon *zd)
{
	u32 en = readl(zd->decon + DECON_INT_EN);

	writel(en & ~DECON_INT_EN_FRAME_START, zd->decon + DECON_INT_EN);
}

static void decon_frame_start_irq_unmask(struct zumapro_decon *zd)
{
	u32 en = readl(zd->decon + DECON_INT_EN);

	writel(DECON_INT_PEND_FRAME_START, zd->decon + DECON_INT_PEND);
	writel(en | DECON_INT_EN_FRAME_START | DECON_INT_EN_INT_EN,
	       zd->decon + DECON_INT_EN);
}

static irqreturn_t decon_frame_start_irq(int irq, void *data)
{
	struct zumapro_decon *zd = data;

	if (!(readl(zd->decon + DECON_INT_PEND) & DECON_INT_PEND_FRAME_START))
		return IRQ_NONE;

	writel(DECON_INT_PEND_FRAME_START, zd->decon + DECON_INT_PEND);
	if (!decon_update_is_pending(zd))
		drm_crtc_handle_vblank(&zd->crtc);
	return IRQ_HANDLED;
}

static int zumapro_decon_crtc_enable_vblank(struct drm_crtc *crtc)
{
	decon_frame_start_irq_unmask(to_zumapro_decon(crtc->dev));
	return 0;
}

static void zumapro_decon_crtc_disable_vblank(struct drm_crtc *crtc)
{
	decon_frame_start_irq_mask(to_zumapro_decon(crtc->dev));
}

static void dsim_write_payload(struct zumapro_decon *zd, const u8 *buf, size_t len)
{
	size_t i;
	u32 word;

	for (i = 0; i < len; i += sizeof(word)) {
		word = 0;
		memcpy(&word, buf + i, min(sizeof(word), len - i));
		writel(le32_to_cpu((__force __le32)word), zd->dsim + DSIM_PAYLOAD);
	}
}

static u32 dsim_dcs_header(const u8 *buf, size_t len)
{
	if (len == 1)
		return MIPI_DSI_DCS_SHORT_WRITE | buf[0] << 8;
	if (len == 2)
		return MIPI_DSI_DCS_SHORT_WRITE_PARAM | buf[0] << 8 | buf[1] << 16;
	return MIPI_DSI_DCS_LONG_WRITE | (len & 0xff) << 8 | (len >> 8) << 16;
}

static int dsim_dcs_write(struct zumapro_decon *zd, const u8 *buf, size_t len)
{
	u32 fifo;

	writel(DSIM_INTSRC_SFR_PH_FIFO_EMPTY | DSIM_INTSRC_SFR_PL_FIFO_EMPTY,
	       zd->dsim + DSIM_INTSRC);
	if (len > 2)
		dsim_write_payload(zd, buf, len);
	writel(dsim_dcs_header(buf, len), zd->dsim + DSIM_PKTHDR);

	return readl_poll_timeout(zd->dsim + DSIM_FIFOCTRL, fifo,
				  fifo & DSIM_FIFOCTRL_EMPTY_PH_SFR, 200, 100000);
}

static void cm4_set_refresh(struct zumapro_decon *zd, unsigned int hz)
{
	const u8 rate = hz == 60 ? 0x01 : 0x00;
	const struct cm4_cmd cmds[] = {
		{ 3, { 0xf0, 0x5a, 0x5a } },
		{ 4, { 0xb0, 0x00, 0x02, 0xb9 } },
		{ 2, { 0xb9, rate } },
		{ 2, { 0xbd, 0x21 } },
		{ 2, { 0x60, rate } },
		{ 2, { 0xf7, 0x0f } },
		{ 3, { 0xf0, 0xa5, 0xa5 } },
	};
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		ret = dsim_dcs_write(zd, cmds[i].data, cmds[i].len);
		if (ret) {
			drm_err(&zd->sysfb.dev, "cm4: DCS %#04x failed: %d\n",
				cmds[i].data[0], ret);
			return;
		}
	}
}

static void zumapro_decon_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct zumapro_decon *zd = to_zumapro_decon(crtc->dev);

	if (zd->dsim)
		cm4_set_refresh(zd, drm_mode_vrefresh(&crtc->state->adjusted_mode));
}

/* panel-gs-cm4.c: DRM_MODE_TIMING(hz, 1280, 80, 24, 46, 2856, 12, 4, 28), 66 x 147 mm */
#define CM4_MODE(hz, mode_type) \
	.clock = 1430 * 2900 * (hz) / 1000, \
	.hdisplay = CM4_HDISPLAY, .hsync_start = 1360, .hsync_end = 1384, .htotal = 1430, \
	.vdisplay = CM4_VDISPLAY, .vsync_start = 2868, .vsync_end = 2872, .vtotal = 2900, \
	.width_mm = 66, .height_mm = 147, \
	.type = DRM_MODE_TYPE_DRIVER | (mode_type)

static const struct drm_display_mode cm4_modes[] = {
	{ CM4_MODE(120, DRM_MODE_TYPE_PREFERRED) },
	{ CM4_MODE(60, 0) },
};

static int cm4_get_modes(struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(cm4_modes); i++) {
		mode = drm_mode_duplicate(connector->dev, &cm4_modes[i]);
		if (!mode)
			break;
		drm_mode_set_name(mode);
		drm_mode_probed_add(connector, mode);
	}

	connector->display_info.width_mm = cm4_modes[0].width_mm;
	connector->display_info.height_mm = cm4_modes[0].height_mm;
	return i;
}

static int zumapro_decon_connector_get_modes(struct drm_connector *connector)
{
	if (to_zumapro_decon(connector->dev)->dsim)
		return cm4_get_modes(connector);

	return drm_sysfb_connector_helper_get_modes(connector);
}

static int zumapro_decon_plane_atomic_check(struct drm_plane *plane,
					    struct drm_atomic_commit *state)
{
	struct zumapro_decon *zd = to_zumapro_decon(plane->dev);
	struct drm_plane_state *plane_state = drm_atomic_get_new_plane_state(state, plane);
	struct drm_crtc_state *crtc_state = NULL;
	int ret;

	if (plane_state->crtc)
		crtc_state = drm_atomic_get_new_crtc_state(state, plane_state->crtc);

	ret = drm_atomic_helper_check_plane_state(plane_state, crtc_state,
						  DRM_PLANE_NO_SCALING, DRM_PLANE_NO_SCALING,
						  false, false);
	if (ret || !plane_state->visible)
		return ret;

	if (plane_state->fb->pitches[0] != zd->sysfb.fb_pitch)
		return -EINVAL;

	return 0;
}

static void zumapro_decon_plane_atomic_disable(struct drm_plane *plane,
					       struct drm_atomic_commit *state)
{
	struct zumapro_decon *zd = to_zumapro_decon(plane->dev);

	memset_io(zd->sysfb.fb_addr.vaddr_iomem, 0,
		  zd->sysfb.fb_pitch * zd->sysfb.fb_mode.vdisplay);
	decon_scan_from(zd, zd->abl_base);
}

static void zumapro_decon_plane_atomic_update(struct drm_plane *plane,
					      struct drm_atomic_commit *state)
{
	struct zumapro_decon *zd = to_zumapro_decon(plane->dev);
	struct drm_plane_state *plane_state = drm_atomic_get_new_plane_state(state, plane);

	if (!plane_state->visible) {
		zumapro_decon_plane_atomic_disable(plane, state);
		return;
	}

	decon_scan_from(zd, drm_fb_dma_get_gem_addr(plane_state->fb, plane_state, 0));
}

static const u32 zumapro_decon_plane_formats[] = {
	DRM_FORMAT_XRGB8888,
};

static const struct drm_plane_helper_funcs zumapro_decon_plane_helper_funcs = {
	.atomic_check = zumapro_decon_plane_atomic_check,
	.atomic_update = zumapro_decon_plane_atomic_update,
	.atomic_disable = zumapro_decon_plane_atomic_disable,
};

static const struct drm_plane_funcs zumapro_decon_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static const struct drm_crtc_helper_funcs zumapro_decon_crtc_helper_funcs = {
	DRM_SYSFB_CRTC_HELPER_FUNCS,
	DRM_CRTC_HELPER_VBLANK_FUNCS,
	.mode_set_nofb = zumapro_decon_crtc_mode_set_nofb,
};

static const struct drm_crtc_funcs zumapro_decon_crtc_funcs = {
	DRM_SYSFB_CRTC_FUNCS,
	.destroy = drm_crtc_cleanup,
	.enable_vblank = zumapro_decon_crtc_enable_vblank,
	.disable_vblank = zumapro_decon_crtc_disable_vblank,
};

static const struct drm_encoder_funcs zumapro_decon_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static const struct drm_connector_helper_funcs zumapro_decon_connector_helper_funcs = {
	.get_modes = zumapro_decon_connector_get_modes,
};

static const struct drm_connector_funcs zumapro_decon_connector_funcs = {
	DRM_SYSFB_CONNECTOR_FUNCS,
	.destroy = drm_connector_cleanup,
};

static const struct drm_mode_config_funcs zumapro_decon_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static int zumapro_decon_map_regs(struct zumapro_decon *zd, struct platform_device *pdev)
{
	zd->decon = devm_platform_ioremap_resource_byname(pdev, "decon");
	if (IS_ERR(zd->decon))
		return PTR_ERR(zd->decon);

	zd->rdma = devm_platform_ioremap_resource_byname(pdev, "rdma");
	return PTR_ERR_OR_ZERO(zd->rdma);
}

static int zumapro_decon_map_panel(struct zumapro_decon *zd, struct platform_device *pdev)
{
	struct device_node *panel = of_get_compatible_child(pdev->dev.of_node, "google,cm4");
	const struct drm_display_mode *mode = &zd->sysfb.fb_mode;

	if (!panel)
		return 0;
	of_node_put(panel);

	if (mode->hdisplay != CM4_HDISPLAY || mode->vdisplay != CM4_VDISPLAY)
		return dev_err_probe(&pdev->dev, -EINVAL, "scanout is not a cm4 panel's size\n");

	zd->dsim = devm_platform_ioremap_resource_byname(pdev, "dsim");
	return PTR_ERR_OR_ZERO(zd->dsim);
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
	zd->abl_base = base;
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

static int zumapro_decon_request_irq(struct zumapro_decon *zd, struct platform_device *pdev)
{
	int irq = platform_get_irq_byname(pdev, "frame_start");

	if (irq < 0)
		return irq;

	decon_frame_start_irq_mask(zd);
	return devm_request_irq(&pdev->dev, irq, decon_frame_start_irq, 0, DRIVER_NAME, zd);
}

static int zumapro_decon_init_modeset(struct zumapro_decon *zd)
{
	struct drm_device *dev = &zd->sysfb.dev;
	const struct drm_format_info *format = zd->sysfb.fb_format;
	struct drm_display_mode *mode = &zd->sysfb.fb_mode;
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

	ret = drm_universal_plane_init(dev, &zd->primary_plane, 0, &zumapro_decon_plane_funcs,
				       zumapro_decon_plane_formats,
				       ARRAY_SIZE(zumapro_decon_plane_formats), NULL,
				       DRM_PLANE_TYPE_PRIMARY, NULL);
	if (ret)
		return ret;
	drm_plane_helper_add(&zd->primary_plane, &zumapro_decon_plane_helper_funcs);

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

	ret = drm_vblank_init(dev, 1);
	if (ret)
		return ret;

	drm_mode_config_reset(dev);
	return 0;
}

DEFINE_DRM_GEM_DMA_FOPS(zumapro_decon_fops);

static const struct drm_driver zumapro_decon_driver = {
	DRM_GEM_DMA_DRIVER_OPS,
	DRM_FBDEV_DMA_DRIVER_OPS,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.driver_features = DRIVER_ATOMIC | DRIVER_GEM | DRIVER_MODESET,
	.fops = &zumapro_decon_fops,
};

static int zumapro_decon_probe(struct platform_device *pdev)
{
	const struct drm_format_info *format = drm_format_info(DRM_FORMAT_XRGB8888);
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

	ret = zumapro_decon_map_panel(zd, pdev);
	if (ret)
		return ret;

	ret = zumapro_decon_map_fb(zd, pdev, &fb);
	if (ret)
		return ret;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	rdma_set_xrgb8888(zd);

	ret = zumapro_decon_init_modeset(zd);
	if (ret)
		return ret;

	ret = zumapro_decon_request_irq(zd, pdev);
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
