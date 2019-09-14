/*
** $Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/gl_init.c#7 $
*/

/*! \file   gl_init.c
    \brief  Main routines of Linux driver

    This file contains the main routines of Linux driver for MediaTek Inc. 802.11
    Wireless LAN Adapters.
*/





/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "gl_cfg80211.h"
#include "precomp.h"
#if defined(MTK_TC1_FEATURE_TEMP)
#include <tc1_partition.h>	/* build */
#endif
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/* #define MAX_IOREQ_NUM   10 */


#define FIX_ALPS00409409406
#undef TEST_FOR_NET_RCU_LOCK


#ifdef FIX_ALPS00409409406
atomic_t fgIsUnderEarlierSuspend = {.counter = 0 };
#else
BOOLEAN fgIsUnderEarlierSuspend = false;
#endif

struct semaphore g_halt_sem;
int g_u4HaltFlag = 0;

#if defined(MTK_PACKET_FILTERING_SUPPORT)
UINT_8 g_packet_switch = 0;
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
/* Tasklet mechanism is like buttom-half in Linux. We just want to
 * send a signal to OS for interrupt defer processing. All resources
 * are NOT allowed reentry, so txPacket, ISR-DPC and ioctl must avoid preempty.
 */
typedef struct _WLANDEV_INFO_T {
	struct net_device *prDev;
} WLANDEV_INFO_T, *P_WLANDEV_INFO_T;

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

MODULE_AUTHOR(NIC_AUTHOR);
MODULE_DESCRIPTION(NIC_DESC);
MODULE_SUPPORTED_DEVICE(NIC_NAME);

#if 0
MODULE_LICENSE("MTK Propietary");
#else
MODULE_LICENSE("GPL");
#endif

#define NIC_INF_NAME    "wlan%d"	/* interface name */

/* support to change debug module info dynamically */
UINT_8 aucDebugModule[DBG_MODULE_NUM];
UINT_32 u4DebugModule = 0;


/* 4 2007/06/26, mikewu, now we don't use this, we just fix the number of wlan device to 1 */
static WLANDEV_INFO_T arWlanDevInfo[CFG_MAX_WLAN_DEVICES] = { {0} };

static UINT_32 u4WlanDevNum;	/* How many NICs coexist now */

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#if CFG_ENABLE_WIFI_DIRECT
static SUB_MODULE_HANDLER rSubModHandler[SUB_MODULE_NUM] = { {NULL} };
#endif

#define CHAN2G(_channel, _freq, _flags)         \
    {                                           \
    .band               = IEEE80211_BAND_2GHZ,  \
    .center_freq        = (_freq),              \
    .hw_value           = (_channel),           \
    .flags              = (_flags),             \
    .max_antenna_gain   = 0,                    \
    .max_power          = 30,                   \
    }
static struct ieee80211_channel mtk_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

#define CHAN5G(_channel, _flags)                    \
    {                                               \
    .band               = IEEE80211_BAND_5GHZ,      \
    .center_freq        = 5000 + (5 * (_channel)),  \
    .hw_value           = (_channel),               \
    .flags              = (_flags),                 \
    .max_antenna_gain   = 0,                        \
    .max_power          = 30,                       \
    }
static struct ieee80211_channel mtk_5ghz_channels[] = {
	CHAN5G(34, 0), CHAN5G(36, 0),
	CHAN5G(38, 0), CHAN5G(40, 0),
	CHAN5G(42, 0), CHAN5G(44, 0),
	CHAN5G(46, 0), CHAN5G(48, 0),
	CHAN5G(52, 0), CHAN5G(56, 0),
	CHAN5G(60, 0), CHAN5G(64, 0),
	CHAN5G(100, 0), CHAN5G(104, 0),
	CHAN5G(108, 0), CHAN5G(112, 0),
	CHAN5G(116, 0), CHAN5G(120, 0),
	CHAN5G(124, 0), CHAN5G(128, 0),
	CHAN5G(132, 0), CHAN5G(136, 0),
	CHAN5G(140, 0), CHAN5G(149, 0),
	CHAN5G(153, 0), CHAN5G(157, 0),
	CHAN5G(161, 0), CHAN5G(165, 0),
	CHAN5G(169, 0), CHAN5G(173, 0),
	CHAN5G(184, 0), CHAN5G(188, 0),
	CHAN5G(192, 0), CHAN5G(196, 0),
	CHAN5G(200, 0), CHAN5G(204, 0),
	CHAN5G(208, 0), CHAN5G(212, 0),
	CHAN5G(216, 0),
};

/* for cfg80211 - rate table */
static struct ieee80211_rate mtk_rates[] = {
	RATETAB_ENT(10, 0x1000, 0),
	RATETAB_ENT(20, 0x1001, 0),
	RATETAB_ENT(55, 0x1002, 0),
	RATETAB_ENT(110, 0x1003, 0),	/* 802.11b */
	RATETAB_ENT(60, 0x2000, 0),
	RATETAB_ENT(90, 0x2001, 0),
	RATETAB_ENT(120, 0x2002, 0),
	RATETAB_ENT(180, 0x2003, 0),
	RATETAB_ENT(240, 0x2004, 0),
	RATETAB_ENT(360, 0x2005, 0),
	RATETAB_ENT(480, 0x2006, 0),
	RATETAB_ENT(540, 0x2007, 0),	/* 802.11a/g */
};

#define mtk_a_rates         (mtk_rates + 4)
#define mtk_a_rates_size    (sizeof(mtk_rates) / sizeof(mtk_rates[0]) - 4)
#define mtk_g_rates         (mtk_rates + 0)
#define mtk_g_rates_size    (sizeof(mtk_rates) / sizeof(mtk_rates[0]) - 0)

#define MT6620_MCS_INFO                                 \
{                                                       \
    .rx_mask        = {0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0},\
    .rx_highest     = 0,                                \
    .tx_params      = IEEE80211_HT_MCS_TX_DEFINED,      \
}

#define MT6620_HT_CAP                                   \
{                                                       \
    .ht_supported   = true,                             \
    .cap            = IEEE80211_HT_CAP_SUP_WIDTH_20_40  \
		    | IEEE80211_HT_CAP_SM_PS            \
		    | IEEE80211_HT_CAP_GRN_FLD          \
		    | IEEE80211_HT_CAP_SGI_20           \
		    | IEEE80211_HT_CAP_SGI_40,          \
    .ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,       \
    .ampdu_density  = IEEE80211_HT_MPDU_DENSITY_NONE,   \
    .mcs            = MT6620_MCS_INFO,                  \
}

/* public for both Legacy Wi-Fi / P2P access */
struct ieee80211_supported_band mtk_band_2ghz = {
	.band = IEEE80211_BAND_2GHZ,
	.channels = mtk_2ghz_channels,
	.n_channels = ARRAY_SIZE(mtk_2ghz_channels),
	.bitrates = mtk_g_rates,
	.n_bitrates = mtk_g_rates_size,
	.ht_cap = MT6620_HT_CAP,
};

/* public for both Legacy Wi-Fi / P2P access */
struct ieee80211_supported_band mtk_band_5ghz = {
	.band = IEEE80211_BAND_5GHZ,
	.channels = mtk_5ghz_channels,
	.n_channels = ARRAY_SIZE(mtk_5ghz_channels),
	.bitrates = mtk_a_rates,
	.n_bitrates = mtk_a_rates_size,
	.ht_cap = MT6620_HT_CAP,
};

static const UINT_32 mtk_cipher_suites[] = {
	/* keep WEP first, it may be removed below */
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,

	/* keep last -- depends on hw flags! */
	WLAN_CIPHER_SUITE_AES_CMAC
};

static struct cfg80211_ops mtk_wlan_ops = {
	.change_virtual_intf = mtk_cfg80211_change_iface,
	.add_key = mtk_cfg80211_add_key,
	.get_key = mtk_cfg80211_get_key,
	.del_key = mtk_cfg80211_del_key,
	.set_default_key = mtk_cfg80211_set_default_key,
	.get_station = mtk_cfg80211_get_station,
	.scan = mtk_cfg80211_scan,
	.connect = mtk_cfg80211_connect,
	.disconnect = mtk_cfg80211_disconnect,
	.join_ibss = mtk_cfg80211_join_ibss,
	.leave_ibss = mtk_cfg80211_leave_ibss,
	.set_power_mgmt = mtk_cfg80211_set_power_mgmt,
	.set_pmksa = mtk_cfg80211_set_pmksa,
	.del_pmksa = mtk_cfg80211_del_pmksa,
	.flush_pmksa = mtk_cfg80211_flush_pmksa,

	/* Action Frame TX/RX */
	.remain_on_channel = mtk_cfg80211_remain_on_channel,
	.cancel_remain_on_channel = mtk_cfg80211_cancel_remain_on_channel,
	.mgmt_tx = mtk_cfg80211_mgmt_tx,
	.mgmt_tx_cancel_wait = mtk_cfg80211_mgmt_tx_cancel_wait,
#ifdef CONFIG_NL80211_TESTMODE
	.testmode_cmd = mtk_cfg80211_testmode_cmd,
#endif
};

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

#if defined(CONFIG_HAS_EARLYSUSPEND)
extern int glRegisterEarlySuspend(struct early_suspend *prDesc,
				  early_suspend_callback wlanSuspend,
				  late_resume_callback wlanResume);

extern int glUnregisterEarlySuspend(struct early_suspend *prDesc);
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
/*----------------------------------------------------------------------------*/
/*!
* \brief Override the implementation of select queue
*
* \param[in] dev Pointer to struct net_device
* \param[in] skb Pointer to struct skb_buff
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
unsigned int _cfg80211_classify8021d(struct sk_buff *skb)
{
	unsigned int dscp = 0;

	/* skb->priority values from 256->263 are magic values
	 * directly indicate a specific 802.1d priority.  This is
	 * to allow 802.1d priority to be passed directly in from
	 * tags
	 */

	if (skb->priority >= 256 && skb->priority <= 263) {
		return skb->priority - 256;
	}
	switch (skb->protocol) {
	case htons(ETH_P_IP):
		dscp = ip_hdr(skb)->tos & 0xfc;
		break;
	}
	return dscp >> 5;
}


static const UINT_16 au16Wlan1dToQueueIdx[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };

static UINT_16 wlanSelectQueue(struct net_device *dev, struct sk_buff *skb)
{
	skb->priority = _cfg80211_classify8021d(skb);

	return au16Wlan1dToQueueIdx[skb->priority];
}
#endif


/*----------------------------------------------------------------------------*/
/*!
* \brief Load NVRAM data and translate it into REG_INFO_T
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
* \param[out] prRegInfo  Pointer to struct REG_INFO_T
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static void glLoadNvram(IN P_GLUE_INFO_T prGlueInfo, OUT P_REG_INFO_T prRegInfo)
{
	UINT_32 i, j;
	UINT_8 aucTmp[2];
	PUINT_8 pucDest;

	ASSERT(prGlueInfo);
	ASSERT(prRegInfo);

	if ((!prGlueInfo) || (!prRegInfo)) {
		return;
	}

	if (kalCfgDataRead16(prGlueInfo,
			     sizeof(WIFI_CFG_PARAM_STRUCT) - sizeof(UINT_16),
			     (PUINT_16) aucTmp) == TRUE) {
		prGlueInfo->fgNvramAvailable = TRUE;

		/* load MAC Address */
