/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rt_rf.c

	Abstract:
	Ralink Wireless driver RF related functions

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
*/


#include "rt_config.h"


#ifdef RTMP_RF_RW_SUPPORT
/*
	========================================================================
	
	Routine Description: Write RT30xx RF register through MAC

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NDIS_STATUS RT30xxWriteRFRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			regID,
	IN	UCHAR			value)
{
	RF_CSR_CFG_STRUC	rfcsr = { { 0 } };
	UINT				i = 0;


#ifdef RTMP_MAC_PCI
	if ((pAd->bPCIclkOff == TRUE) || (pAd->LastMCUCmd == SLEEP_MCU_CMD))
	{
		DBGPRINT_ERR(("RT30xxWriteRFRegister. Not allow to write RF 0x%x : fail\n",  regID));	
		return STATUS_UNSUCCESSFUL;
	}
#endif // RTMP_MAC_PCI //

	{
		ASSERT((regID <= pAd->chipCap.MaxNumOfRfId)); // R0~R31 or R63

		do
		{
			RTMP_IO_READ32(pAd, RF_CSR_CFG, &rfcsr.word);

			if (!rfcsr.field.RF_CSR_KICK)
				break;
			i++;
		}
		while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));

		if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
		{
			DBGPRINT_RAW(RT_DEBUG_ERROR, ("Retry count exhausted or device removed!!!\n"));
			return STATUS_UNSUCCESSFUL;
		}

		rfcsr.field.RF_CSR_WR = 1;
		rfcsr.field.RF_CSR_KICK = 1;
		rfcsr.field.TESTCSR_RFACC_REGNUM = regID; // R0~R31
		rfcsr.field.RF_CSR_DATA = value;
		
		RTMP_IO_WRITE32(pAd, RF_CSR_CFG, rfcsr.word);
	}

	return NDIS_STATUS_SUCCESS;
}


/*
	========================================================================
	
	Routine Description: Read RT30xx RF register through MAC

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NDIS_STATUS RT30xxReadRFRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			regID,
	IN	PUCHAR			pValue)
{
	RF_CSR_CFG_STRUC	rfcsr = { { 0 } };
	UINT				i=0, k=0;



#ifdef RTMP_MAC_PCI
	if ((pAd->bPCIclkOff == TRUE) || (pAd->LastMCUCmd == SLEEP_MCU_CMD))
	{
		DBGPRINT_ERR(("RT30xxReadRFRegister. Not allow to read RF 0x%x : fail\n",  regID));	
		return STATUS_UNSUCCESSFUL;
	}
#endif // RTMP_MAC_PCI //

	{
		ASSERT((regID <= pAd->chipCap.MaxNumOfRfId)); // R0~R63

		for (i=0; i<MAX_BUSY_COUNT; i++)
		{
			RTMP_IO_READ32(pAd, RF_CSR_CFG, &rfcsr.word);

			if (rfcsr.field.RF_CSR_KICK == BUSY)									
			{																
				continue;													
			}																
			rfcsr.word = 0;
			rfcsr.field.RF_CSR_WR = 0;
			rfcsr.field.RF_CSR_KICK = 1;
			rfcsr.field.TESTCSR_RFACC_REGNUM = regID;
			RTMP_IO_WRITE32(pAd, RF_CSR_CFG, rfcsr.word);
			for (k=0; k<MAX_BUSY_COUNT; k++)
			{
				RTMP_IO_READ32(pAd, RF_CSR_CFG, &rfcsr.word);

				if (rfcsr.field.RF_CSR_KICK == IDLE)
					break;
			}
			if ((rfcsr.field.RF_CSR_KICK == IDLE) &&
				(rfcsr.field.TESTCSR_RFACC_REGNUM == regID))
			{
				*pValue = (UCHAR)(rfcsr.field.RF_CSR_DATA);
				break;
			}
		}
		if (rfcsr.field.RF_CSR_KICK == BUSY)
		{																	
			DBGPRINT_ERR(("RF read R%d=0x%X fail, i[%d], k[%d]\n", regID, rfcsr.word,i,k));
			return STATUS_UNSUCCESSFUL;
		}
	}

	return STATUS_SUCCESS;
}


VOID NICInitRFRegisters(
	IN RTMP_ADAPTER *pAd)
{
	if (pAd->chipOps.AsicRfInit)
		pAd->chipOps.AsicRfInit(pAd);
}


#endif // RTMP_RF_RW_SUPPORT //

