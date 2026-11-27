// SPDX-License-Identifier: GPL-2.0-only

#include <linux/array_size.h>
#include <linux/clk.h>
#include <linux/container_of.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_opp.h>
#include <linux/units.h>

struct zumapro_gpu_dvfs_level {
	u32 g3dl2_khz;
	u32 g3d_khz;
};

/* gpu_dvfs_table_v2 */
static const struct zumapro_gpu_dvfs_level zumapro_gpu_dvfs_levels[] = {
	{ 160000, 150000 },
	{ 386000, 302000 },
	{ 430000, 337000 },
	{ 531000, 419000 },
	{ 659000, 521000 },
	{ 749000, 649000 },
	{ 900000, 890000 },
	{ 950000, 940000 },
};

struct zumapro_gpu_dvfs {
	struct generic_pm_domain pd;
	struct clk *g3dl2;
	struct clk *g3d;
};

static struct zumapro_gpu_dvfs *to_zumapro_gpu_dvfs(struct generic_pm_domain *pd)
{
	return container_of(pd, struct zumapro_gpu_dvfs, pd);
}

static int zumapro_gpu_dvfs_add_opp(struct device *dev, unsigned int level)
{
	struct dev_pm_opp_data data = {
		.level = level,
		.freq = zumapro_gpu_dvfs_levels[level].g3dl2_khz * HZ_PER_KHZ,
	};

	return dev_pm_opp_add_dynamic(dev, &data);
}

static int zumapro_gpu_dvfs_attach_dev(struct generic_pm_domain *pd,
				       struct device *dev)
{
	unsigned int level;
	int ret;

	for (level = 0; level < ARRAY_SIZE(zumapro_gpu_dvfs_levels); level++) {
		ret = zumapro_gpu_dvfs_add_opp(dev, level);
		if (ret) {
			dev_pm_opp_remove_all_dynamic(dev);
			return ret;
		}
	}

	return 0;
}

static void zumapro_gpu_dvfs_detach_dev(struct generic_pm_domain *pd,
					struct device *dev)
{
	dev_pm_opp_remove_all_dynamic(dev);
}

static int zumapro_gpu_dvfs_set_level(struct zumapro_gpu_dvfs *dvfs,
				      const struct zumapro_gpu_dvfs_level *lvl)
{
	int ret;

	ret = clk_set_rate(dvfs->g3dl2, lvl->g3dl2_khz * HZ_PER_KHZ);
	if (ret)
		return ret;

	return clk_set_rate(dvfs->g3d, lvl->g3d_khz * HZ_PER_KHZ);
}

static int zumapro_gpu_dvfs_set_performance_state(struct generic_pm_domain *pd,
						  unsigned int state)
{
	if (state >= ARRAY_SIZE(zumapro_gpu_dvfs_levels))
		return -EINVAL;

	return zumapro_gpu_dvfs_set_level(to_zumapro_gpu_dvfs(pd),
					  &zumapro_gpu_dvfs_levels[state]);
}

static int zumapro_gpu_dvfs_get_clks(struct device *dev,
				     struct zumapro_gpu_dvfs *dvfs)
{
	dvfs->g3dl2 = devm_clk_get(dev, "g3dl2");
	if (IS_ERR(dvfs->g3dl2))
		return dev_err_probe(dev, PTR_ERR(dvfs->g3dl2),
				     "failed to get g3dl2 clock\n");

	dvfs->g3d = devm_clk_get(dev, "g3d");
	if (IS_ERR(dvfs->g3d))
		return dev_err_probe(dev, PTR_ERR(dvfs->g3d),
				     "failed to get g3d clock\n");

	return 0;
}

static int zumapro_gpu_dvfs_init_pd(struct device *dev,
				    struct zumapro_gpu_dvfs *dvfs)
{
	int ret;

	dvfs->pd.name = dev_name(dev);
	dvfs->pd.attach_dev = zumapro_gpu_dvfs_attach_dev;
	dvfs->pd.detach_dev = zumapro_gpu_dvfs_detach_dev;
	dvfs->pd.set_performance_state = zumapro_gpu_dvfs_set_performance_state;
	dvfs->pd.flags = GENPD_FLAG_OPP_TABLE_FW;

	ret = pm_genpd_init(&dvfs->pd, NULL, false);
	if (ret)
		return dev_err_probe(dev, ret, "failed to init power domain\n");

	ret = of_genpd_add_provider_simple(dev->of_node, &dvfs->pd);
	if (ret) {
		pm_genpd_remove(&dvfs->pd);
		return dev_err_probe(dev, ret, "failed to add power domain provider\n");
	}

	return 0;
}

static int zumapro_gpu_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct zumapro_gpu_dvfs *dvfs;
	int ret;

	dvfs = devm_kzalloc(dev, sizeof(*dvfs), GFP_KERNEL);
	if (!dvfs)
		return -ENOMEM;

	ret = zumapro_gpu_dvfs_get_clks(dev, dvfs);
	if (ret)
		return ret;

	return zumapro_gpu_dvfs_init_pd(dev, dvfs);
}

static const struct of_device_id zumapro_gpu_dvfs_of_match[] = {
	{ .compatible = "google,zumapro-gpu-dvfs" },
	{ }
};

static struct platform_driver zumapro_gpu_dvfs_driver = {
	.driver = {
		.name = "zumapro-gpu-dvfs",
		.of_match_table = zumapro_gpu_dvfs_of_match,
		.suppress_bind_attrs = true,
	},
	.probe = zumapro_gpu_dvfs_probe,
};
builtin_platform_driver(zumapro_gpu_dvfs_driver);