#if !defined(MTK_TC1_FEATURE_TEMP)
		for (i = 0; i < sizeof(PARAM_MAC_ADDR_LEN); i += sizeof(UINT_16)) {
			kalCfgDataRead16(prGlueInfo,
					 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, aucMacAddress) + i,
					 (PUINT_16) (((PUINT_8) prRegInfo->aucMacAddr) + i));
		}
#else
		TC1_FAC_NAME(FacReadWifiMacAddr) ((unsigned char *)prRegInfo->aucMacAddr);
#endif

		/* load country code */
		kalCfgDataRead16(prGlueInfo,
				 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, aucCountryCode[0]),
				 (PUINT_16) aucTmp);

		/* cast to wide characters */
		prRegInfo->au2CountryCode[0] = (UINT_16) aucTmp[0];
		prRegInfo->au2CountryCode[1] = (UINT_16) aucTmp[1];

		/* load default normal TX power */
		for (i = 0; i < sizeof(TX_PWR_PARAM_T); i += sizeof(UINT_16)) {
			kalCfgDataRead16(prGlueInfo,
					 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, rTxPwr) + i,
					 (PUINT_16) (((PUINT_8) &(prRegInfo->rTxPwr)) + i));
		}

		/* load feature flags */
		kalCfgDataRead16(prGlueInfo,
				 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, ucTxPwrValid), (PUINT_16) aucTmp);
		prRegInfo->ucTxPwrValid = aucTmp[0];
		prRegInfo->ucSupport5GBand = aucTmp[1];

		kalCfgDataRead16(prGlueInfo,
				 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, uc2G4BwFixed20M),
				 (PUINT_16) aucTmp);
		prRegInfo->uc2G4BwFixed20M = aucTmp[0];
		prRegInfo->uc5GBwFixed20M = aucTmp[1];

		kalCfgDataRead16(prGlueInfo,
				 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, ucEnable5GBand),
				 (PUINT_16) aucTmp);
		prRegInfo->ucEnable5GBand = aucTmp[0];

		/* load EFUSE overriding part */
		for (i = 0; i < sizeof(prRegInfo->aucEFUSE); i += sizeof(UINT_16)) {
			kalCfgDataRead16(prGlueInfo,
					 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, aucEFUSE) + i,
					 (PUINT_16) (((PUINT_8) &(prRegInfo->aucEFUSE)) + i));
		}

		/* load band edge tx power control */
		kalCfgDataRead16(prGlueInfo,
				 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, fg2G4BandEdgePwrUsed),
				 (PUINT_16) aucTmp);
		prRegInfo->fg2G4BandEdgePwrUsed = (BOOLEAN) aucTmp[0];
		if (aucTmp[0]) {
			prRegInfo->cBandEdgeMaxPwrCCK = (INT_8) aucTmp[1];

			kalCfgDataRead16(prGlueInfo,
					 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, cBandEdgeMaxPwrOFDM20),
					 (PUINT_16) aucTmp);
			prRegInfo->cBandEdgeMaxPwrOFDM20 = (INT_8) aucTmp[0];
			prRegInfo->cBandEdgeMaxPwrOFDM40 = (INT_8) aucTmp[1];
		}

		/* load regulation subbands */
		kalCfgDataRead16(prGlueInfo,
				 OFFSET_OF(WIFI_CFG_PARAM_STRUCT, ucRegChannelListMap),
				 (PUINT_16) aucTmp);
		prRegInfo->eRegChannelListMap = (ENUM_REG_CH_MAP_T) aucTmp[0];
		prRegInfo->ucRegChannelListIndex = aucTmp[1];

		if (prRegInfo->eRegChannelListMap == REG_CH_MAP_CUSTOMIZED) {
			for (i = 0; i < MAX_SUBBAND_NUM; i++) {
				pucDest = (PUINT_8) &prRegInfo->rDomainInfo.rSubBand[i];
				for (j = 0; j < 6; j += sizeof(UINT_16)) {
					kalCfgDataRead16(prGlueInfo,
							 OFFSET_OF(WIFI_CFG_PARAM_STRUCT,
								   aucRegSubbandInfo)
							 + (i * 6 + j), (PUINT_16) aucTmp);

					*pucDest++ = aucTmp[0];
					*pucDest++ = aucTmp[1];
				}
			}
		}
	} else {
		prGlueInfo->fgNvramAvailable = FALSE;
	}

	return;
}


