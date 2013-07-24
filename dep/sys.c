/**
 * @file   sys.c
 * @date   Tue Jul 20 16:19:46 2010
 * 
 * @brief  Code to call kernel time routines and also display server statistics.
 * 
 * 
 */

#include "../ptpd.h"


int 
isTimeInternalNegative(const TimeInternal * p)
{
	return (p->seconds < 0) || (p->nanoseconds < 0);
}


int 
snprint_TimeInternal(char *s, int max_len, const TimeInternal * p)
{
	int len = 0;

	if (isTimeInternalNegative(p))
		len += snprintf(&s[len], max_len - len, "-");

	len += snprintf(&s[len], max_len - len, "%d.%09d",
	    abs(p->seconds), abs(p->nanoseconds));

	return len;
}


int 
snprint_ClockIdentity(char *s, int max_len, const ClockIdentity id, const char *info)
{
	int len = 0;
	int i;

	if (info)
		len += snprintf(&s[len], max_len - len, "%s", info);

	for (i = 0; ;) {
		len += snprintf(&s[len], max_len - len, "%02x", (unsigned char) id[i]);

		if (++i >= CLOCK_IDENTITY_LENGTH)
			break;

		// uncomment the line below to print a separator after each byte except the last one
		// len += snprintf(&s[len], max_len - len, "%s", "-");
	}

	return len;
}


int 
snprint_PortIdentity(char *s, int max_len, const PortIdentity *id, const char *info)
{
	int len = 0;

	if (info)
		len += snprintf(&s[len], max_len - len, "%s", info);

	len += snprint_ClockIdentity(&s[len], max_len - len, id->clockIdentity, NULL);
	len += snprintf(&s[len], max_len - len, "/%02x", (unsigned) id->portNumber);
	return len;
}


void
message(int priority, const char * format, ...)
{
	extern RunTimeOpts rtOpts;
	va_list ap;
	va_start(ap, format);
	if(rtOpts.useSysLog) {
		static Boolean logOpened;
		if(!logOpened) {
			openlog("ptpd", 0, LOG_USER);
			logOpened = TRUE;
		}
		vsyslog(priority, format, ap);
	} else {
		fprintf(stderr, "(ptpd %s) ",
			priority == LOG_EMERG ? "emergency" :
			priority == LOG_ALERT ? "alert" :
			priority == LOG_CRIT ? "critical" :
			priority == LOG_ERR ? "error" :
			priority == LOG_WARNING ? "warning" :
			priority == LOG_NOTICE ? "notice" :
			priority == LOG_INFO ? "info" :
			priority == LOG_DEBUG ? "debug" :
			"???");
		vfprintf(stderr, format, ap);
	}
	va_end(ap);
}

char *
translatePortState(PtpClock *ptpClock)
{
	char *s;
	switch(ptpClock->portState) {
	case PTP_INITIALIZING:  s = "init";  break;
	case PTP_FAULTY:        s = "flt";   break;
	case PTP_LISTENING:     s = "lstn";  break;
	case PTP_PASSIVE:       s = "pass";  break;
	case PTP_UNCALIBRATED:  s = "uncl";  break;
	case PTP_SLAVE:         s = "slv";   break;
	case PTP_PRE_MASTER:    s = "pmst";  break;
	case PTP_MASTER:        s = "mst";   break;
	case PTP_DISABLED:      s = "dsbl";  break;
	default:                s = "?";     break;
	}
	return s;
}

