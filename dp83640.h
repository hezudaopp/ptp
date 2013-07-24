#ifndef _DP83640_CHARDEV_H_
#define _DP83640_CHARDEV_H_

#define DP83640_MAGIC              	't'
#define DP83640_IOC_MAGIC 			'D'
/*
typedef int32_t Integer32;

typedef struct {
  Integer32 seconds;
  Integer32 nanoseconds;
} TimeInternal;
*/
#define SETTIME         _IOW(DP83640_MAGIC,0,TimeInternal)
//#define ADJFREQ          _IOW(DP83640_MAGIC,1,__s32)
#define TMPADJFREQ      _IOW(DP83640_MAGIC,1,TmpRate)
#define ADJTIME         _IOW(DP83640_MAGIC,2,TimeInternal)
#define RDTIME         	_IOR(DP83640_MAGIC,3,TimeInternal)
#define ADJFREQ         _IOW(DP83640_MAGIC,4,int32_t)
#define PTP_RD_TXTS		_IOR(DP83640_IOC_MAGIC, 16, TimeInternal)
#define DEVICE_P_FILENAME     "/dev/dp83640_driver"
#define DP83640_MAXNR     5 //3
#define DEVICE_FILENAME       "/dev/dp83640dev"
#endif