#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
* \brief called by txthread, run sub module init function
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID wlanSubModRunInit(P_GLUE_INFO_T prGlueInfo)
{
	/*now, we only have p2p module */
	if (rSubModHandler[P2P_MODULE].fgIsInited == FALSE) {
		rSubModHandler[P2P_MODULE].subModInit(prGlueInfo);
		rSubModHandler[P2P_MODULE].fgIsInited = TRUE;
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief called by txthread, run sub module exit function
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID wlanSubModRunExit(P_GLUE_INFO_T prGlueInfo)
{
	/*now, we only have p2p module */
	if (rSubModHandler[P2P_MODULE].fgIsInited == TRUE) {
		rSubModHandler[P2P_MODULE].subModExit(prGlueInfo);
		rSubModHandler[P2P_MODULE].fgIsInited = FALSE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief set sub module init flag, force TxThread to run sub modle init
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
BOOLEAN wlanSubModInit(P_GLUE_INFO_T prGlueInfo)
{
	/* 4  Mark HALT, notify main thread to finish current job */
	prGlueInfo->u4Flag |= GLUE_FLAG_SUB_MOD_INIT;
	/* wake up main thread */
	wake_up_interruptible(&prGlueInfo->waitq);
	/* wait main thread  finish sub module INIT */
	wait_for_completion_interruptible(&prGlueInfo->rSubModComp);

#if 0
	if (prGlueInfo->prAdapter->fgIsP2PRegistered) {
		p2pNetRegister(prGlueInfo);
	}
#endif

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief set sub module exit flag, force TxThread to run sub modle exit
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
BOOLEAN wlanSubModExit(P_GLUE_INFO_T prGlueInfo)
{
#if 0
	if (prGlueInfo->prAdapter->fgIsP2PRegistered) {
		p2pNetUnregister(prGlueInfo);
	}
#endif

	/* 4  Mark HALT, notify main thread to finish current job */
	prGlueInfo->u4Flag |= GLUE_FLAG_SUB_MOD_EXIT;
	/* wake up main thread */
	wake_up_interruptible(&prGlueInfo->waitq);
	/* wait main thread finish sub module EXIT */
	wait_for_completion_interruptible(&prGlueInfo->rSubModComp);

	return TRUE;
}


/*----------------------------------------------------------------------------*/
/*!
* \brief set by sub module, indicate sub module is already inserted
*
* \param[in]  rSubModInit, function pointer point to sub module init function
* \param[in]  rSubModExit,  function pointer point to sub module exit function
* \param[in]  eSubModIdx,  sub module index
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID
wlanSubModRegisterInitExit(SUB_MODULE_INIT rSubModInit,
			   SUB_MODULE_EXIT rSubModExit, ENUM_SUB_MODULE_IDX_T eSubModIdx)
{
	rSubModHandler[eSubModIdx].subModInit = rSubModInit;
	rSubModHandler[eSubModIdx].subModExit = rSubModExit;
	rSubModHandler[eSubModIdx].fgIsInited = FALSE;
}

#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief check wlan is launched or not
*
* \param[in]  (none)
*
* \return TRUE, wlan is already started
*             FALSE, wlan is not started yet
*/
/*----------------------------------------------------------------------------*/
BOOLEAN wlanIsLaunched(VOID)
{
	struct net_device *prDev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;

	/* 4 <0> Sanity check */
	ASSERT(u4WlanDevNum <= CFG_MAX_WLAN_DEVICES);
	if (0 == u4WlanDevNum) {
		return FALSE;
	}

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;

	ASSERT(prDev);
	if (NULL == prDev) {
		return FALSE;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);
	if (NULL == prGlueInfo) {
		return FALSE;
	}

	return prGlueInfo->prAdapter->fgIsWlanLaunched;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief Export wlan GLUE_INFO_T pointer to p2p module
*
* \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
*
* \return TRUE: get GlueInfo pointer successfully
*            FALSE: wlan is not started yet
*/
/*---------------------------------------------------------------------------*/
BOOLEAN wlanExportGlueInfo(P_GLUE_INFO_T *prGlueInfoExpAddr)
{
	struct net_device *prDev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;

	if (0 == u4WlanDevNum) {
		return FALSE;
	}

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (NULL == prDev) {
		return FALSE;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	if (NULL == prGlueInfo) {
		return FALSE;
	}

	if (FALSE == prGlueInfo->prAdapter->fgIsWlanLaunched) {
		return FALSE;
	}

	*prGlueInfoExpAddr = prGlueInfo;
	return TRUE;
}

#endif


/*----------------------------------------------------------------------------*/
/*!
* \brief Release prDev from wlandev_array and free tasklet object related to it.
*
* \param[in] prDev  Pointer to struct net_device
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static void wlanClearDevIdx(struct net_device *prDev)
{
	int i;

	ASSERT(prDev);

	for (i = 0; i < CFG_MAX_WLAN_DEVICES; i++) {
		if (arWlanDevInfo[i].prDev == prDev) {
			arWlanDevInfo[i].prDev = NULL;
			u4WlanDevNum--;
		}
	}

	return;
}				/* end of wlanClearDevIdx() */


/*----------------------------------------------------------------------------*/
/*!
* \brief Allocate an unique interface index, net_device::ifindex member for this
*        wlan device. Store the net_device in wlandev_array, and initialize
*        tasklet object related to it.
*
* \param[in] prDev  Pointer to struct net_device
*
* \retval >= 0      The device number.
* \retval -1        Fail to get index.
*/
/*----------------------------------------------------------------------------*/
static int wlanGetDevIdx(struct net_device *prDev)
{
	int i;

	ASSERT(prDev);

	for (i = 0; i < CFG_MAX_WLAN_DEVICES; i++) {
		if (arWlanDevInfo[i].prDev == (struct net_device *)NULL) {
			/* Reserve 2 bytes space to store one digit of
			 * device number and NULL terminator.
			 */
			arWlanDevInfo[i].prDev = prDev;
			u4WlanDevNum++;
			return i;
		}
	}

	return -1;
}				/* end of wlanGetDevIdx() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A method of struct net_device, a primary SOCKET interface to configure
*        the interface lively. Handle an ioctl call on one of our devices.
*        Everything Linux ioctl specific is done here. Then we pass the contents
*        of the ifr->data to the request message handler.
*
* \param[in] prDev      Linux kernel netdevice
*
* \param[in] prIFReq    Our private ioctl request structure, typed for the generic
*                       struct ifreq so we can use ptr to function
*
* \param[in] cmd        Command ID
*
* \retval WLAN_STATUS_SUCCESS The IOCTL command is executed successfully.
* \retval OTHER The execution of IOCTL command is failed.
*/
/*----------------------------------------------------------------------------*/
int wlanDoIOCTL(struct net_device *prDev, struct ifreq *prIFReq, int i4Cmd)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	int ret = 0;

	/* Verify input parameters for the following functions */
	ASSERT(prDev && prIFReq);
	if (!prDev || !prIFReq) {
		DBGLOG(INIT, WARN, ("%s Invalid input data\n", __func__));
		return -EINVAL;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, WARN, ("%s No glue info\n", __func__));
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		return -EINVAL;
	}
	/* printk ("ioctl %x\n", i4Cmd); */

	if (i4Cmd == SIOCGIWPRIV) {
		/* 0x8B0D, get private ioctl table */
		ret = wext_get_priv(prDev, prIFReq);
	} else if ((i4Cmd >= SIOCIWFIRST) && (i4Cmd < SIOCIWFIRSTPRIV)) {
		/* 0x8B00 ~ 0x8BDF, wireless extension region */
		ret = wext_support_ioctl(prDev, prIFReq, i4Cmd);
	} else if ((i4Cmd >= SIOCIWFIRSTPRIV) && (i4Cmd < SIOCIWLASTPRIV)) {
		/* 0x8BE0 ~ 0x8BFF, private ioctl region */
		ret = priv_support_ioctl(prDev, prIFReq, i4Cmd);
	} else {
		DBGLOG(INIT, WARN, ("Unexpected ioctl command: 0x%04x\n", i4Cmd));
		/* return 0 for safe? */
	}

	return ret;
}				/* end of wlanDoIOCTL() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is to set multicast list and set rx mode.
*
* \param[in] prDev  Pointer to struct net_device
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/

static struct delayed_work workq;
static struct net_device *gPrDev;

static void wlanSetMulticastList(struct net_device *prDev)
{
	gPrDev = prDev;
	schedule_delayed_work(&workq, 0);
}

/* FIXME: Since we cannot sleep in the wlanSetMulticastList, we arrange
 * another workqueue for sleeping. We don't want to block
 * tx_thread, so we can't let tx_thread to do this */

static void wlanSetMulticastListWorkQueue(struct work_struct *work)
{

	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_32 u4PacketFilter = 0;
	UINT_32 u4SetInfoLen;
	struct net_device *prDev = gPrDev;

	down(&g_halt_sem);
	if (g_u4HaltFlag) {
		up(&g_halt_sem);
		return;
	}

	prGlueInfo = (NULL != prDev) ? *((P_GLUE_INFO_T *) netdev_priv(prDev)) : NULL;
	ASSERT(prDev);
	ASSERT(prGlueInfo);
	if (!prDev || !prGlueInfo) {
		DBGLOG(INIT, WARN, ("abnormal dev or skb: prDev(0x%p), prGlueInfo(0x%p)\n",
				    prDev, prGlueInfo));
		up(&g_halt_sem);
		return;
	}

	if (prDev->flags & IFF_PROMISC) {
		u4PacketFilter |= PARAM_PACKET_FILTER_PROMISCUOUS;
	}

	if (prDev->flags & IFF_BROADCAST) {
		u4PacketFilter |= PARAM_PACKET_FILTER_BROADCAST;
		printk("u4PacketFilter |= PARAM_PACKET_FILTER_BROADCAST\n");
	}

	if (prDev->flags & IFF_MULTICAST) {
		if ((prDev->flags & IFF_ALLMULTI) ||
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
		    (netdev_mc_count(prDev) > MAX_NUM_GROUP_ADDR)) {
#else
		    (prDev->mc_count > MAX_NUM_GROUP_ADDR)) {
#endif

			u4PacketFilter |= PARAM_PACKET_FILTER_ALL_MULTICAST;
			printk("u4PacketFilter |= PARAM_PACKET_FILTER_ALL_MULTICAST\n");
		}
		else {
			u4PacketFilter |= PARAM_PACKET_FILTER_MULTICAST;
			printk("u4PacketFilter |= PARAM_PACKET_FILTER_MULTICAST\n");
		}
	}

	up(&g_halt_sem);

	if (kalIoctl(prGlueInfo,
		     wlanoidSetCurrentPacketFilter,
		     &u4PacketFilter,
		     sizeof(u4PacketFilter),
		     FALSE, FALSE, TRUE, FALSE, &u4SetInfoLen) != WLAN_STATUS_SUCCESS) {
		return;
	}


	if (u4PacketFilter & PARAM_PACKET_FILTER_MULTICAST) {
		/* Prepare multicast address list */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
		struct netdev_hw_addr *ha;
#else
		struct dev_mc_list *prMcList;
#endif
		PUINT_8 prMCAddrList = NULL;
		UINT_32 i = 0;

		down(&g_halt_sem);
		if (g_u4HaltFlag) {
			up(&g_halt_sem);
			return;
		}

		prMCAddrList = kalMemAlloc(MAX_NUM_GROUP_ADDR * ETH_ALEN, VIR_MEM_TYPE);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
		netdev_for_each_mc_addr(ha, prDev) {
			if (i < MAX_NUM_GROUP_ADDR) {
				memcpy((prMCAddrList + i * ETH_ALEN), ha->addr, ETH_ALEN);
				i++;
			}
		}
#else
		for (i = 0, prMcList = prDev->mc_list;
		     (prMcList) && (i < prDev->mc_count) && (i < MAX_NUM_GROUP_ADDR);
		     i++, prMcList = prMcList->next) {
			memcpy((prMCAddrList + i * ETH_ALEN), prMcList->dmi_addr, ETH_ALEN);
		}
#endif

		up(&g_halt_sem);

		kalIoctl(prGlueInfo,
			 wlanoidSetMulticastList,
			 prMCAddrList, (i * ETH_ALEN), FALSE, FALSE, TRUE, FALSE, &u4SetInfoLen);

		kalMemFree(prMCAddrList, VIR_MEM_TYPE, MAX_NUM_GROUP_ADDR * ETH_ALEN);
	}

	return;
}				/* end of wlanSetMulticastList() */


/* FIXME: Since we cannot sleep in the wlanSetMulticastList, we arrange
 * another workqueue for sleeping. We don't want to block
 * tx_thread, so we can't let tx_thread to do this */

void p2pSetMulticastListWorkQueueWrapper(P_GLUE_INFO_T prGlueInfo)
{

	ASSERT(prGlueInfo);

	if (!prGlueInfo) {
		DBGLOG(INIT, WARN, ("abnormal dev or skb: prGlueInfo(0x%p)\n", prGlueInfo));
		return;
	}
#if CFG_ENABLE_WIFI_DIRECT
	if (prGlueInfo->prAdapter->fgIsP2PRegistered) {
		mtk_p2p_wext_set_Multicastlist(prGlueInfo);
	}
#endif

	return;
}				/* end of p2pSetMulticastListWorkQueueWrapper() */



/*----------------------------------------------------------------------------*/
/*!
* \brief This function is TX entry point of NET DEVICE.
*
* \param[in] prSkb  Pointer of the sk_buff to be sent
* \param[in] prDev  Pointer to struct net_device
*
* \retval NETDEV_TX_OK - on success.
* \retval NETDEV_TX_BUSY - on failure, packet will be discarded by upper layer.
*/
/*----------------------------------------------------------------------------*/
int wlanHardStartXmit(struct sk_buff *prSkb, struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));

	P_QUE_ENTRY_T prQueueEntry = NULL;
	P_QUE_T prTxQueue = NULL;
	UINT_16 u2QueueIdx = 0;

#if CFG_BOW_TEST
	UINT_32 i;
#endif

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prSkb);
	ASSERT(prDev);
	ASSERT(prGlueInfo);

	if (prGlueInfo->u4Flag & GLUE_FLAG_HALT) {
		DBGLOG(INIT, INFO, ("GLUE_FLAG_HALT skip tx\n"));
		dev_kfree_skb(prSkb);
		return NETDEV_TX_OK;
	}

	prQueueEntry = (P_QUE_ENTRY_T) GLUE_GET_PKT_QUEUE_ENTRY(prSkb);
	prTxQueue = &prGlueInfo->rTxQueue;

#if CFG_BOW_TEST
	DBGLOG(BOW, TRACE, ("sk_buff->len: %d\n", prSkb->len));
	DBGLOG(BOW, TRACE, ("sk_buff->data_len: %d\n", prSkb->data_len));
	DBGLOG(BOW, TRACE, ("sk_buff->data:\n"));

	for (i = 0; i < prSkb->len; i++) {
		DBGLOG(BOW, TRACE, ("%4x", prSkb->data[i]));

		if ((i + 1) % 16 == 0) {
			DBGLOG(BOW, TRACE, ("\n"));
		}
	}

	DBGLOG(BOW, TRACE, ("\n"));
#endif

	if (wlanProcessSecurityFrame(prGlueInfo->prAdapter, (P_NATIVE_PACKET) prSkb) == FALSE) {

#if CFG_DBG_GPIO_PINS
		{
			/* TX request from OS */
			mtk_wcn_stp_debug_gpio_assert(IDX_TX_REQ, DBG_TIE_LOW);
			kalUdelay(1);
			mtk_wcn_stp_debug_gpio_assert(IDX_TX_REQ, DBG_TIE_HIGH);
		}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 26)
		u2QueueIdx = skb_get_queue_mapping(prSkb);
		ASSERT(u2QueueIdx < CFG_MAX_TXQ_NUM);
#endif

		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);
		QUEUE_INSERT_TAIL(prTxQueue, prQueueEntry);
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_TX_QUE);

		GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingFrameNum);
		GLUE_INC_REF_CNT(prGlueInfo->
				 ai4TxPendingFrameNumPerQueue[NETWORK_TYPE_AIS_INDEX][u2QueueIdx]);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 26)
		if (prGlueInfo->ai4TxPendingFrameNumPerQueue[NETWORK_TYPE_AIS_INDEX][u2QueueIdx] >=
		    CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD) {
			netif_stop_subqueue(prDev, u2QueueIdx);
		}
#else
		if (prGlueInfo->i4TxPendingFrameNum >= CFG_TX_STOP_NETIF_QUEUE_THRESHOLD) {
			netif_stop_queue(prDev);
		}
#endif
	} else {
		/* printk("is security frame\n"); */

		GLUE_INC_REF_CNT(prGlueInfo->i4TxPendingSecurityFrameNum);
	}

	DBGLOG(TX, EVENT,
	       ("\n+++++ pending frame %ld len = %d +++++\n", prGlueInfo->i4TxPendingFrameNum,
		prSkb->len));
	prGlueInfo->rNetDevStats.tx_bytes += prSkb->len;
	prGlueInfo->rNetDevStats.tx_packets++;

	/* pr->u4Flag |= GLUE_FLAG_TXREQ; */
	/* wake_up_interruptible(&prGlueInfo->waitq); */
	kalSetEvent(prGlueInfo);


	/* For Linux, we'll always return OK FLAG, because we'll free this skb by ourself */
	return NETDEV_TX_OK;
}				/* end of wlanHardStartXmit() */


/*----------------------------------------------------------------------------*/
/*!
* \brief A method of struct net_device, to get the network interface statistical
*        information.
*
* Whenever an application needs to get statistics for the interface, this method
* is called. This happens, for example, when ifconfig or netstat -i is run.
*
* \param[in] prDev      Pointer to struct net_device.
*
* \return net_device_stats buffer pointer.
*/
/*----------------------------------------------------------------------------*/
struct net_device_stats *wlanGetStats(IN struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));