void 
displayStats(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	static int start = 1;
	static char sbuf[SCREEN_BUFSZ];
	int len = 0;
	struct timeval now;
	char time_str[MAXTIMESTR];

	if (start && rtOpts->csvStats) {
		start = 0;
		printf("timestamp, state, clock ID, one way delay, offset from master, "
		       "slave to master, master to slave, drift, variance");
		fflush(stdout);
	}
	memset(sbuf, ' ', sizeof(sbuf));

	gettimeofday(&now, 0);
	strftime(time_str, MAXTIMESTR, "%Y-%m-%d %X", localtime(&now.tv_sec));

	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s%s:%06d, %s",
		       rtOpts->csvStats ? "\n" : "\rstate: ",
		       time_str, (int)now.tv_usec,
		       translatePortState(ptpClock));

	if (ptpClock->portState == PTP_SLAVE) {
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			 &ptpClock->parentPortIdentity, ", ");

		/* 
		 * if grandmaster ID differs from parent port ID then
		 * also print GM ID 
		 */
		if (memcmp(ptpClock->grandmasterIdentity, 
			   ptpClock->parentPortIdentity.clockIdentity, 
			   CLOCK_IDENTITY_LENGTH)) {
			len += snprint_ClockIdentity(sbuf + len, 
						     sizeof(sbuf) - len,
						     ptpClock->grandmasterIdentity, 
						     " GM:");
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if (!rtOpts->csvStats)
			len += snprintf(sbuf + len, 
					sizeof(sbuf) - len, "owd: ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->meanPathDelay);

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if (!rtOpts->csvStats)
			len += snprintf(sbuf + len, sizeof(sbuf) - len, 
					"ofm: ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->offsetFromMaster);

		len += sprintf(sbuf + len,
			       ", %s%d.%09d" ", %s%d.%09d",
			       rtOpts->csvStats ? "" : "stm: ",
			       ptpClock->delaySM.seconds,
			       abs(ptpClock->delaySM.nanoseconds),
			       rtOpts->csvStats ? "" : "mts: ",
			       ptpClock->delayMS.seconds,
			       abs(ptpClock->delayMS.nanoseconds));
		
		len += sprintf(sbuf + len, ", %s%d",
		    rtOpts->csvStats ? "" : "drift: ", 
			       ptpClock->observed_drift);
	}
	else {
		if (ptpClock->portState == PTP_MASTER) {
			len += snprint_ClockIdentity(sbuf + len, 
						     sizeof(sbuf) - len,
						     ptpClock->clockIdentity, 
						     " (ID:");
			len += snprintf(sbuf + len, sizeof(sbuf) - len, ")");
		}
	}
	write(1, sbuf, rtOpts->csvStats ? len : SCREEN_MAXSZ + 1);
}

Boolean 
nanoSleep(TimeInternal * t)
{
	struct timespec ts, tr;

	ts.tv_sec = t->seconds;
	ts.tv_nsec = t->nanoseconds;

	if (nanosleep(&ts, &tr) < 0) {
		t->seconds = tr.tv_sec;
		t->nanoseconds = tr.tv_nsec;
		return FALSE;
	}
	return TRUE;
}

void 
getTime(TimeInternal * time)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		PERROR("clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;

}

void 
setTime(TimeInternal * time)
{
	struct timeval tv;
 
	tv.tv_sec = time->seconds;
	tv.tv_usec = time->nanoseconds / 1000;
	settimeofday(&tv, 0);
	NOTIFY("resetting system clock to %ds %dns\n",
	       time->seconds, time->nanoseconds);
}

/*CHANGE add set driver time function*/
void
setDriverTime(TimeInternal * time)
{
    int dev;
    dev=open(DEVICE_FILENAME,O_RDWR|O_NDELAY);
    if(dev < 0) ERROR("cant open driver in setDriverTime");
    ioctl(dev,SETTIME, time);
    close(dev);
	NOTIFY("resetting driver clock to %ds %dns\n",
	       time->seconds, time->nanoseconds);
}
/*CHANGE ENDS*/
/*CHANGE add adjust driver time function*/
void
adjDriverTime(TimeInternal * timeoffset)
{
    int dev;
    dev=open(DEVICE_FILENAME,O_RDWR|O_NDELAY);
    if(dev < 0) ERROR("cant open driver in adjDriverTime");
    ioctl(dev,ADJTIME, timeoffset);
    close(dev);
	NOTIFY("Adjusting driver clock by %ds %dns\n",
	       timeoffset->seconds, timeoffset->nanoseconds);
}
/*CHANGE ENDS*/
/*CHANGE add temporary frequency adjustment function*/
void
tmpAdjDriverFreq(TmpRate * tmpRate)
{
    int dev;
    dev=open(DEVICE_FILENAME,O_RDWR|O_NDELAY);
    if(dev < 0) ERROR("cant open driver in tmpAdjDriverTime");
    ioctl(dev,TMPADJFREQ, tmpRate);
    close(dev);
	NOTIFY("Adjusting temporary frequency by %ds %dns\n",
	       tmpRate->offset.seconds, tmpRate->offset.nanoseconds);
	NOTIFY("Current Rate is: %d \n",tmpRate->currentRate);
}
/*CHANGE ENDS*/
/*CHANGE*/

void
adjDriverRate( PtpClock * ptpClock)
{
	DBGV("adjusting Driver Rate...\n");
	if(ptpClock->last_sync_receive_time.nanoseconds == 0)//需要上次接收sync报文的时间
	{
		ERROR("last_sync_receive_time is still zero\n");
		return ;
	}
	//display
	DBGV("currentOffset is:\n");
	timeInternal_display(&ptpClock->offsetFromMaster);
	DBGV("currentRate is: %d\t averageOffset is: %d\t averagePeriod is: %d\n",
			ptpClock->currentRate,ptpClock->averageOffset,ptpClock->averagePeriod);//当前频率，平局偏差，平均周期。
	DBGV("current sync receive time is:\n");
	timeInternal_display(&ptpClock->sync_receive_time);//当前收到sync包时间
	DBGV("last sync receive time is:\n");
	timeInternal_display(&ptpClock->last_sync_receive_time);//上次收到sync包时间
	TimeInternal tmpSyncPeriod;
	subTime(&tmpSyncPeriod,&ptpClock->sync_receive_time,&ptpClock->last_sync_receive_time);//计算sync包的临时周期
	int32_t syncPeriod = tmpSyncPeriod.seconds*1000000000 + tmpSyncPeriod.nanoseconds;//将sync包的周期转换成纳秒单位级
	DBGV("syncPeriod is: %dns",syncPeriod);
	int32_t diff;
	//Check if we need a new averagePeriod model
	if( abs(diff = ptpClock->averagePeriod - syncPeriod) > abs(syncPeriod) >>2 ) //use 1/2*syncPeriod as threshold
	{
		ptpClock->averagePeriod = syncPeriod;
		DBGV("syncPeriod difference is too large: %d\n", diff);
		DBGV("save the syncPeriod and return\n");
		return;
	}
	ptpClock->averagePeriod = (ptpClock->averagePeriod + 0.1*syncPeriod)/1.1;
	DBGV("now the averagePeriod is %d\n",ptpClock->averagePeriod);
	if( abs(diff = ptpClock->averageOffset - ptpClock->offsetFromMaster.nanoseconds) >
	abs(ptpClock->offsetFromMaster.nanoseconds)>>4)
	{
		ptpClock->averageOffset = ptpClock->offsetFromMaster.nanoseconds;
		DBGV("offset difference is too large: %d\n", diff);
		DBGV("save the syncPeriod and return\n");
		return;
	}
	ptpClock->averageOffset = (ptpClock->averageOffset + 0.5 * ptpClock->offsetFromMaster.nanoseconds)/1.5;

	int64_t tmpCurrentRate;
	tmpCurrentRate = ((int64_t)ptpClock->averageOffset << 32) / ptpClock->averagePeriod;
	tmpCurrentRate *= 8;
	DBGV("tmpRate is: %ld\n", tmpCurrentRate);

	if (ptpClock->flagAdjRate > 0)// the initial value of flagAdjRate is 1 so run this block twice
	{
		DBGV("last currentRate is: %d\ttmpCurrentRateis: %d\n",ptpClock->currentRate, (int32_t)tmpCurrentRate);
		int32_t changeRate;
		changeRate = ptpClock->currentRate + (int32_t)tmpCurrentRate;
		ptpClock->currentRate = changeRate;
		int dev;
		dev=open(DEVICE_FILENAME,O_RDWR|O_NDELAY);
		if(dev < 0) ERROR("cant open driver in tmpAdjDriverTime");
		int ret= ioctl(dev,ADJFREQ, &changeRate);
		if( ret <0)
		{
			ERROR("error in ioctl\n");
		}
		close(dev);
		NOTIFY("Adjusting frequency by %d\n",
				changeRate);
		//to make count average from the beginning
		ptpClock->averageOffset = 0;
		ptpClock->averagePeriod = 0;
	}
	ptpClock->flagAdjRate--;
	return;
}
/*CHANGE ENDS*/
/*CHANGE add read time function*/
void
readDriverTime(TimeInternal * time)
{
    int dev;
    dev=open(DEVICE_FILENAME,O_RDWR|O_NDELAY);
    if(dev < 0) ERROR("cant open driver in ReadDriverTime");
    ioctl(dev, RDTIME, time);
    close(dev);
	DBGV("driver time is %ds %dns\n",
	       time->seconds, time->nanoseconds);
}
/*CHANGE ENDS*/

/*CHANGE ADD lock on read cache time*/
int writew_lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	return(fcntl(fd,F_SETLKW,&fl));
}

int unlockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_UNLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	return(fcntl(fd,F_SETLK,&fl));
}
/*CHANGE add read time function*/
/*this is like the readDriverTime() but read different file*/
void
readCacheTime(TimeInternal * time, int dev)
{
//    int dev;
//    dev=open(DEVICE_P_FILENAME,O_RDWR|O_NDELAY);
//    if(dev < 0) ERROR("cant open driver in ReadCachedTime");
	//写锁定
	writew_lockfile(dev);
	//dp83640接口
    ioctl(dev, PTP_RD_TXTS, time);
    //写解锁
    unlockfile(dev);
//    close(dev);
	DBGV("read cached time is %ds %dns\n",
	       time->seconds, time->nanoseconds);
}
/*CHANGE ENDS*/
/*CHANGE store offset time in a file for analyze*/
int
time2log(TimeInternal * timeInternal)
{
	double time = ((double)timeInternal->nanoseconds / 1000000000) + timeInternal->seconds;
	FILE * fd;
    fd = fopen("./offset", "a");
    if(fd == NULL) {ERROR("Can't open file: offset"); return -1;}
    fprintf(fd,"%.9lf\n",time);
    fclose(fd);
    return 0;
}
/*CHANGE ENDS*/
double 
getRand(void)
{
	return ((rand() * 1.0) / RAND_MAX);
}

