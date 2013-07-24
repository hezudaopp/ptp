/**
 * @file   msg.c
 * @author George Neville-Neil <gnn@neville-neil.com>
 * @date   Tue Jul 20 16:17:05 2010
 * 
 * @brief  Functions to pack and unpack messages.
 * 
 * See spec annex d
 */

#include "../ptpd.h"

/*Unpack Header from IN buffer to msgTmpHeader field */
void 
msgUnpackHeader(void *buf, MsgHeader * header)
{
	header->transportSpecific = (*(Nibble *) (buf + 0)) >> 4;
	header->messageType = (*(Enumeration4 *) (buf + 0)) & 0x0F;
	header->versionPTP = (*(UInteger4 *) (buf + 1)) & 0x0F;
	/* force reserved bit to zero if not */
	header->messageLength = flip16(*(UInteger16 *) (buf + 2));
	header->domainNumber = (*(UInteger8 *) (buf + 4));
	memcpy(header->flagField, (buf + 6), FLAG_FIELD_LENGTH);
	memcpy(&header->correctionfield.msb, (buf + 8), 4);
	memcpy(&header->correctionfield.lsb, (buf + 12), 4);
	header->correctionfield.msb = flip32(header->correctionfield.msb);
	header->correctionfield.lsb = flip32(header->correctionfield.lsb);
	memcpy(header->sourcePortIdentity.clockIdentity, (buf + 20), 
	       CLOCK_IDENTITY_LENGTH);
	header->sourcePortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 28));
	header->sequenceId = flip16(*(UInteger16 *) (buf + 30));
	header->controlField = (*(UInteger8 *) (buf + 32));
	header->logMessageInterval = (*(Integer8 *) (buf + 33));

#ifdef PTPD_DBG
	msgHeader_display(header);
#endif /* PTPD_DBG */
}
void
msgDisplayBuf(void *buf,UInteger8 len)
{
	DBGV("buf is: \n");
	int row = len / 16;
	int i = 0;
	int j = 0;
	for(i=0; i< row; i++)
	{
		fprintf(stderr,"%.2x :",i);
		for(j = 0; j < 16; j++)
		{
			fprintf(stderr,"%.2x ",*(UInteger8 *) (buf + i*16 + j));
		}
		fprintf(stderr,"\n");
	}
	int remain = len % 16;
	fprintf(stderr,"%.2x :", row);
	for(i = 0; i < remain; i++)
	{
		fprintf(stderr,"%.2x ",*(UInteger8 *) (buf + row*16 + i));
	}
	fprintf(stderr,"\n");
}
/*Pack header message into OUT buffer of ptpClock*/
void 
msgPackHeader(void *buf, PtpClock * ptpClock)
{
	//CHANG this to zero because no need anymore???
	Nibble transport = 0x00;//0x80;

	/* (spec annex D) */
	*(UInteger8 *) (buf + 0) = transport;
	*(UInteger4 *) (buf + 1) = ptpClock->versionNumber;
	*(UInteger8 *) (buf + 4) = ptpClock->domainNumber;

/* CHANGE set flag in different pack message
  	if (ptpClock->twoStepFlag)
		*(UInteger8 *) (buf + 6) = TWO_STEP_FLAG;
*/
	*(UInteger8 *) (buf + 6) = 0x00;

	memset((buf + 8), 0, 8);
	memcpy((buf + 20), ptpClock->portIdentity.clockIdentity, 
	       CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 28) = flip16(ptpClock->portIdentity.portNumber);
	*(UInteger8 *) (buf + 33) = 0x7F;
	/* Default value(spec Table 24) */
}