#if 0
	WLAN_STATUS rStatus;
	UINT_32 u4XmitError = 0;
	UINT_32 u4XmitOk = 0;
	UINT_32 u4RecvError = 0;
	UINT_32 u4RecvOk = 0;
	UINT_32 u4BufLen;

	ASSERT(prDev);

	/* @FIX ME: need a more clear way to do this */


	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryXmitError,
			   &u4XmitError, sizeof(UINT_32), TRUE, TRUE, TRUE, &u4BufLen);

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryXmitOk,
			   &u4XmitOk, sizeof(UINT_32), TRUE, TRUE, TRUE, &u4BufLen);
	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryRcvOk,
			   &u4RecvOk, sizeof(UINT_32), TRUE, TRUE, TRUE, &u4BufLen);
	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryRcvError,
			   &u4RecvError, sizeof(UINT_32), TRUE, TRUE, TRUE, &u4BufLen);
	prGlueInfo->rNetDevStats.rx_packets = u4RecvOk;
	prGlueInfo->rNetDevStats.tx_packets = u4XmitOk;
	prGlueInfo->rNetDevStats.tx_errors = u4XmitError;
	prGlueInfo->rNetDevStats.rx_errors = u4RecvError;
	/* prGlueInfo->rNetDevStats.rx_bytes   = rCustomNetDevStats.u4RxBytes; */
	/* prGlueInfo->rNetDevStats.tx_bytes   = rCustomNetDevStats.u4TxBytes; */
	/* prGlueInfo->rNetDevStats.rx_errors  = rCustomNetDevStats.u4RxErrors; */
	/* prGlueInfo->rNetDevStats.multicast  = rCustomNetDevStats.u4Multicast; */
#endif
	/* prGlueInfo->rNetDevStats.rx_packets = 0; */
	/* prGlueInfo->rNetDevStats.tx_packets = 0; */
	prGlueInfo->rNetDevStats.tx_errors = 0;
	prGlueInfo->rNetDevStats.rx_errors = 0;
	/* prGlueInfo->rNetDevStats.rx_bytes   = 0; */
	/* prGlueInfo->rNetDevStats.tx_bytes   = 0; */
	prGlueInfo->rNetDevStats.rx_errors = 0;
	prGlueInfo->rNetDevStats.multicast = 0;

	return &prGlueInfo->rNetDevStats;

}				/* end of wlanGetStats() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->init
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0         The execution of wlanInit succeeds.
* \retval -ENXIO    No such device.
*/
/*----------------------------------------------------------------------------*/
static int wlanInit(struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	if (!prDev) {
		return -ENXIO;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 12)
	INIT_DELAYED_WORK(&workq, wlanSetMulticastListWorkQueue);
#else
	INIT_DELAYED_WORK(&workq, wlanSetMulticastListWorkQueue, NULL);
#endif

	return 0;		/* success */
}				/* end of wlanInit() */


/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->uninit
*
* \param[in] prDev      Pointer to struct net_device.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static void wlanUninit(struct net_device *prDev)
{
	return;
}				/* end of wlanUninit() */


/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->open
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0     The execution of wlanOpen succeeds.
* \retval < 0   The execution of wlanOpen failed.
*/
/*----------------------------------------------------------------------------*/
static int wlanOpen(struct net_device *prDev)
{
	ASSERT(prDev);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 26)
	netif_tx_start_all_queues(prDev);
#else
	netif_start_queue(prDev);
#endif

	return 0;		/* success */
}				/* end of wlanOpen() */


/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->stop
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0     The execution of wlanStop succeeds.
* \retval < 0   The execution of wlanStop failed.
*/
/*----------------------------------------------------------------------------*/
static int wlanStop(struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	struct cfg80211_scan_request *prScanRequest = NULL;
	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prDev);

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));

	/* CFG80211 down */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prGlueInfo->prScanRequest != NULL) {
		prScanRequest = prGlueInfo->prScanRequest;
		prGlueInfo->prScanRequest = NULL;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (prScanRequest) {
		cfg80211_scan_done(prScanRequest, TRUE);
	}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 26)
	netif_tx_stop_all_queues(prDev);
#else
	netif_stop_queue(prDev);
#endif

	return 0;		/* success */
}				/* end of wlanStop() */


/*----------------------------------------------------------------------------*/
/*!
 * \brief Update Channel table for cfg80211 for Wi-Fi Direct based on current country code
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   none
 */
/*----------------------------------------------------------------------------*/
VOID wlanUpdateChannelTable(P_GLUE_INFO_T prGlueInfo)
{
	UINT_8 i, j;
	UINT_8 ucNumOfChannel;
	RF_CHANNEL_INFO_T aucChannelList[ARRAY_SIZE(mtk_2ghz_channels) +
					 ARRAY_SIZE(mtk_5ghz_channels)];

	/* 1. Disable all channel */
	for (i = 0; i < ARRAY_SIZE(mtk_2ghz_channels); i++) {
		mtk_2ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
	}

	for (i = 0; i < ARRAY_SIZE(mtk_5ghz_channels); i++) {
		mtk_5ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
	}

	/* 2. Get current domain channel list */
	rlmDomainGetChnlList(prGlueInfo->prAdapter,
			     BAND_NULL,
			     ARRAY_SIZE(mtk_2ghz_channels) + ARRAY_SIZE(mtk_5ghz_channels),
			     &ucNumOfChannel, aucChannelList);

	/* 3. Enable specific channel based on domain channel list */
	for (i = 0; i < ucNumOfChannel; i++) {
		switch (aucChannelList[i].eBand) {
		case BAND_2G4:
			for (j = 0; j < ARRAY_SIZE(mtk_2ghz_channels); j++) {
				if (mtk_2ghz_channels[j].hw_value == aucChannelList[i].ucChannelNum) {
					mtk_2ghz_channels[j].flags &= ~IEEE80211_CHAN_DISABLED;
					break;
				}
			}
			break;

		case BAND_5G:
			for (j = 0; j < ARRAY_SIZE(mtk_5ghz_channels); j++) {
				if (mtk_5ghz_channels[j].hw_value == aucChannelList[i].ucChannelNum) {
					mtk_5ghz_channels[j].flags &= ~IEEE80211_CHAN_DISABLED;
					break;
				}
			}
			break;

		default:
			break;
		}
	}

	return;
}


/*----------------------------------------------------------------------------*/
/*!
* \brief Register the device to the kernel and return the index.
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0     The execution of wlanNetRegister succeeds.
* \retval < 0   The execution of wlanNetRegister failed.
*/
/*----------------------------------------------------------------------------*/
static INT_32 wlanNetRegister(struct wireless_dev *prWdev)
{
	P_GLUE_INFO_T prGlueInfo;
	INT_32 i4DevIdx = -1;

	ASSERT(prWdev);


	do {
		if (!prWdev) {
			break;
		}

		prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);

		if ((i4DevIdx = wlanGetDevIdx(prWdev->netdev)) < 0) {
			DBGLOG(INIT, ERROR, ("wlanNetRegister: net_device number exceeds.\n"));
			break;
		}

		/* adjust channel support status */
		wlanUpdateChannelTable((P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy));

		if (wiphy_register(prWdev->wiphy) < 0) {
			DBGLOG(INIT, ERROR,
			       ("wlanNetRegister: wiphy context is not registered.\n"));
			wlanClearDevIdx(prWdev->netdev);
			i4DevIdx = -1;
		}

		if (register_netdev(prWdev->netdev) < 0) {
			DBGLOG(INIT, ERROR,
			       ("wlanNetRegister: net_device context is not registered.\n"));

			wiphy_unregister(prWdev->wiphy);
			wlanClearDevIdx(prWdev->netdev);
			i4DevIdx = -1;
		}

		if (i4DevIdx != -1) {
			prGlueInfo->fgIsRegistered = TRUE;
		}
	}
	while (FALSE);

	return i4DevIdx;	/* success */
}				/* end of wlanNetRegister() */


/*----------------------------------------------------------------------------*/
/*!
* \brief Unregister the device from the kernel
*
* \param[in] prWdev      Pointer to struct net_device.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID wlanNetUnregister(struct wireless_dev *prWdev)
{
	P_GLUE_INFO_T prGlueInfo;

	if (!prWdev) {
		DBGLOG(INIT, ERROR, ("wlanNetUnregister: The device context is NULL\n"));
		return;
	}

	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);

	wlanClearDevIdx(prWdev->netdev);
	unregister_netdev(prWdev->netdev);
	wiphy_unregister(prWdev->wiphy);

	prGlueInfo->fgIsRegistered = FALSE;

	DBGLOG(INIT, INFO, ("unregister wireless_dev(0x%p)\n", prWdev));

	return;
}				/* end of wlanNetUnregister() */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
static const struct net_device_ops wlan_netdev_ops = {
	.ndo_open = wlanOpen,
	.ndo_stop = wlanStop,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
	.ndo_set_rx_mode = wlanSetMulticastList,
#else
	.ndo_set_multicast_list = wlanSetMulticastList,
#endif
	.ndo_get_stats = wlanGetStats,
	.ndo_do_ioctl = wlanDoIOCTL,
	.ndo_start_xmit = wlanHardStartXmit,
	.ndo_init = wlanInit,
	.ndo_uninit = wlanUninit,
	.ndo_select_queue = wlanSelectQueue,
};
#endif

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support wlan_wowlan_support = {
	.flags = WIPHY_WOWLAN_DISCONNECT | WIPHY_WOWLAN_ANY,
};
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief A method for creating Linux NET4 struct net_device object and the
*        private data(prGlueInfo and prAdapter). Setup the IO address to the HIF.
*        Assign the function pointer to the net_device object
*
* \param[in] pvData     Memory address for the device
*
* \retval Not null      The wireless_dev object.
* \retval NULL          Fail to create wireless_dev object
*/
/*----------------------------------------------------------------------------*/
static struct lock_class_key rSpinKey[SPIN_LOCK_NUM];
static struct wireless_dev *wlanNetCreate(PVOID pvData)
{
	struct wireless_dev *prWdev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	UINT_32 i;
	struct device *prDev;

	/* 4 <1.1> Create wireless_dev */
	prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	DBGLOG(INIT, INFO, ("wireless_dev prWdev(0x%p) allocated\n", prWdev));
	if (!prWdev) {
		DBGLOG(INIT, ERROR, ("Allocating memory to wireless_dev context failed\n"));
		return NULL;
	}
	/* 4 <1.2> Create wiphy */
	prWdev->wiphy = wiphy_new(&mtk_wlan_ops, sizeof(GLUE_INFO_T));
	DBGLOG(INIT, INFO, ("wiphy (0x%p) allocated\n", prWdev->wiphy));
	if (!prWdev->wiphy) {
		DBGLOG(INIT, ERROR, ("Allocating memory to wiphy device failed\n"));
		kfree(prWdev);
		return NULL;
	}
	/* 4 <1.3> co-relate wiphy & prDev */
#if MTK_WCN_HIF_SDIO
	mtk_wcn_hif_sdio_get_dev(*((MTK_WCN_HIF_SDIO_CLTCTX *) pvData), &prDev);
#else
	prDev = ((struct sdio_func *)pvData)->dev;
#endif
	if (!prDev) {
		printk(KERN_ALERT DRV_NAME "unable to get struct dev for wlan\n");
	}
	set_wiphy_dev(prWdev->wiphy, prDev);

