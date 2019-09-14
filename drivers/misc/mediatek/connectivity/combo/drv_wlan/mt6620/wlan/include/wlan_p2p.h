/*
** $Id: //Department/DaVinci/TRUNK/WiFi_P2P_Driver/include/wlan_p2p.h#3 $
*/

/*! \file   "wlan_p2p.h"
    \brief This file contains the declairations of Wi-Fi Direct command
	   processing routines for MediaTek Inc. 802.11 Wireless LAN Adapters.
*/





#ifndef _WLAN_P2P_H
#define _WLAN_P2P_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#if CFG_ENABLE_WIFI_DIRECT
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/* Service Discovery */
typedef struct _PARAM_P2P_SEND_SD_RESPONSE {
	PARAM_MAC_ADDRESS rReceiverAddr;
	UINT_8 fgNeedTxDoneIndication;
	UINT_8 ucChannelNum;
	UINT_16 u2PacketLength;
	UINT_8 aucPacketContent[0];	/*native 802.11 */
} PARAM_P2P_SEND_SD_RESPONSE, *P_PARAM_P2P_SEND_SD_RESPONSE;

typedef struct _PARAM_P2P_GET_SD_REQUEST {
	PARAM_MAC_ADDRESS rTransmitterAddr;
	UINT_16 u2PacketLength;
	UINT_8 aucPacketContent[0];	/*native 802.11 */
} PARAM_P2P_GET_SD_REQUEST, *P_PARAM_P2P_GET_SD_REQUEST;

typedef struct _PARAM_P2P_GET_SD_REQUEST_EX {
	PARAM_MAC_ADDRESS rTransmitterAddr;
	UINT_16 u2PacketLength;
	UINT_8 ucChannelNum;	/* Channel Number Where SD Request is received. */
	UINT_8 ucSeqNum;	/* Get SD Request by sequence number. */
	UINT_8 aucPacketContent[0];	/*native 802.11 */
} PARAM_P2P_GET_SD_REQUEST_EX, *P_PARAM_P2P_GET_SD_REQUEST_EX;

typedef struct _PARAM_P2P_SEND_SD_REQUEST {
	PARAM_MAC_ADDRESS rReceiverAddr;
	UINT_8 fgNeedTxDoneIndication;
	UINT_8 ucVersionNum;	/* Indicate the Service Discovery Supplicant Version. */
	UINT_16 u2PacketLength;
	UINT_8 aucPacketContent[0];	/*native 802.11 */
} PARAM_P2P_SEND_SD_REQUEST, *P_PARAM_P2P_SEND_SD_REQUEST;

/* Service Discovery 1.0. */
typedef struct _PARAM_P2P_GET_SD_RESPONSE {
	PARAM_MAC_ADDRESS rTransmitterAddr;
	UINT_16 u2PacketLength;
	UINT_8 aucPacketContent[0];	/*native 802.11 */
} PARAM_P2P_GET_SD_RESPONSE, *P_PARAM_P2P_GET_SD_RESPONSE;

/* Service Discovery 2.0. */
typedef struct _PARAM_P2P_GET_SD_RESPONSE_EX {
	PARAM_MAC_ADDRESS rTransmitterAddr;
	UINT_16 u2PacketLength;
	UINT_8 ucSeqNum;	/* Get SD Response by sequence number. */
	UINT_8 aucPacketContent[0];	/*native 802.11 */
} PARAM_P2P_GET_SD_RESPONSE_EX, *P_PARAM_P2P_GET_SD_RESPONSE_EX;


typedef struct _PARAM_P2P_TERMINATE_SD_PHASE {
	PARAM_MAC_ADDRESS rPeerAddr;
} PARAM_P2P_TERMINATE_SD_PHASE, *P_PARAM_P2P_TERMINATE_SD_PHASE;


/*! \brief Key mapping of BSSID */
typedef struct _P2P_PARAM_KEY_T {
	UINT_32 u4Length;	/*!< Length of structure */
	UINT_32 u4KeyIndex;	/*!< KeyID */
	UINT_32 u4KeyLength;	/*!< Key length in bytes */
	PARAM_MAC_ADDRESS arBSSID;	/*!< MAC address */
	PARAM_KEY_RSC rKeyRSC;
	UINT_8 aucKeyMaterial[32];	/*!< Key content by above setting */
} P2P_PARAM_KEY_T, *P_P2P_PARAM_KEY_T;

#if CONFIG_NL80211_TESTMODE

typedef struct _NL80211_DRIVER_TEST_PRE_PARAMS {
	UINT_16 idx_mode;
	UINT_16 idx;
	UINT_32 value;
} NL80211_DRIVER_TEST_PRE_PARAMS, *P_NL80211_DRIVER_TEST_PRE_PARAMS;


typedef struct _NL80211_DRIVER_TEST_PARAMS {
	UINT_32 index;
	UINT_32 buflen;
} NL80211_DRIVER_TEST_PARAMS, *P_NL80211_DRIVER_TEST_PARAMS;


/* P2P Sigma*/
typedef struct _NL80211_DRIVER_P2P_SIGMA_PARAMS {
	NL80211_DRIVER_TEST_PARAMS hdr;
	UINT_32 idx;
	UINT_32 value;
} NL80211_DRIVER_P2P_SIGMA_PARAMS, *P_NL80211_DRIVER_P2P_SIGMA_PARAMS;


/* Hotspot Client Management */
typedef struct _NL80211_DRIVER_HOTSPOT_BLOCK_PARAMS {
	NL80211_DRIVER_TEST_PARAMS hdr;
	UINT_8 ucblocked;
	UINT_8 aucBssid[MAC_ADDR_LEN];
} NL80211_DRIVER_HOTSPOT_BLOCK_PARAMS, *P_NL80211_DRIVER_HOTSPOT_BLOCK_PARAMS;