/*Pack SYNC message into OUT buffer of ptpClock*/
void 
msgPackSync(void *buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	/* changes in header，buf[0]后4个bit置0 */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType，这个计算感觉没有必要 */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x00;
	/*CHANGE set flag here*/
  	if (ptpClock->twoStepFlag)
		*(UInteger8 *) (buf + 6) = TWO_STEP_FLAG;
  	else
  		/*CHANGE SET FLAG TO 0X00*/
  		*(UInteger8 *) (buf + 6) = 0x00;
  	/*CHANG ENDS*/
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(SYNC_LENGTH);
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentSyncSequenceId);
	*(UInteger8 *) (buf + 32) = 0x00;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logSyncInterval;
	memset((buf + 8), 0, 8);

	/* Sync message */
	*(UInteger16 *) (buf + 34) = flip16(originTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(originTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(originTimestamp->nanosecondsField);
	/*CHANGE add 2 extra bytes here*/
//	*(UInteger16 *) (buf + 44) = 0x0000;
	/*CHANGE ENDS*/
}

/*Unpack Sync message from IN buffer */
/*CHANGE T2 comes from reserved parts in sync message*/
void 
msgUnpackSync(void *buf, MsgSync * sync, TimeInternal * sync_receive_time)
{
	sync->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	sync->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	sync->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	/*get sync_receive_time from reserved */
	sync_receive_time->seconds += (*(UInteger8 *) (buf + 5));
	sync_receive_time->nanoseconds =
			flip32(*(UInteger32 *) (buf + 16)) & 0x7fffffff; //ignore the 31bit
#ifdef PTPD_DBG
	msgSync_display(sync, sync_receive_time);
#endif /* PTPD_DBG */
}
/*CHANGE ENDS*/


/*Pack Announce message into OUT buffer of ptpClock*/
void 
msgPackAnnounce(void *buf, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0B;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(ANNOUNCE_LENGTH);
	/*CHANGE SET FLAG TO 0X00*/
	*(UInteger8 *) (buf + 6) = 0x00;

	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentAnnounceSequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logAnnounceInterval;

	/* Announce message */
	memset((buf + 34), 0, 10);
	*(Integer16 *) (buf + 44) = flip16(ptpClock->currentUtcOffset);
	*(UInteger8 *) (buf + 47) = ptpClock->grandmasterPriority1;
	*(UInteger8 *) (buf + 48) = ptpClock->clockQuality.clockClass;
	*(Enumeration8 *) (buf + 49) = ptpClock->clockQuality.clockAccuracy;
	*(UInteger16 *) (buf + 50) = 
		flip16(ptpClock->clockQuality.offsetScaledLogVariance);
	*(UInteger8 *) (buf + 52) = ptpClock->grandmasterPriority2;
	memcpy((buf + 53), ptpClock->grandmasterIdentity, CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 61) = flip16(ptpClock->stepsRemoved);
	*(Enumeration8 *) (buf + 63) = ptpClock->timeSource;
}
/*CHANGE package flag*/
void
msgPackFlag(void *buf, PtpClock *ptpClock)
{
	*(UInteger16 *) (buf + 6) = *(UInteger16 *)(buf + 6) | flip16(0x003c);
}
/*Unpack Announce message from IN buffer of ptpClock to msgtmp.Announce*/
void 
msgUnpackAnnounce(void *buf, MsgAnnounce * announce)
{
	announce->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	announce->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	announce->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	announce->currentUtcOffset = flip16(*(UInteger16 *) (buf + 44));
	announce->grandmasterPriority1 = *(UInteger8 *) (buf + 47);
	announce->grandmasterClockQuality.clockClass = 
		*(UInteger8 *) (buf + 48);
	announce->grandmasterClockQuality.clockAccuracy = 
		*(Enumeration8 *) (buf + 49);
	announce->grandmasterClockQuality.offsetScaledLogVariance = 
		flip16(*(UInteger16 *) (buf + 50));
	announce->grandmasterPriority2 = *(UInteger8 *) (buf + 52);
	memcpy(announce->grandmasterIdentity, (buf + 53), 
	       CLOCK_IDENTITY_LENGTH);
	announce->stepsRemoved = flip16(*(UInteger16 *) (buf + 61));
	announce->timeSource = *(Enumeration8 *) (buf + 63);
	
#ifdef PTPD_DBG
	msgAnnounce_display(announce);
#endif /* PTPD_DBG */
}

/*pack Follow_up message into OUT buffer of ptpClock*/
void 
msgPackFollowUp(void *buf, Timestamp * preciseOriginTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x08;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(FOLLOW_UP_LENGTH);
	/*CHANGE SET FLAG TO 0X00*/
	*(UInteger8 *) (buf + 6) = 0x00;
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentSyncSequenceId - 1);
	/* sentSyncSequenceId has already been incremented in "issueSync" */
	*(UInteger8 *) (buf + 32) = 0x02;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logSyncInterval;

	/* Follow_up message */
	*(UInteger16 *) (buf + 34) = 
		flip16(preciseOriginTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = 
		flip32(preciseOriginTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = 
		flip32(preciseOriginTimestamp->nanosecondsField);
}

/*Unpack Follow_up message from IN buffer of ptpClock to msgtmp.follow*/
void 
msgUnpackFollowUp(void *buf, MsgFollowUp * follow)
{
	follow->preciseOriginTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	follow->preciseOriginTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	follow->preciseOriginTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));

#ifdef PTPD_DBG
	msgFollowUp_display(follow);
#endif /* PTPD_DBG */

}