	/* 4 <1.4> configure wireless_dev & wiphy */
	prWdev->iftype = NL80211_IFTYPE_STATION;
	prWdev->wiphy->max_scan_ssids = 1;	/* FIXME: for combo scan */
	prWdev->wiphy->max_scan_ie_len = 512;
	prWdev->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_ADHOC);
	prWdev->wiphy->bands[IEEE80211_BAND_2GHZ] = &mtk_band_2ghz;
	/* for the WPS probe request suband issue */
	/* prWdev->wiphy->bands[IEEE80211_BAND_5GHZ] = &mtk_band_5ghz; */
	prWdev->wiphy->bands[IEEE80211_BAND_5GHZ] = NULL;
	prWdev->wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	prWdev->wiphy->cipher_suites = (const u32 *)mtk_cipher_suites;
	prWdev->wiphy->n_cipher_suites = ARRAY_SIZE(mtk_cipher_suites);
	prWdev->wiphy->flags = WIPHY_FLAG_CUSTOM_REGULATORY | WIPHY_FLAG_SUPPORTS_FW_ROAM;
#ifdef CONFIG_PM
	kalMemCopy(&prWdev->wiphy->wowlan, &wlan_wowlan_support,
		   sizeof(struct wiphy_wowlan_support));
#endif
	/* 4 <2> Create Glue structure */
	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, ("Allocating memory to glue layer failed\n"));
		goto netcreate_err;
	}
	/* 4 <3> Initial Glue structure */
	/* 4 <3.1> create net device */
	prGlueInfo->prDevHandler =
	    alloc_netdev_mq(sizeof(P_GLUE_INFO_T), NIC_INF_NAME, ether_setup, CFG_MAX_TXQ_NUM);

	DBGLOG(INIT, INFO, ("net_device prDev(0x%p) allocated\n", prGlueInfo->prDevHandler));
	if (!prGlueInfo->prDevHandler) {
		DBGLOG(INIT, ERROR, ("Allocating memory to net_device context failed\n"));
		goto netcreate_err;
	}
	/* 4 <3.1.1> initialize net device varaiables */
	*((P_GLUE_INFO_T *) netdev_priv(prGlueInfo->prDevHandler)) = prGlueInfo;

	prGlueInfo->prDevHandler->netdev_ops = &wlan_netdev_ops;
#ifdef CONFIG_WIRELESS_EXT
	prGlueInfo->prDevHandler->wireless_handlers = &wext_handler_def;
#endif
	netif_carrier_off(prGlueInfo->prDevHandler);
	netif_tx_stop_all_queues(prGlueInfo->prDevHandler);

	/* 4 <3.1.2> co-relate with wiphy bi-directionally */
	prGlueInfo->prDevHandler->ieee80211_ptr = prWdev;
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	prGlueInfo->prDevHandler->features = NETIF_F_HW_CSUM;
#endif
	prWdev->netdev = prGlueInfo->prDevHandler;

	/* 4 <3.1.3> co-relate net device & prDev */
	SET_NETDEV_DEV(prGlueInfo->prDevHandler, wiphy_dev(prWdev->wiphy));

	/* 4 <3.2> initiali glue variables */
	prGlueInfo->eParamMediaStateIndicated = PARAM_MEDIA_STATE_DISCONNECTED;
	prGlueInfo->ePowerState = ParamDeviceStateD0;
	prGlueInfo->fgIsMacAddrOverride = FALSE;
	prGlueInfo->fgIsRegistered = FALSE;
	prGlueInfo->prScanRequest = NULL;

	init_completion(&prGlueInfo->rScanComp);
	init_completion(&prGlueInfo->rHaltComp);
	init_completion(&prGlueInfo->rPendComp);
#if CFG_ENABLE_WIFI_DIRECT
	init_completion(&prGlueInfo->rSubModComp);
#endif

	/* initialize timer for OID timeout checker */
	kalOsTimerInitialize(prGlueInfo, kalTimeoutHandler);

	for (i = 0; i < SPIN_LOCK_NUM; i++) {
		spin_lock_init(&prGlueInfo->rSpinLock[i]);
		lockdep_set_class(&prGlueInfo->rSpinLock[i], &rSpinKey[i]);
	}

	/* initialize semaphore for ioctl */
	sema_init(&prGlueInfo->ioctl_sem, 1);

	/* initialize semaphore for ioctl */
	sema_init(&g_halt_sem, 1);
	g_u4HaltFlag = 0;

	/* 4 <4> Create Adapter structure */
	prAdapter = (P_ADAPTER_T) wlanAdapterCreate(prGlueInfo);

	if (!prAdapter) {
		DBGLOG(INIT, ERROR, ("Allocating memory to adapter failed\n"));
		goto netcreate_err;
	}

	prGlueInfo->prAdapter = prAdapter;

#ifdef CONFIG_CFG80211_WEXT
	/* 4 <5> Use wireless extension to replace IOCTL */
	prWdev->wiphy->wext = &wext_handler_def;
#endif

	goto netcreate_done;

 netcreate_err:
	if (NULL != prAdapter) {
		wlanAdapterDestroy(prAdapter);
		prAdapter = NULL;
	}

	if (NULL != prGlueInfo->prDevHandler) {
		free_netdev(prGlueInfo->prDevHandler);
		prGlueInfo->prDevHandler = NULL;
	}

	if (NULL != prWdev->wiphy) {
		wiphy_free(prWdev->wiphy);
		prWdev->wiphy = NULL;
	}

	if (NULL != prWdev) {
		/* Free net_device and private data, which are allocated by
		 * alloc_netdev().
		 */
		kfree(prWdev);
		prWdev = NULL;
	}

 netcreate_done:

	return prWdev;
}				/* end of wlanNetCreate() */


/*----------------------------------------------------------------------------*/
/*!
* \brief Destroying the struct net_device object and the private data.
*
* \param[in] prWdev      Pointer to struct wireless_dev.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID wlanNetDestroy(struct wireless_dev *prWdev)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prWdev);

	if (!prWdev) {
		DBGLOG(INIT, ERROR, ("wlanNetDestroy: The device context is NULL\n"));
		return;
	}

	/* prGlueInfo is allocated with net_device */
	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);
	ASSERT(prGlueInfo);

	/* destroy kal OS timer */
	kalCancelTimer(prGlueInfo);

	glClearHifInfo(prGlueInfo);

	wlanAdapterDestroy(prGlueInfo->prAdapter);
	prGlueInfo->prAdapter = NULL;

	/* Free net_device and private data, which are allocated by alloc_netdev().
	 */
	free_netdev(prWdev->netdev);
	wiphy_free(prWdev->wiphy);

	kfree(prWdev);

	return;
}				/* end of wlanNetDestroy() */

#ifndef CONFIG_X86
UINT_8 g_aucBufIpAddr[32] = { 0 };

static void wlanEarlySuspend(void)
{
	struct in_device *in_dev;	/* ALPS00409409406 */
	struct net_device *prDev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_8 ip[4] = { 0 };
	UINT_32 u4NumIPv4 = 0;
#if defined(CONFIG_IPV6) && defined(ENABLE_IPV6_WLAN)
	UINT_8 ip6[16] = { 0 };	/* FIX ME: avoid to allocate large memory in stack */
#endif
	UINT_32 u4NumIPv6 = 0;
	UINT_32 i;
	P_PARAM_NETWORK_ADDRESS_IP prParamIpAddr;

#if defined(MTK_PACKET_FILTERING_SUPPORT)
	UINT_32 u4PacketFilter = 0, u4SetInfoLen = 0, u4QueryInfoLen = 0;
	WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
#endif


	DBGLOG(INIT, INFO, ("*********wlanEarlySuspend************\n"));

	/* <1> Sanity check and acquire the net_device */
	ASSERT(u4WlanDevNum <= CFG_MAX_WLAN_DEVICES);
	if (u4WlanDevNum == 0) {
		DBGLOG(INIT, ERROR, ("wlanEarlySuspend u4WlanDevNum==0 invalid!!\n"));
		return;
	}
	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	ASSERT(prDev);

	/* <2> acquire the prGlueInfo */
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);

#ifdef FIX_ALPS00409409406
	atomic_set(&fgIsUnderEarlierSuspend, 1);
#else
	fgIsUnderEarlierSuspend = true;
#endif

	/* ALPS00409409406 */
#ifdef FIX_ALPS00409409406
	/* <3> get the IPv4 address */
	in_dev = in_dev_get(prDev);
	if (!in_dev)
		return;

	/* rtnl_lock(); */
	if (!in_dev->ifa_list || !in_dev->ifa_list->ifa_local) {
		in_dev_put(in_dev);
		/* rtnl_unlock(); */
		DBGLOG(INIT, INFO, ("ip is not avaliable.\n"));
		return;
	}
	/* <4> copy the IPv4 address */
	kalMemCopy(ip, &(in_dev->ifa_list->ifa_local), sizeof(ip));
	/* rtnl_unlock(); */
	in_dev_put(in_dev);

	DBGLOG(INIT, INFO, ("ip is %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]));

#else

	/* <3> get the IPv4 address */
	if (!prDev || !(prDev->ip_ptr) ||
	    !((struct in_device *)(prDev->ip_ptr))->ifa_list ||
	    !(&(((struct in_device *)(prDev->ip_ptr))->ifa_list->ifa_local))) {
		DBGLOG(INIT, INFO, ("ip is not avaliable.\n"));
		return;
	}
	/* <4> copy the IPv4 address */
	kalMemCopy(ip, &(((struct in_device *)(prDev->ip_ptr))->ifa_list->ifa_local), sizeof(ip));
	DBGLOG(INIT, INFO, ("ip is %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]));
#endif


	/* <5> set the rx filter */
#if defined(MTK_PACKET_FILTERING_SUPPORT)
	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryCurrentPacketFilter,
			   &u4PacketFilter,
			   sizeof(u4PacketFilter), FALSE, FALSE, TRUE, FALSE, &u4QueryInfoLen);


	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, ("wlanoidQueryCurrentPacketFilter fail 0x%lx\n", rStatus));
	} else {
		DBGLOG(INIT, INFO, ("wlanoidQueryCurrentPacketFilter 0x%lx\n", u4PacketFilter));
	}

/* u4PacketFilter &= ~PARAM_PACKET_FILTER_BROADCAST;   It will filter all the broadcast packet. */
	u4PacketFilter &= ~PARAM_PACKET_FILTER_P2P_MASK;
	u4PacketFilter &= ~PARAM_PACKET_FILTER_MULTICAST;

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetCurrentPacketFilter,
			   &u4PacketFilter,
			   sizeof(u4PacketFilter), FALSE, FALSE, TRUE, FALSE, &u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, ("wlanoidSetCurrentPacketFilter fail 0x%lx\n", rStatus));
	} else {
		DBGLOG(INIT, INFO, ("wlanoidSetCurrentPacketFilter 0x%lx\n", u4PacketFilter));
	}

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryCurrentPacketFilter,
			   &u4PacketFilter,
			   sizeof(u4PacketFilter), FALSE, FALSE, TRUE, FALSE, &u4QueryInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, ("wlanoidQueryCurrentPacketFilter fail 0x%lx\n", rStatus));
	} else {
		DBGLOG(INIT, INFO, ("wlanoidQueryCurrentPacketFilter 0x%lx\n", u4PacketFilter));
	}

	g_packet_switch = 1;