#if CFG_SUPPORT_WFD
typedef struct _NL80211_DRIVER_WFD_PARAMS {
	NL80211_DRIVER_TEST_PARAMS hdr;
	UINT_32 WfdCmdType;
	UINT_8 WfdEnable;
	UINT_8 WfdCoupleSinkStatus;
	UINT_8 WfdSessionAvailable;
	UINT_8 WfdSigmaMode;
	UINT_16 WfdDevInfo;
	UINT_16 WfdControlPort;
	UINT_16 WfdMaximumTp;
	UINT_16 WfdExtendCap;
	UINT_8 WfdCoupleSinkAddress[MAC_ADDR_LEN];
	UINT_8 WfdAssociatedBssid[MAC_ADDR_LEN];
	UINT_8 WfdVideoIp[4];
	UINT_8 WfdAudioIp[4];
	UINT_16 WfdVideoPort;
	UINT_16 WfdAudioPort;
	UINT_32 WfdFlag;
	UINT_32 WfdPolicy;
	UINT_32 WfdState;
	UINT_8 WfdSessionInformationIE[24 * 8];	/* Include Subelement ID, length */
	UINT_16 WfdSessionInformationIELen;
	UINT_8 aucReserved1[2];
	UINT_8 aucWfdPrimarySinkMac[MAC_ADDR_LEN];
	UINT_8 aucWfdSecondarySinkMac[MAC_ADDR_LEN];
	UINT_32 WfdAdvanceFlag;
	UINT_8 aucWfdLocalIp[4];
	UINT_8 aucReserved2[64];
	UINT_8 aucReserved3[64];
	UINT_8 aucReserved4[64];
} NL80211_DRIVER_WFD_PARAMS, *P_NL80211_DRIVER_WFD_PARAMS;
#endif



#endif


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*--------------------------------------------------------------*/
/* Routines to handle command                                   */
/*--------------------------------------------------------------*/
WLAN_STATUS
wlanoidSetAddP2PKey(IN P_ADAPTER_T prAdapter,
		    IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSetRemoveP2PKey(IN P_ADAPTER_T prAdapter,
		       IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSetNetworkAddress(IN P_ADAPTER_T prAdapter,
			 IN PVOID pvSetBuffer,
			 IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSetP2PMulticastList(IN P_ADAPTER_T prAdapter,
			   IN PVOID pvSetBuffer,
			   IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

/*--------------------------------------------------------------*/
/* Service Discovery Subroutines                                */
/*--------------------------------------------------------------*/
WLAN_STATUS
wlanoidSendP2PSDRequest(IN P_ADAPTER_T prAdapter,
			IN PVOID pvSetBuffer,
			IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSendP2PSDResponse(IN P_ADAPTER_T prAdapter,
			 IN PVOID pvSetBuffer,
			 IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidGetP2PSDRequest(IN P_ADAPTER_T prAdapter,
		       IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidGetP2PSDResponse(IN P_ADAPTER_T prAdapter,
			IN PVOID pvQueryBuffer,
			IN UINT_32 u4QueryBufferLen, OUT PUINT_32 puQueryInfoLen);

WLAN_STATUS
wlanoidSetP2PTerminateSDPhase(IN P_ADAPTER_T prAdapter,
			      IN PVOID pvQueryBuffer,
			      IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);

#if CFG_SUPPORT_ANTI_PIRACY
WLAN_STATUS
wlanoidSetSecCheckRequest(IN P_ADAPTER_T prAdapter,
			  IN PVOID pvSetBuffer,
			  IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidGetSecCheckResponse(IN P_ADAPTER_T prAdapter,
			   IN PVOID pvQueryBuffer,
			   IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);
#endif

WLAN_STATUS
wlanoidSetNoaParam(IN P_ADAPTER_T prAdapter,
		   IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSetOppPsParam(IN P_ADAPTER_T prAdapter,
		     IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSetUApsdParam(IN P_ADAPTER_T prAdapter,
		     IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryP2pPowerSaveProfile(IN P_ADAPTER_T prAdapter,
				IN PVOID pvQueryBuffer,
				IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetP2pPowerSaveProfile(IN P_ADAPTER_T prAdapter,
			      IN PVOID pvSetBuffer,
			      IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSetP2pSetNetworkAddress(IN P_ADAPTER_T prAdapter,
			       IN PVOID pvSetBuffer,
			       IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidQueryP2pOpChannel(IN P_ADAPTER_T prAdapter,
			 IN PVOID pvQueryBuffer,
			 IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);

WLAN_STATUS
wlanoidQueryP2pVersion(IN P_ADAPTER_T prAdapter,
		       IN PVOID pvQueryBuffer,
		       IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);

WLAN_STATUS
wlanoidSetP2pSupplicantVersion(IN P_ADAPTER_T prAdapter,
			       IN PVOID pvSetBuffer,
			       IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

WLAN_STATUS
wlanoidSetP2pWPSmode(IN P_ADAPTER_T prAdapter,
		     IN PVOID pvQueryBuffer,
		     IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);

#if CFG_SUPPORT_P2P_RSSI_QUERY
WLAN_STATUS
wlanoidQueryP2pRssi(IN P_ADAPTER_T prAdapter,
		    IN PVOID pvQueryBuffer,
		    IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);
#endif


/*--------------------------------------------------------------*/
/* Callbacks for event indication                               */
/*--------------------------------------------------------------*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif
#endif				/* _WLAN_P2P_H */