/*pack PdelayReq message into OUT buffer of ptpClock*/
void 
msgPackPDelayReq(void *buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x02;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(PDELAY_REQ_LENGTH);
	/*CHANGE SET FLAG TO 0X00*/
	*(UInteger8 *) (buf + 6) = 0x00;
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentPDelayReqSequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */
	memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = flip16(originTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(originTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(originTimestamp->nanosecondsField);
	memset((buf + 44), 0, 10);
	/* RAZ reserved octets */
}

/*pack delayReq message into OUT buffer of ptpClock*/
void 
msgPackDelayReq(void *buf, Timestamp * originTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x01;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(DELAY_REQ_LENGTH);
	/*CHANGE SET FLAG TO 0X00*/
	*(UInteger8 *) (buf + 6) = 0x00;
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->sentDelayReqSequenceId);
	*(UInteger8 *) (buf + 32) = 0x01;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */
	memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = flip16(originTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(originTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(originTimestamp->nanosecondsField);
}

/*pack delayResp message into OUT buffer of ptpClock*/
void 
msgPackDelayResp(void *buf, MsgHeader * header, Timestamp * receiveTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x09;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(DELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = header->domainNumber;
	/*CHANGE SET FLAG TO 0X00*/
	*(UInteger8 *) (buf + 6) = 0x00;
	memset((buf + 8), 0, 8);
	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = flip32(header->correctionfield.msb);
	*(Integer32 *) (buf + 12) = flip32(header->correctionfield.lsb);

	*(UInteger16 *) (buf + 30) = flip16(header->sequenceId);
	
	*(UInteger8 *) (buf + 32) = 0x03;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = ptpClock->logMinDelayReqInterval;
	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) = 
		flip16(receiveTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(receiveTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(receiveTimestamp->nanosecondsField);
	memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, 
	       CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) = 
		flip16(header->sourcePortIdentity.portNumber);
}





/*pack PdelayResp message into OUT buffer of ptpClock*/
void 
msgPackPDelayResp(void *buf, MsgHeader * header, Timestamp * requestReceiptTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x03;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(PDELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = header->domainNumber;
	memset((buf + 8), 0, 8);
	/*CHANGE set flag here*/
  	if (ptpClock->twoStepFlag)
		*(UInteger8 *) (buf + 6) = TWO_STEP_FLAG;
  	else
		*(UInteger8 *) (buf + 6) = 0x00;
  	/*CHANGE ENDS*/

	*(UInteger16 *) (buf + 30) = flip16(header->sequenceId);

	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) = flip16(requestReceiptTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = flip32(requestReceiptTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = flip32(requestReceiptTimestamp->nanosecondsField);
	memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) = flip16(header->sourcePortIdentity.portNumber);

}

//CHANGE ugly fix of the problem that can't get 32bit int correctly
UInteger32
get32(void *buf)
{
	UInteger16 up,low;
	UInteger32 temp;
	up = flip16(*(UInteger16 *) (buf));
	low = flip16(*(UInteger16 *) (buf + 2));
	temp =((UInteger32)up << 16) + (UInteger32)low;
	return temp;
}
/*Unpack delayReq message from IN buffer of ptpClock to msgtmp.req*/
void 
msgUnpackDelayReq(void *buf, MsgDelayReq * delayreq)
{
	delayreq->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	delayreq->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	delayreq->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40)) & 0x7fffffff;
	//CHANGE here we go the ugly fix
	delayreq->originTimestamp.secondsField.lsb = get32(buf + 36);
	delayreq->originTimestamp.nanosecondsField = get32(buf + 40) & 0x7fffffff;
	//CHANGE ENDS

#ifdef PTPD_DBG
	msgDelayReq_display(delayreq);
#endif /* PTPD_DBG */

}


/*Unpack PdelayReq message from IN buffer of ptpClock to msgtmp.req*/
void 
msgUnpackPDelayReq(void *buf, MsgPDelayReq * pdelayreq)
{
	/*CHANGE the position of timestamp is somehow move forward 2 ocetets*/
	pdelayreq->originTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	pdelayreq->originTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	pdelayreq->originTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40))& 0x7fffffff;
	//CHANGE here we go the ugly fix
	pdelayreq->originTimestamp.secondsField.lsb = get32(buf + 36);
	pdelayreq->originTimestamp.nanosecondsField = get32(buf + 40) & 0x7fffffff;
	//CHANGE ENDS

#ifdef PTPD_DBG
	//print all the buf!
//	DBGV("head of the buff is :\n");
//	DBGV("0x%.4x\n",flip16(*(UInteger16 *) (buf)));
	DBGV("2 bytes buf+36 is: 0x%.4x\n",flip16(*(UInteger16 *) (buf + 36)));
	DBGV("2 bytes buf+38 is: 0x%.4x\n",flip16(*(UInteger16 *) (buf + 38)));
	DBGV("4 bytes buf+38 is: 0x%.8x\n",flip32(*(UInteger32 *) (buf + 36)));
	DBGV("2 bytes buf+40 is: 0x%.4x\n",flip16(*(UInteger16 *) (buf + 40)));
	DBGV("2 bytes buf+42 is: 0x%.4x\n",flip16(*(UInteger16 *) (buf + 42)));
	DBGV("4 bytes buf+40 is: 0x%.8x\n",flip32(*(UInteger32 *) (buf + 40)));
	DBGV("reserved field is:\n");
	Timestamp ts;
	ts.secondsField.msb = flip16(*(UInteger16 *) (buf + 44));
	ts.secondsField.lsb = flip32(*(UInteger32 *) (buf + 46));
	ts.nanosecondsField = flip32(*(UInteger32 *) (buf + 50));
	timestamp_display(&ts);
#endif /* PTPD_DBG */

#ifdef PTPD_DBG
	DBGV("PDelayReq received time is: \n");
	msgPDelayReq_display(pdelayreq);

#endif /* PTPD_DBG */

}


/*Unpack delayResp message from IN buffer of ptpClock to msgtmp.presp*/
void 
msgUnpackDelayResp(void *buf, MsgDelayResp * resp, TimeInternal * req_send_time)
{
	resp->receiveTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	resp->receiveTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	resp->receiveTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40)) & 0x7fffffff;
	/*CHANGE get req_send_time from reserved */
	req_send_time->seconds += (*(UInteger8 *) (buf + 5));
	req_send_time->nanoseconds =
			flip32(*(UInteger32 *) (buf + 16)) & 0x7fffffff;
	memcpy(resp->requestingPortIdentity.clockIdentity, 
	       (buf + 44), CLOCK_IDENTITY_LENGTH);
	resp->requestingPortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 52));