#endif

	/* todo: traverse between list to find whole sets of IPv4 addresses */
	if (!((ip[0] == 0) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 0))) {
		u4NumIPv4++;
	}
#if defined(CONFIG_IPV6) && defined(ENABLE_IPV6_WLAN)
#if 0
	if (!prDev || !(prDev->ip6_ptr) ||
	    !((struct in_device *)(prDev->ip6_ptr))->ifa_list ||
	    !(&(((struct in_device *)(prDev->ip6_ptr))->ifa_list->ifa_local))) {
		DBGLOG(INIT, INFO, ("ipv6 is not avaliable.\n"));
		return;
	}
	/* <6> copy the IPv6 address */
	kalMemCopy(ip6, &(((struct in_device *)(prDev->ip6_ptr))->ifa_list->ifa_local),
		   sizeof(ip6));
	DBGLOG(INIT, INFO,
	       ("ipv6 is %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n", ip6[0], ip6[1], ip6[2],
		ip6[3], ip6[4], ip6[5], ip6[6], ip6[7], ip6[8], ip6[9], ip6[10], ip6[11], ip6[12],
		ip6[13], ip6[14], ip6[15]
	       ));

	/* todo: traverse between list to find whole sets of IPv6 addresses */
	if (!((ip6[0] == 0) &&
	      (ip6[1] == 0) && (ip6[2] == 0) && (ip6[3] == 0) && (ip6[4] == 0) && (ip6[5] == 0))) {
		/* u4NumIPv6++; */
	}
#else
	{
		struct inet6_dev *in6_dev = NULL;

		in6_dev = in6_dev_get(prDev);
		if (!in6_dev)
			return;

		/* rtnl_lock(); */
		if (!in6_dev->ac_list || !in6_dev->ac_list->aca_addr.s6_addr16) {
			/* rtnl_unlock(); */
			in6_dev_put(in6_dev);
			DBGLOG(INIT, INFO, ("ipv6 is not avaliable.\n"));
			return;
		}
		/* <4> copy the IPv6 address */
		kalMemCopy(ip6, in6_dev->ac_list->aca_addr.s6_addr16, sizeof(ip6));
		/* rtnl_unlock(); */
		in6_dev_put(in6_dev);

		DBGLOG(INIT, INFO, ("ipv6 is %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n",
				    ip6[0], ip6[1], ip6[2], ip6[3],
				    ip6[4], ip6[5], ip6[6], ip6[7],
				    ip6[8], ip6[9], ip6[10], ip6[11],
				    ip6[12], ip6[13], ip6[14], ip6[15]
		       ));

		/* todo: traverse between list to find whole sets of IPv6 addresses */
		if (!((ip6[0] == 0) &&
		      (ip6[1] == 0) &&
		      (ip6[2] == 0) && (ip6[3] == 0) && (ip6[4] == 0) && (ip6[5] == 0))) {
			u4NumIPv6++;
		}

	}
#endif


#endif

	/* <7> set up the ARP filter */
	{
		WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
		UINT_32 u4SetInfoLen = 0;
/* UINT_8 aucBuf[32] = {0}; */
		UINT_32 u4Len = OFFSET_OF(PARAM_NETWORK_ADDRESS_LIST, arAddress);
		P_PARAM_NETWORK_ADDRESS_LIST prParamNetAddrList = (P_PARAM_NETWORK_ADDRESS_LIST) g_aucBufIpAddr;	/* aucBuf; */
		P_PARAM_NETWORK_ADDRESS prParamNetAddr = prParamNetAddrList->arAddress;

		kalMemZero(g_aucBufIpAddr, sizeof(g_aucBufIpAddr));

		prParamNetAddrList->u4AddressCount = u4NumIPv4 + u4NumIPv6;
		prParamNetAddrList->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
		for (i = 0; i < u4NumIPv4; i++) {
			prParamNetAddr->u2AddressLength = sizeof(PARAM_NETWORK_ADDRESS_IP);	/* 4;; */
			prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
#if 0
			kalMemCopy(prParamNetAddr->aucAddress, ip, sizeof(ip));
			prParamNetAddr =
			    (P_PARAM_NETWORK_ADDRESS) ((UINT_32) prParamNetAddr + sizeof(ip));
			u4Len += OFFSET_OF(PARAM_NETWORK_ADDRESS, aucAddress) + sizeof(ip);
#else
			prParamIpAddr = (P_PARAM_NETWORK_ADDRESS_IP) prParamNetAddr->aucAddress;
			kalMemCopy(&prParamIpAddr->in_addr, ip, sizeof(ip));
			prParamNetAddr =
			    (P_PARAM_NETWORK_ADDRESS) ((UINT_32) prParamNetAddr +
						       sizeof(PARAM_NETWORK_ADDRESS));
			u4Len +=
			    OFFSET_OF(PARAM_NETWORK_ADDRESS,
				      aucAddress) + sizeof(PARAM_NETWORK_ADDRESS);
#endif
		}
#if defined(CONFIG_IPV6) && defined(ENABLE_IPV6_WLAN)
		for (i = 0; i < u4NumIPv6; i++) {
			prParamNetAddr->u2AddressLength = 6;
			prParamNetAddr->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;
			kalMemCopy(prParamNetAddr->aucAddress, ip6, sizeof(ip6));
			prParamNetAddr =
			    (P_PARAM_NETWORK_ADDRESS) ((UINT_32) prParamNetAddr + sizeof(ip6));
			u4Len += OFFSET_OF(PARAM_NETWORK_ADDRESS, aucAddress) + sizeof(ip6);
		}
#endif
		ASSERT(u4Len <= sizeof(g_aucBufIpAddr /*aucBuf */));

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetNetworkAddress,
				   (PVOID) prParamNetAddrList,
				   u4Len, FALSE, FALSE, TRUE, FALSE, &u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, INFO, ("set HW pattern filter fail 0x%lx\n", rStatus));
		}
	}
}

static void wlanLateResume(void)
{
	struct in_device *in_dev;	/* ALPS00409409406 */
	struct net_device *prDev = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
#ifdef TEST_FOR_NET_RCU_LOCK
	UINT_8 ip[4] = { 0 };
#endif
#ifdef CONFIG_IPV6
	UINT_8 ip6[16] = { 0 };	/* FIX ME: avoid to allocate large memory in stack */
#endif
#if defined(MTK_PACKET_FILTERING_SUPPORT)
	UINT_32 u4PacketFilter = 0, u4SetInfoLen = 0, u4QueryInfoLen = 0;
	WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
#endif
	DBGLOG(INIT, INFO, ("*********wlanLateResume************\n"));

	/* <1> Sanity check and acquire the net_device */
	ASSERT(u4WlanDevNum <= CFG_MAX_WLAN_DEVICES);
	if (u4WlanDevNum == 0) {
		DBGLOG(INIT, ERROR, ("wlanLateResume u4WlanDevNum==0 invalid!!\n"));
		return;
	}
	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (NULL == prDev)
		return;
	ASSERT(prDev);

	/* <2> acquire the prGlueInfo */
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);

#ifdef FIX_ALPS00409409406
	atomic_set(&fgIsUnderEarlierSuspend, 0);
#else
	fgIsUnderEarlierSuspend = false;
#endif

	/* <3> get the IPv4 address */
	/* ALPS00409409406 */
#ifdef FIX_ALPS00409409406
	in_dev = in_dev_get(prDev);
	if (!in_dev)
		return;

	/* rtnl_lock(); */
	if (!in_dev->ifa_list || !in_dev->ifa_list->ifa_local) {
		in_dev_put(in_dev);
		/* rtnl_unlock(); */
		DBGLOG(INIT, INFO, ("ip is not avaliable.\n"));
		return;
	}
#ifdef TEST_FOR_NET_RCU_LOCK
	{
		int i;
		for (i = 0; i < 1000; i++) {
			volatile struct in_device *inDev = in_dev;
			volatile struct in_ifaddr *inIfaddr = inDev;
			kalMemCopy(ip, &(inIfaddr->ifa_local), sizeof(ip));
			DBGLOG(INIT, INFO, ("ip is %d,%d%d,%d\n", ip[0], ip[1], ip[2], ip[3]));
		}
	}
#endif
	in_dev_put(in_dev);
	/* rtnl_unlock(); */

#else
	if (!prDev || !(prDev->ip_ptr) ||
	    !((struct in_device *)(prDev->ip_ptr))->ifa_list ||
	    !(&(((struct in_device *)(prDev->ip_ptr))->ifa_list->ifa_local))) {
		DBGLOG(INIT, INFO, ("ip is not avaliable.\n"));
		return;
	}
#endif


#if defined(CONFIG_IPV6) && defined(ENABLE_IPV6_WLAN)

#if 0
	/* <4> get the IPv6 address */
	if (!prDev || !(prDev->ip6_ptr) ||
	    !((struct in_device *)(prDev->ip6_ptr))->ifa_list ||
	    !(&(((struct in_device *)(prDev->ip6_ptr))->ifa_list->ifa_local))) {
		DBGLOG(INIT, INFO, ("ipv6 is not avaliable.\n"));
		return;
	}
	/* <5> copy the IPv6 address */
	kalMemCopy(ip6, &(((struct in_device *)(prDev->ip6_ptr))->ifa_list->ifa_local),
		   sizeof(ip6));
	DBGLOG(INIT, INFO,
	       ("ipv6 is %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n", ip6[0], ip6[1], ip6[2],
		ip6[3], ip6[4], ip6[5], ip6[6], ip6[7], ip6[8], ip6[9], ip6[10], ip6[11], ip6[12],
		ip6[13], ip6[14], ip6[15]
	       ));
#else
	{
		struct inet6_dev *in6_dev = NULL;

		in6_dev = in6_dev_get(prDev);
		if (!in6_dev)
			return;

		/* rtnl_lock(); */
		if (!in6_dev->ac_list || !in6_dev->ac_list->aca_addr.s6_addr16) {
			/* rtnl_unlock(); */
			in6_dev_put(in6_dev);
			DBGLOG(INIT, INFO, ("ipv6 is not avaliable.\n"));
			return;
		}
		/* rtnl_unlock(); */
		in6_dev_put(in6_dev);
	}
#endif

#endif

	/* <6> clear the Rx filter */
#if defined(MTK_PACKET_FILTERING_SUPPORT)
	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryCurrentPacketFilter,
			   &u4PacketFilter,
			   sizeof(u4PacketFilter), FALSE, FALSE, TRUE, FALSE, &u4QueryInfoLen);

