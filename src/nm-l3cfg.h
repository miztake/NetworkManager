// SPDX-License-Identifier: LGPL-2.1+

#ifndef __NM_L3CFG_H__
#define __NM_L3CFG_H__

#include "platform/nmp-object.h"
#include "nm-l3-config-data.h"

#define NM_TYPE_L3CFG            (nm_l3cfg_get_type ())
#define NM_L3CFG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_L3CFG, NML3Cfg))
#define NM_L3CFG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_L3CFG, NML3CfgClass))
#define NM_IS_L3CFG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_L3CFG))
#define NM_IS_L3CFG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NM_TYPE_L3CFG))
#define NM_L3CFG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_L3CFG, NML3CfgClass))

#define NM_L3CFG_NETNS   "netns"
#define NM_L3CFG_IFINDEX "ifindex"

#define NM_L3CFG_SIGNAL_NOTIFY "l3cfg-notify"

typedef enum {
	NM_L3_CONFIG_NOTIFY_TYPE_ROUTES_TEMPORARY_NOT_AVAILABLE_EXPIRED,
	_NM_L3_CONFIG_NOTIFY_TYPE_NUM,
} NML3ConfigNotifyType;

#define NM_L3_CONFIG_NOTIFY_TYPE_ROUTES_TEMPORARY_NOT_AVAILABLE_EXPIRED_DETAIL "routes-temporary-not-available"

struct _NML3CfgPrivate;

struct _NML3Cfg {
	GObject parent;
	struct {
		struct _NML3CfgPrivate *p;
		NMNetns *netns;
		NMPlatform *platform;
		const NMPObject *pllink;
		int ifindex;
		bool changed_configs:1;
	} priv;
};

typedef struct _NML3CfgClass NML3CfgClass;

GType nm_l3cfg_get_type (void);

NML3Cfg *nm_l3cfg_new (NMNetns *netns, int ifindex);

/*****************************************************************************/

void _nm_l3cfg_notify_platform_change_on_idle (NML3Cfg *self, guint32 obj_type_flags);

void _nm_l3cfg_notify_platform_change (NML3Cfg *self,
                                       NMPlatformSignalChangeType change_type,
                                       const NMPObject *obj);

/*****************************************************************************/

static inline int
nm_l3cfg_get_ifindex (const NML3Cfg *self)
{
	nm_assert (NM_IS_L3CFG (self));

	return self->priv.ifindex;
}

static inline const char *
nm_l3cfg_get_ifname (const NML3Cfg *self)
{
	nm_assert (NM_IS_L3CFG (self));

	return nmp_object_link_get_ifname (self->priv.pllink);
}

static inline NMNetns *
nm_l3cfg_get_netns (const NML3Cfg *self)
{
	nm_assert (NM_IS_L3CFG (self));

	return self->priv.netns;
}

static inline NMPlatform *
nm_l3cfg_get_platform (const NML3Cfg *self)
{
	nm_assert (NM_IS_L3CFG (self));

	return self->priv.platform;
}

/*****************************************************************************/

typedef enum {
	NM_L3CFG_PROPERTY_EMIT_TYPE_ANY,
	NM_L3CFG_PROPERTY_EMIT_TYPE_IP4_ROUTE,
	NM_L3CFG_PROPERTY_EMIT_TYPE_IP6_ROUTE,
} NML3CfgPropertyEmitType;

void nm_l3cfg_property_emit_register (NML3Cfg *self,
                                      GObject *target_obj,
                                      const GParamSpec *target_property,
                                      NML3CfgPropertyEmitType emit_type);

void nm_l3cfg_property_emit_unregister (NML3Cfg *self,
                                        GObject *target_obj,
                                        const GParamSpec *target_property);

/*****************************************************************************/

void nm_l3cfg_mark_config_dirty (NML3Cfg *self,
                                 gconstpointer tag,
                                 gboolean dirty);

void nm_l3cfg_add_config (NML3Cfg *self,
                          gconstpointer tag,
                          gboolean replace_same_tag,
                          const NML3ConfigData *l3cfg,
                          int priority,
                          guint32 default_route_penalty_4,
                          guint32 default_route_penalty_6,
                          NML3ConfigMergeFlags merge_flags);

void nm_l3cfg_remove_config (NML3Cfg *self,
                             gconstpointer tag,
                             const NML3ConfigData *ifcfg);

void nm_l3cfg_remove_config_all (NML3Cfg *self,
                                 gconstpointer tag,
                                 gboolean only_dirty);

/*****************************************************************************/

typedef enum {
	/* ASSUME means to keep any pre-existing extra routes/addresses, while
	 * also not adding routes/addresses that are not present yet. This is to
	 * gracefully take over after restart, where the existing IP configuration
	 * should not change. */
	NM_L3_CFG_COMMIT_TYPE_ASSUME,

	/* UPDATE means to add new addresses/routes, while also removing addresses/routes
	 * that are no longer present (but were previously configured by NetworkManager).
	 * Routes/addresses that were removed externally won't be re-added, and routes/addresses
	 * that are added externally won't be removed. */
	NM_L3_CFG_COMMIT_TYPE_UPDATE,

	/* This is a full sync. It configures the IP addresses/routes that are indicated,
	 * while removing the existing ones from the interface. */
	NM_L3_CFG_COMMIT_TYPE_REAPPLY,
} NML3CfgCommitType;

gboolean nm_l3cfg_platform_commit (NML3Cfg *self,
                                   NML3CfgCommitType commit_type,
                                   int addr_family,
                                   gboolean *out_final_failure_for_temporary_not_available);

#endif /* __NM_L3CFG_H__ */