#ifdef PTPD_DBG
	msgDelayResp_display(resp, req_send_time);
#endif /* PTPD_DBG */
}


/*Unpack PdelayResp message from IN buffer of ptpClock to msgtmp.presp*/
void 
msgUnpackPDelayResp(void *buf, MsgPDelayResp * presp)
{
	presp->requestReceiptTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	presp->requestReceiptTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	presp->requestReceiptTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	memcpy(presp->requestingPortIdentity.clockIdentity, 
	       (buf + 44), CLOCK_IDENTITY_LENGTH);
	presp->requestingPortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 52));

#ifdef PTPD_DBG
	msgPDelayResp_display(presp);
#endif /* PTPD_DBG */
}

/*pack PdelayRespfollowup message into OUT buffer of ptpClock*/
void 
msgPackPDelayRespFollowUp(void *buf, MsgHeader * header, Timestamp * responseOriginTimestamp, PtpClock * ptpClock)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0A;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = flip16(PDELAY_RESP_FOLLOW_UP_LENGTH);
	/*CHANGE SET FLAG TO 0X00*/
	*(UInteger8 *) (buf + 6) = 0x00;
	*(UInteger16 *) (buf + 30) = flip16(ptpClock->PdelayReqHeader.sequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */

	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = flip32(header->correctionfield.msb);
	*(Integer32 *) (buf + 12) = flip32(header->correctionfield.lsb);

	/* Pdelay_resp_follow_up message */
	*(UInteger16 *) (buf + 34) = 
		flip16(responseOriginTimestamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = 
		flip32(responseOriginTimestamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = 
		flip32(responseOriginTimestamp->nanosecondsField);
	memcpy((buf + 44), header->sourcePortIdentity.clockIdentity, 
	       CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) = 
		flip16(header->sourcePortIdentity.portNumber);
}

/*Unpack PdelayResp message from IN buffer of ptpClock to msgtmp.presp*/
void 
msgUnpackPDelayRespFollowUp(void *buf, MsgPDelayRespFollowUp * prespfollow)
{
	prespfollow->responseOriginTimestamp.secondsField.msb = 
		flip16(*(UInteger16 *) (buf + 34));
	prespfollow->responseOriginTimestamp.secondsField.lsb = 
		flip32(*(UInteger32 *) (buf + 36));
	prespfollow->responseOriginTimestamp.nanosecondsField = 
		flip32(*(UInteger32 *) (buf + 40));
	memcpy(prespfollow->requestingPortIdentity.clockIdentity, 
	       (buf + 44), CLOCK_IDENTITY_LENGTH);
	prespfollow->requestingPortIdentity.portNumber = 
		flip16(*(UInteger16 *) (buf + 52));
}


/** 
 * Dump the most recent packet in the daemon
 * 
 * @param ptpClock The central clock structure
 */
void msgDump(PtpClock *ptpClock)
{

#if defined(freebsd)
	static int dumped = 0;
#endif /* FreeBSD */

	msgDebugHeader(&ptpClock->msgTmpHeader);
	switch (ptpClock->msgTmpHeader.messageType) {
	case SYNC:
		msgDebugSync(&ptpClock->msgTmp.sync);
		break;
    
	case ANNOUNCE:
		msgDebugAnnounce(&ptpClock->msgTmp.announce);
		break;
    
	case FOLLOW_UP:
		msgDebugFollowUp(&ptpClock->msgTmp.follow);
		break;
    
	case DELAY_REQ:
		msgDebugDelayReq(&ptpClock->msgTmp.req);
		break;
    
	case DELAY_RESP:
		msgDebugDelayResp(&ptpClock->msgTmp.resp);
		break;
    
	case MANAGEMENT:
		msgDebugManagement(&ptpClock->msgTmp.manage);
		break;
    
	default:
		NOTIFY("msgDump:unrecognized message\n");
		break;
	}

#if defined(freebsd)
	/* Only dump the first time, after that just do a message. */
	if (dumped != 0) 
		return;

	dumped++;
	NOTIFY("msgDump: core file created.\n");    

	switch(rfork(RFFDG|RFPROC|RFNOWAIT)) {
	case -1:
		NOTIFY("could not fork to core dump! errno: %s", 
		       strerror(errno));
		break;
	case 0:
		abort(); /* Generate a core dump */
	default:
		/* This default intentionally left blank. */
		break;
	}
#endif /* FreeBSD */
}

/** 
 * Dump a PTP message header
 * 
 * @param header a pre-filled msg header structure
 */

void msgDebugHeader(MsgHeader *header)
{
	NOTIFY("msgDebugHeader: messageType %d\n", header->messageType);
	NOTIFY("msgDebugHeader: versionPTP %d\n", header->versionPTP);
	NOTIFY("msgDebugHeader: messageLength %d\n", header->messageLength);
	NOTIFY("msgDebugHeader: domainNumber %d\n", header->domainNumber);
	NOTIFY("msgDebugHeader: flags %02hhx %02hhx\n", 
	       header->flagField[0], header->flagField[1]);
	NOTIFY("msgDebugHeader: correctionfield %d\n", header->correctionfield);
	NOTIFY("msgDebugHeader: sourcePortIdentity.clockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%02hhx:%02hhx\n",
	       header->sourcePortIdentity.clockIdentity[0], 
	       header->sourcePortIdentity.clockIdentity[1], 
	       header->sourcePortIdentity.clockIdentity[2], 
	       header->sourcePortIdentity.clockIdentity[3], 
	       header->sourcePortIdentity.clockIdentity[4], 
	       header->sourcePortIdentity.clockIdentity[5], 
	       header->sourcePortIdentity.clockIdentity[6], 
	       header->sourcePortIdentity.clockIdentity[7]);
	NOTIFY("msgDebugHeader: sourcePortIdentity.portNumber %d\n",
	       header->sourcePortIdentity.portNumber);
	NOTIFY("msgDebugHeader: sequenceId %d\n", header->sequenceId);
	NOTIFY("msgDebugHeader: controlField %d\n", header->controlField);
	NOTIFY("msgDebugHeader: logMessageIntervale %d\n", 
	       header->logMessageInterval);

}

/** 
 * Dump the contents of a sync packet
 * 
 * @param sync A pre-filled MsgSync structure
 */

void msgDebugSync(MsgSync *sync)
{
	NOTIFY("msgDebugSync: originTimestamp.seconds %u\n",
	       sync->originTimestamp.secondsField);
	NOTIFY("msgDebugSync: originTimestamp.nanoseconds %d\n",
	       sync->originTimestamp.nanosecondsField);
}

/** 
 * Dump the contents of a announce packet
 * 
 * @param sync A pre-filled MsgAnnounce structure
 */

void msgDebugAnnounce(MsgAnnounce *announce)
{
	NOTIFY("msgDebugAnnounce: originTimestamp.seconds %u\n",
	       announce->originTimestamp.secondsField);
	NOTIFY("msgDebugAnnounce: originTimestamp.nanoseconds %d\n",
	       announce->originTimestamp.nanosecondsField);
	NOTIFY("msgDebugAnnounce: currentUTCOffset %d\n", 
	       announce->currentUtcOffset);
	NOTIFY("msgDebugAnnounce: grandmasterPriority1 %d\n", 
	       announce->grandmasterPriority1);
	NOTIFY("msgDebugAnnounce: grandmasterClockQuality.clockClass %d\n",
	       announce->grandmasterClockQuality.clockClass);
	NOTIFY("msgDebugAnnounce: grandmasterClockQuality.clockAccuracy %d\n",
	       announce->grandmasterClockQuality.clockAccuracy);
	NOTIFY("msgDebugAnnounce: "
	       "grandmasterClockQuality.offsetScaledLogVariance %d\n",
	       announce->grandmasterClockQuality.offsetScaledLogVariance);
	NOTIFY("msgDebugAnnounce: grandmasterPriority2 %d\n", 
	       announce->grandmasterPriority2);
	NOTIFY("msgDebugAnnounce: grandmasterClockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%02hhx:%02hhx\n",
	       announce->grandmasterIdentity[0], 
	       announce->grandmasterIdentity[1], 
	       announce->grandmasterIdentity[2], 
	       announce->grandmasterIdentity[3], 
	       announce->grandmasterIdentity[4], 
	       announce->grandmasterIdentity[5], 
	       announce->grandmasterIdentity[6], 
	       announce->grandmasterIdentity[7]);
	NOTIFY("msgDebugAnnounce: stepsRemoved %d\n", 
	       announce->stepsRemoved);
	NOTIFY("msgDebugAnnounce: timeSource %d\n", 
	       announce->timeSource);
}

/** 
 * NOT IMPLEMENTED
 * 
 * @param req 
 */
void msgDebugDelayReq(MsgDelayReq *req) {}

/** 
 * Dump the contents of a followup packet
 * 
 * @param follow A pre-fille MsgFollowUp structure
 */
void msgDebugFollowUp(MsgFollowUp *follow)
{
	NOTIFY("msgDebugFollowUp: preciseOriginTimestamp.seconds %u\n",
	       follow->preciseOriginTimestamp.secondsField);
	NOTIFY("msgDebugFollowUp: preciseOriginTimestamp.nanoseconds %d\n",
	       follow->preciseOriginTimestamp.nanosecondsField);
}

/** 
 * Dump the contents of a delay response packet
 * 
 * @param resp a pre-filled MsgDelayResp structure
 */
void msgDebugDelayResp(MsgDelayResp *resp)
{
	NOTIFY("msgDebugDelayResp: delayReceiptTimestamp.seconds %u\n",
	       resp->receiveTimestamp.secondsField);
	NOTIFY("msgDebugDelayResp: delayReceiptTimestamp.nanoseconds %d\n",
	       resp->receiveTimestamp.nanosecondsField);
	NOTIFY("msgDebugDelayResp: requestingPortIdentity.clockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%02hhx:%02hhx\n",
	       resp->requestingPortIdentity.clockIdentity[0], 
	       resp->requestingPortIdentity.clockIdentity[1], 
	       resp->requestingPortIdentity.clockIdentity[2], 
	       resp->requestingPortIdentity.clockIdentity[3], 
	       resp->requestingPortIdentity.clockIdentity[4], 
	       resp->requestingPortIdentity.clockIdentity[5], 
	       resp->requestingPortIdentity.clockIdentity[6], 
	       resp->requestingPortIdentity.clockIdentity[7]);
	NOTIFY("msgDebugDelayResp: requestingPortIdentity.portNumber %d\n",
	       resp->requestingPortIdentity.portNumber);
}

/** 
 * Dump the contents of management packet
 * 
 * @param manage a pre-filled MsgManagement structure
 */

void msgDebugManagement(MsgManagement *manage)
{
	NOTIFY("msgDebugDelayManage: targetPortIdentity.clockIdentity "
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%02hhx:%02hhx\n",
	       manage->targetPortIdentity.clockIdentity[0], 
	       manage->targetPortIdentity.clockIdentity[1], 
	       manage->targetPortIdentity.clockIdentity[2], 
	       manage->targetPortIdentity.clockIdentity[3], 
	       manage->targetPortIdentity.clockIdentity[4], 
	       manage->targetPortIdentity.clockIdentity[5], 
	       manage->targetPortIdentity.clockIdentity[6], 
	       manage->targetPortIdentity.clockIdentity[7]);
	NOTIFY("msgDebugDelayManage: targetPortIdentity.portNumber %d\n",
	       manage->targetPortIdentity.portNumber);
	NOTIFY("msgDebugManagement: startingBoundaryHops %d\n",
	       manage->startingBoundaryHops);
	NOTIFY("msgDebugManagement: boundaryHops %d\n", manage->boundaryHops);
	NOTIFY("msgDebugManagement: actionField %d\n", manage->actionField);
	NOTIFY("msgDebugManagement: tvl %s\n", manage->tlv);
}