/* u4PacketFilter |= PARAM_PACKET_FILTER_BROADCAST|PARAM_PACKET_FILTER_MULTICAST; */
	u4PacketFilter &= ~PARAM_PACKET_FILTER_P2P_MASK;
	u4PacketFilter |= PARAM_PACKET_FILTER_MULTICAST;	/* only enable multicast filter. */

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, ("wlanoidQueryCurrentPacketFilter fail 0x%lx\n", rStatus));
	} else {
		DBGLOG(INIT, INFO, ("wlanoidQueryCurrentPacketFilter 0x%lx\n", u4PacketFilter));
	}

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetCurrentPacketFilter,
			   &u4PacketFilter,
			   sizeof(u4PacketFilter), FALSE, FALSE, TRUE, FALSE, &u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, ("wlanoidSetCurrentPacketFilter fail 0x%lx\n", rStatus));
	} else {
		DBGLOG(INIT, INFO, ("wlanoidSetCurrentPacketFilter  0x%lx\n", u4PacketFilter));
	}

	g_packet_switch = 0;
#endif

#ifdef FIX_ALPS00409409406
#ifdef TEST_FOR_NET_RCU_LOCK
	in_dev = in_dev_get(prDev);
	if (in_dev) {
		int i;
		for (i = 0; i < 1000; i++) {
			volatile struct in_device *inDev = in_dev;
			volatile struct in_ifaddr *inIfaddr = inDev;
			kalMemCopy(ip, &(inIfaddr->ifa_local), sizeof(ip));
			DBGLOG(INIT, INFO, ("ip is %d,%d%d,%d\n", ip[0], ip[1], ip[2], ip[3]));
		}
		in_dev_put(in_dev);
	}
#endif
#endif


	/* <7> clear the ARP filter */
	{
		WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
		UINT_32 u4SetInfoLen = 0;
/* UINT_8 aucBuf[32] = {0}; */
		UINT_32 u4Len = sizeof(PARAM_NETWORK_ADDRESS_LIST);
		P_PARAM_NETWORK_ADDRESS_LIST prParamNetAddrList = (P_PARAM_NETWORK_ADDRESS_LIST) g_aucBufIpAddr;	/* aucBuf; */

		kalMemZero(g_aucBufIpAddr, sizeof(g_aucBufIpAddr));

		prParamNetAddrList->u4AddressCount = 0;
		prParamNetAddrList->u2AddressType = PARAM_PROTOCOL_ID_TCP_IP;

		ASSERT(u4Len <= sizeof(g_aucBufIpAddr /*aucBuf */));
		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetNetworkAddress,
				   (PVOID) prParamNetAddrList,
				   u4Len, FALSE, FALSE, TRUE, FALSE, &u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, INFO, ("set HW pattern filter fail 0x%lx\n", rStatus));
		}
	}
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static struct early_suspend mt6620_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
};

static void wlan_early_suspend(struct early_suspend *h)
{
	DBGLOG(INIT, INFO, ("*********wlan_early_suspend************\n"));
	wlanEarlySuspend();
}

static void wlan_late_resume(struct early_suspend *h)
{
	DBGLOG(INIT, INFO, ("*********wlan_late_resume************\n"));
	wlanLateResume();
}
#endif				/* defined(CONFIG_HAS_EARLYSUSPEND) */
#endif				/* ! CONFIG_X86 */

extern void wlanRegisterNotifier(void);
extern void wlanUnregisterNotifier(void);


typedef int (*set_p2p_mode) (struct net_device *netdev, PARAM_CUSTOM_P2P_SET_STRUC_T p2pmode);
typedef void (*set_dbg_level) (unsigned char modules[DBG_MODULE_NUM]);

extern void register_set_p2p_mode_handler(set_p2p_mode handler);
extern void register_set_dbg_level_handler(set_dbg_level handler);

static int set_p2p_mode_handler(struct net_device *netdev, PARAM_CUSTOM_P2P_SET_STRUC_T p2pmode)
{
	extern BOOLEAN fgIsResetting;
	P_GLUE_INFO_T prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(netdev));
	PARAM_CUSTOM_P2P_SET_STRUC_T rSetP2P;
	WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;

	rSetP2P.u4Enable = p2pmode.u4Enable;
	rSetP2P.u4Mode = p2pmode.u4Mode;

	if ((!rSetP2P.u4Enable) && (fgIsResetting == FALSE)) {
		p2pNetUnregister(prGlueInfo, TRUE);
	}

	rWlanStatus = kalIoctl(prGlueInfo,
			       wlanoidSetP2pMode,
			       (PVOID) &rSetP2P,
			       sizeof(PARAM_CUSTOM_P2P_SET_STRUC_T),
			       FALSE, FALSE, TRUE, FALSE, &u4BufLen);

	DBGLOG(INIT, INFO, ("set_p2p_mode_handler ret = 0x%08lx\n", (UINT_32) rWlanStatus));

	/* Need to check fgIsP2PRegistered, in case of whole chip reset.
	 * in this case, kalIOCTL return success always, 
	 * and prGlueInfo->prP2pInfo may be NULL */
	if ((rSetP2P.u4Enable) && (prGlueInfo->prAdapter->fgIsP2PRegistered) &&
	    (fgIsResetting == FALSE)) {
		p2pNetRegister(prGlueInfo, TRUE);
	}

	return 0;
}

static void set_dbg_level_handler(unsigned char dbg_lvl[DBG_MODULE_NUM])
{
	kalMemCopy(aucDebugModule, dbg_lvl, sizeof(aucDebugModule));
	kalPrint("[wlan] change debug level");
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Wlan probe function. This function probes and initializes the device.
*
* \param[in] pvData     data passed by bus driver init function
*                           _HIF_EHPI: NULL
*                           _HIF_SDIO: sdio bus driver handle
*
* \retval 0 Success
* \retval negative value Failed
*/
/*----------------------------------------------------------------------------*/
static INT_32 wlanProbe(PVOID pvData)
{
	struct wireless_dev *prWdev = NULL;
	P_WLANDEV_INFO_T prWlandevInfo = NULL;
	INT_32 i4DevIdx = 0;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	INT_32 i4Status = 0;
	BOOL bRet = FALSE;


	do {
		/* 4 <1> Initialize the IO port of the interface */
		/*  GeorgeKuo: pData has different meaning for _HIF_XXX:
		 * _HIF_EHPI: pointer to memory base variable, which will be
		 *      initialized by glBusInit().
		 * _HIF_SDIO: bus driver handle
		 */

		bRet = glBusInit(pvData);

		/* Cannot get IO address from interface */
		if (FALSE == bRet) {
			DBGLOG(INIT, ERROR, (KERN_ALERT "wlanProbe: glBusInit() fail\n"));
			i4Status = -EIO;
			break;
		}
		/* 4 <2> Create network device, Adapter, KalInfo, prDevHandler(netdev) */
		if ((prWdev = wlanNetCreate(pvData)) == NULL) {
			DBGLOG(INIT, ERROR, ("wlanProbe: No memory for dev and its private\n"));
			i4Status = -ENOMEM;
			break;
		}
		/* 4 <2.5> Set the ioaddr to HIF Info */
		prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(prWdev->wiphy);
		gPrDev = prGlueInfo->prDevHandler;
		glSetHifInfo(prGlueInfo, (UINT_32) pvData);


		/* main thread is created in this function */
		init_waitqueue_head(&prGlueInfo->waitq);
		/*  */

		QUEUE_INITIALIZE(&prGlueInfo->rCmdQueue);
		QUEUE_INITIALIZE(&prGlueInfo->rTxQueue);


		/* prGlueInfo->main_thread = kthread_run(tx_thread, prGlueInfo->prDevHandler, "tx_thread"); */

		/* 4 <4> Setup IRQ */
		prWlandevInfo = &arWlanDevInfo[i4DevIdx];

		/* i4Status = glBusSetIrq(prWdev->netdev, NULL, *((P_GLUE_INFO_T *) netdev_priv(prWdev->netdev))); */

		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, ("wlanProbe: Set IRQ error\n"));
			break;
		}

		prGlueInfo->i4DevIdx = i4DevIdx;

		prAdapter = prGlueInfo->prAdapter;

		prGlueInfo->u4ReadyFlag = 0;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
		prAdapter->u4CSUMFlags =
		    (CSUM_OFFLOAD_EN_TX_TCP | CSUM_OFFLOAD_EN_TX_UDP | CSUM_OFFLOAD_EN_TX_IP);
#endif

		/* 4 <5> Start Device */
		/*  */
#if CFG_ENABLE_FW_DOWNLOAD
		/* before start adapter, we need to open and load firmware */
		{
			UINT_32 u4FwSize = 0;
			PVOID prFwBuffer = NULL;
			P_REG_INFO_T prRegInfo = &prGlueInfo->rRegInfo;

			/* P_REG_INFO_T prRegInfo = (P_REG_INFO_T) kmalloc(sizeof(REG_INFO_T), GFP_KERNEL); */
			kalMemSet(prRegInfo, 0, sizeof(REG_INFO_T));
			prRegInfo->u4StartAddress = CFG_FW_START_ADDRESS;
			prRegInfo->u4LoadAddress = CFG_FW_LOAD_ADDRESS;

			/* Load NVRAM content to REG_INFO_T */
			glLoadNvram(prGlueInfo, prRegInfo);

			/* kalMemCopy(&prGlueInfo->rRegInfo, prRegInfo, sizeof(REG_INFO_T)); */

			prRegInfo->u4PowerMode = CFG_INIT_POWER_SAVE_PROF;
			prRegInfo->fgEnArpFilter = TRUE;

			if (kalFirmwareImageMapping(prGlueInfo, &prFwBuffer, &u4FwSize) == NULL) {
				i4Status = -EIO;
				goto bailout;
			} else {
				if (wlanAdapterStart(prAdapter, prRegInfo, prFwBuffer, u4FwSize) !=
				    WLAN_STATUS_SUCCESS) {
					i4Status = -EIO;
				}
			}

			kalFirmwareImageUnmapping(prGlueInfo, NULL, prFwBuffer);

 bailout:
			/* kfree(prRegInfo); */

			if (i4Status < 0) {
				break;
			}
		}
#else
		/* P_REG_INFO_T prRegInfo = (P_REG_INFO_T) kmalloc(sizeof(REG_INFO_T), GFP_KERNEL); */
		kalMemSet(&prGlueInfo->rRegInfo, 0, sizeof(REG_INFO_T));
		P_REG_INFO_T prRegInfo = &prGlueInfo->rRegInfo;

		/* Load NVRAM content to REG_INFO_T */
		glLoadNvram(prGlueInfo, prRegInfo);

		prRegInfo->u4PowerMode = CFG_INIT_POWER_SAVE_PROF;

		if (wlanAdapterStart(prAdapter, prRegInfo, NULL, 0) != WLAN_STATUS_SUCCESS) {
			i4Status = -EIO;
			break;
		}
#endif
		if (TRUE == prAdapter->fgEnable5GBand)
			prWdev->wiphy->bands[IEEE80211_BAND_5GHZ] = &mtk_band_5ghz;

		prGlueInfo->main_thread =
		    kthread_run(tx_thread, prGlueInfo->prDevHandler, "tx_thread");

		/* set MAC address */
		{
			WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
			struct sockaddr MacAddr;
			UINT_32 u4SetInfoLen = 0;

			rStatus = kalIoctl(prGlueInfo,
					   wlanoidQueryCurrentAddr,
					   &MacAddr.sa_data,
					   PARAM_MAC_ADDR_LEN,
					   TRUE, TRUE, TRUE, FALSE, &u4SetInfoLen);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, WARN, ("set MAC addr fail 0x%lx\n", rStatus));
				prGlueInfo->u4ReadyFlag = 0;
			} else {
				memcpy(prGlueInfo->prDevHandler->dev_addr, &MacAddr.sa_data,
				       ETH_ALEN);
				memcpy(prGlueInfo->prDevHandler->perm_addr,
				       prGlueInfo->prDevHandler->dev_addr, ETH_ALEN);

				/* card is ready */
				prGlueInfo->u4ReadyFlag = 1;
#if CFG_SHOW_MACADDR_SOURCE
				DBGLOG(INIT, INFO,
				       ("MAC address: " MACSTR, MAC2STR(&MacAddr.sa_data)));
#endif
			}
		}


