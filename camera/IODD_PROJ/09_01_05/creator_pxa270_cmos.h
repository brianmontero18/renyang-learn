/*
 * cmos driver for Creator
 *
 *
 * Copyright (C) 2004 by Microtime Computer Inc.
 *
 * Linux kernel version history:
 * Version   : 1.00
 * History
 *   1.0.0 : Programming start (03/05/2004) -> SOP
 *
 */
 
#ifndef _CREATOR_S3C2410_CMOS_H_ 
#define _CREATOR_S3C2410_CMOS_H_ 

//#include <linux/config.h>
#if defined(__linux__)
#include <asm/ioctl.h>		/* For _IO* macros */
#define CCM_IOCTL_NR(n)	     		_IOC_NR(n)
#elif defined(__FreeBSD__)
#include <sys/ioccom.h>
#define CCM_IOCTL_NR(n)	     		((n) & 0xff)
#endif


#define CCM_MAJOR_NUM			110
#define CCM_IOCTL_MAGIC			CCM_MAJOR_NUM
#define CCM_IO(nr)			_IO(CCM_IOCTL_MAGIC,nr)
#define CCM_IOR(nr,size)		_IOR(CCM_IOCTL_MAGIC,nr,size)
#define CCM_IOW(nr,size)		_IOW(CCM_IOCTL_MAGIC,nr,size)
#define CCM_IOWR(nr,size)		_IOWR(CCM_IOCTL_MAGIC,nr,size)

// function headers


#define CMOS_SIZE_MODE_160_120		0
#define CMOS_SIZE_MODE_320_240		1

typedef enum {				/* CCM ���A			 */
    CMOS_ON=0, 				/* �_��CCM		   	 */	
    CMOS_OFF, 				/* ����CCM		   	 */
    CMOS_IMG_READY,			/* CCM�w�g�_�ʥB�v����ƷǳƦn   */
    CMOS_IMG_EMPTY			/* CCM�w�g�_�ʦ��v����ƨS�ǳƦn */
} cmos_status_e ;	

typedef struct CMOS_INFO {
	unsigned char  command;		/* PC �e�Ӫ�command, 		  */
					/* �]���ݤ���Linux��CCM_IOWR...   */
	unsigned char  ImageSizeMode ; 	/* �v���Ҧ��j�p			  */ 
	unsigned short ImageWidth;	/* �����IOCTL�ɥBcommand=CMOS_ON */
					/* �Ǧ^���v���e�P��		  */
	unsigned short ImageHeight;	        
	unsigned char  HighRef ;	/* �v�T�n���ɶ�,�U�j�ɶ��U������G�׷U�G */
	unsigned char  Status;		/* CCM �ثe���A			 */	
} cmos_info_t;



/* CCM specific ioctls 			*/
#define CMOS_ON				CCM_IOWR( 0x00, cmos_info_t)
#define CMOS_OFF			CCM_IO(   0x01)
#define CMOS_PARAMETER			CCM_IOW(  0x02, cmos_info_t)
#define CMOS_GET_STATUS			CCM_IOR(  0x03, cmos_info_t)

#define PC_CMOS_ON			0
#define PC_CMOS_OFF			1
#define PC_CMOS_PARAMETER		2
#define PC_CMOS_GET_STATUS		3

#endif // _CREATOR_S3C2410_CMOS_H_ 