Boolean 
adjFreq(Integer32 adj)
{
	struct timex t;

	if (adj > ADJ_FREQ_MAX)
		adj = ADJ_FREQ_MAX;
	else if (adj < -ADJ_FREQ_MAX)
		adj = -ADJ_FREQ_MAX;

	t.modes = MOD_FREQUENCY;
	t.freq = adj * ((1 << 16) / 1000);

	return !adjtimex(&t);
}

void
rectifyInsertedTimestamp(TimeInternal *osTime, TimeInternal *timestamp)
{
	DBGV("rectifying \n");
	DBGV("timestamp is: \n");
	timeInternal_display(timestamp);
	DBGV("os_time is: \n");
	timeInternal_display(osTime);
	if(osTime->seconds == timestamp->seconds ||
			abs(osTime->seconds - timestamp->seconds) < 2)
	{
		DBGV("little seconds difference no need to rectify. \n");
		return;
	}
	/*if osTime larger than timestamp like 255
	 * it means driver carried first but os did not
	 * change timestamp forth by 0x100 secs
	*/
	else if(osTime->seconds > timestamp->seconds &&
			(abs(osTime->seconds - timestamp->seconds - 255) < 3))
	{
		timestamp->seconds += 0x100;
	}
	/*if timestamp larger than osTime like 255
	 * it means os carried first but driver did not
	 * change timestamp back by 0x100 secs
	 */
	else if(osTime->seconds < timestamp->seconds &&
			(abs(timestamp->seconds - osTime->seconds - 255) < 3))
	{
		timestamp->seconds -= 0x100;
	}
	else
	{
		DBGV("either a big difference between os_timestamp and driver_one \n");
		DBGV("or an unknown mistake, quit without changing\n");
		return;
	}

	DBGV("rectified\n Now the timestamp is: \n");
	timeInternal_display(timestamp);
	DBGV("os_time is: \n");
	timeInternal_display(osTime);
	return;
}