#if CFG_TCP_IP_CHKSUM_OFFLOAD
		/* set HW checksum offload */
		{
			WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
			UINT_32 u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
			UINT_32 u4SetInfoLen = 0;

			rStatus = kalIoctl(prGlueInfo,
					   wlanoidSetCSUMOffload,
					   (PVOID) &u4CSUMFlags,
					   sizeof(UINT_32),
					   FALSE, FALSE, TRUE, FALSE, &u4SetInfoLen);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, WARN,
				       ("set HW checksum offload fail 0x%lx\n", rStatus));
			}
		}
#endif

		/* 4 <3> Register the card */
		if ((i4DevIdx = wlanNetRegister(prWdev)) < 0) {
			i4Status = -ENXIO;
			DBGLOG(INIT, ERROR,
			       ("wlanProbe: Cannot register the net_device context to the kernel\n"));
			break;
		}
#if defined(CONFIG_HAS_EARLYSUSPEND)
		glRegisterEarlySuspend(&mt6620_early_suspend_desc, wlan_early_suspend,
				       wlan_late_resume);
		wlanRegisterNotifier();
#endif

		/* 4 <6> Initialize /proc filesystem */
#ifdef WLAN_INCLUDE_PROC
		if ((i4Status = procInitProcfs(prDev, NIC_DEVICE_ID_LOW)) < 0) {
			DBGLOG(INIT, ERROR, ("wlanProbe: init procfs failed\n"));
			break;
		}
#endif				/* WLAN_INCLUDE_PROC */

#if CFG_ENABLE_BT_OVER_WIFI
		prGlueInfo->rBowInfo.fgIsNetRegistered = FALSE;
		prGlueInfo->rBowInfo.fgIsRegistered = FALSE;
		glRegisterAmpc(prGlueInfo);
#endif

#if CFG_ENABLE_WIFI_DIRECT
		/* wlan is launched */
		prGlueInfo->prAdapter->fgIsWlanLaunched = TRUE;
		/* if p2p module is inserted, notify tx_thread to init p2p network */
		if (rSubModHandler[P2P_MODULE].subModInit) {
			wlanSubModInit(prGlueInfo);
		}
		/* register set_p2p_mode handler to mtk_wmt_wifi */
		register_set_p2p_mode_handler(set_p2p_mode_handler);
#endif
	}
	while (FALSE);

	return i4Status;
}				/* end of wlanProbe() */


/*----------------------------------------------------------------------------*/
/*!
* \brief A method to stop driver operation and release all resources. Following
*        this call, no frame should go up or down through this interface.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static VOID wlanRemove(VOID)
{
	struct net_device *prDev = NULL;
	P_WLANDEV_INFO_T prWlandevInfo = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;

	DBGLOG(INIT, INFO, ("Remove wlan!\n"));


	/* 4 <0> Sanity check */
	ASSERT(u4WlanDevNum <= CFG_MAX_WLAN_DEVICES);
	if (0 == u4WlanDevNum) {
		DBGLOG(INIT, INFO, ("0 == u4WlanDevNum\n"));
		return;
	}
	/* unregister set_p2p_mode handler to mtk_wmt_wifi */
	register_set_p2p_mode_handler(NULL);

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	prWlandevInfo = &arWlanDevInfo[u4WlanDevNum - 1];

	ASSERT(prDev);
	if (NULL == prDev) {
		DBGLOG(INIT, INFO, ("NULL == prDev\n"));
		return;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);
	if (NULL == prGlueInfo) {
		DBGLOG(INIT, INFO, ("NULL == prGlueInfo\n"));
		free_netdev(prDev);
		return;
	}

#if CFG_ENABLE_BT_OVER_WIFI
	if (prGlueInfo->rBowInfo.fgIsNetRegistered) {
		bowNotifyAllLinkDisconnected(prGlueInfo->prAdapter);
		/* wait 300ms for BoW module to send deauth */
		kalMsleep(300);
	}
#endif

	/* 4 <1> Stopping handling interrupt and free IRQ */
	glBusFreeIrq(prDev, *((P_GLUE_INFO_T *) netdev_priv(prDev)));

	kalMemSet(&(prGlueInfo->prAdapter->rWlanInfo), 0, sizeof(WLAN_INFO_T));

	flush_delayed_work(&workq); /* flush_delayed_work_sync is deprecated */

	down(&g_halt_sem);
	g_u4HaltFlag = 1;

	/* 4 <2> Mark HALT, notify main thread to stop, and clean up queued requests */
	prGlueInfo->u4Flag |= GLUE_FLAG_HALT;
	/* wake up main thread */
	wake_up_interruptible(&prGlueInfo->waitq);
	/* wait main thread stops */
	wait_for_completion_interruptible(&prGlueInfo->rHaltComp);

	DBGLOG(INIT, INFO, ("mtk_sdiod stopped\n"));

	/* prGlueInfo->rHifInfo.main_thread = NULL; */
	prGlueInfo->main_thread = NULL;

#if CFG_ENABLE_BT_OVER_WIFI
	if (prGlueInfo->rBowInfo.fgIsRegistered) {
		glUnregisterAmpc(prGlueInfo);
	}
#endif

	/* 4 <3> Remove /proc filesystem. */
#ifdef WLAN_INCLUDE_PROC
	procRemoveProcfs(prDev, NIC_DEVICE_ID_LOW);
#endif				/* WLAN_INCLUDE_PROC */

	/* 4 <4> wlanAdapterStop */
	prAdapter = prGlueInfo->prAdapter;

	wlanAdapterStop(prAdapter);
	DBGLOG(INIT, INFO, ("Number of Stalled Packets = %ld\n", prGlueInfo->i4TxPendingFrameNum));

#if CFG_ENABLE_WIFI_DIRECT
	prGlueInfo->prAdapter->fgIsWlanLaunched = FALSE;
	if (prGlueInfo->prAdapter->fgIsP2PRegistered) {
		DBGLOG(INIT, INFO, ("p2pNetUnregister...\n"));
		p2pNetUnregister(prGlueInfo, FALSE);
		DBGLOG(INIT, INFO, ("p2pRemove...\n"));
		p2pRemove(prGlueInfo);
	}
#endif

	/* 4 <5> Release the Bus */
	glBusRelease(prDev);

	up(&g_halt_sem);

	/* 4 <6> Unregister the card */
	wlanNetUnregister(prDev->ieee80211_ptr);

	/* 4 <7> Destroy the device */
	wlanNetDestroy(prDev->ieee80211_ptr);
	prDev = NULL;

#if defined(CONFIG_HAS_EARLYSUSPEND)
	glUnregisterEarlySuspend(&mt6620_early_suspend_desc);
#endif
	wlanUnregisterNotifier();

	return;
}				/* end of wlanRemove() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Driver entry point when the driver is configured as a Linux Module, and
*        is called once at module load time, by the user-level modutils
*        application: insmod or modprobe.
*
* \retval 0     Success
*/
/*----------------------------------------------------------------------------*/
/* 1 Module Entry Point */
static int initWlan(void)
{
	int ret = 0, i;

	DBGLOG(INIT, INFO, ("initWlan\n"));

	/* memory pre-allocation */
	kalInitIOBuffer();

	ret = ((glRegisterBus(wlanProbe, wlanRemove) == WLAN_STATUS_SUCCESS) ? 0 : -EIO);

	if (ret == -EIO) {
		kalUninitIOBuffer();
		return ret;
	}

#if (CFG_CHIP_RESET_SUPPORT)
	glResetInit();
#endif

	/* register set_dbg_level handler to mtk_wmt_wifi */
	register_set_dbg_level_handler(set_dbg_level_handler);

	/* Set the initial DEBUG CLASS of each module */
#if DBG
	for (i = 0; i < DBG_MODULE_NUM; i++) {
		aucDebugModule[i] = DBG_CLASS_MASK;	/* enable all */
	}
#else
	/* Initial debug level is D1 */
	for (i = 0; i < DBG_MODULE_NUM; i++) {
		aucDebugModule[i] = DBG_CLASS_ERROR |
		    DBG_CLASS_WARN |
		    DBG_CLASS_STATE | DBG_CLASS_EVENT | DBG_CLASS_TRACE | DBG_CLASS_INFO;
	}
	aucDebugModule[DBG_TX_IDX] &= ~(DBG_CLASS_EVENT | DBG_CLASS_TRACE | DBG_CLASS_INFO);
	aucDebugModule[DBG_RX_IDX] &= ~(DBG_CLASS_EVENT | DBG_CLASS_TRACE | DBG_CLASS_INFO);
	aucDebugModule[DBG_REQ_IDX] &= ~(DBG_CLASS_EVENT | DBG_CLASS_TRACE | DBG_CLASS_INFO);
	aucDebugModule[DBG_INTR_IDX] = 0;
	aucDebugModule[DBG_MEM_IDX] = 0;
#endif				/* DBG */

	return ret;
}				/* end of initWlan() */


/*----------------------------------------------------------------------------*/
/*!
* \brief Driver exit point when the driver as a Linux Module is removed. Called
*        at module unload time, by the user level modutils application: rmmod.
*        This is our last chance to clean up after ourselves.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
/* 1 Module Leave Point */
static VOID exitWlan(void)
{
	DBGLOG(INIT, INFO, ("exitWlan\n"));

	/* unregister set_dbg_level handler to mtk_wmt_wifi */
	register_set_dbg_level_handler(NULL);

#if CFG_CHIP_RESET_SUPPORT
	glResetUninit();
#endif

	glUnregisterBus(wlanRemove);

	/* free pre-allocated memory */
	kalUninitIOBuffer();

	DBGLOG(INIT, INFO, ("exitWlan\n"));

	return;
}				/* end of exitWlan() */


#ifdef MTK_WCN_REMOVE_KERNEL_MODULE
int mtk_wcn_wlan_6620_init(void)
{
	return initWlan();
}

void mtk_wcn_wlan_6620_exit(void)
{
	return exitWlan();
}
EXPORT_SYMBOL(mtk_wcn_wlan_6620_init);
EXPORT_SYMBOL(mtk_wcn_wlan_6620_exit);
#else
module_init(initWlan);
module_exit(exitWlan);
#endif
