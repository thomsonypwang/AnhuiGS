#ifndef SYS_ERRNO_H
#define SYS_ERRNO_H

/* Get the module index number from error code (4th byte from LSB)*/
#define get_module_base(code) ((code&0xF000)>>12)

/* Get notifier message type i.e Error, Warning or Info (3rd byte from LSB)*/
#define get_notifier_msg_type(code) ((code&0x0F00)>>8)

/* Get module notifier code (2nd and 1st byte from LSB)*/
#define get_code(code) (code&0xFF)

#define MOD_ERROR_START(x)  (x << 12 | 0)
#define MOD_WARN_START(x)   (x << 12 | 1)
#define MOD_INFO_START(x)   (x << 12 | 2)

/* Create Module index */
#define MOD_GENERIC    0
/** Unused */
#define MOD_UNUSED_3   2
/** HTTPD module index */
#define MOD_HTTPD      3
/** Application framework module index */
#define MOD_AF         4
/** FTFS module index */
#define MOD_FTFS       5
/** RFGET module index */
#define MOD_RFGET      6
/** JSON module index  */
#define MOD_JSON       7
/** TELNETD module index */
#define MOD_TELNETD    8
/** SIMPLE MDNS module index */
#define MOD_SMDNS      9
/** EXML module index */
#define MOD_EXML       10
/** DHCPD module index */
#define MOD_DHCPD      11
/** MDNS module index */
#define MOD_MDNS       12
/** SYSINFO module index */
#define MOD_SYSINFO   13
/** Unused module index */
#define MOD_UNUSED_1     14
/** CRYPTO module index */
#define MOD_CRYPTO     15
/** HTTP-CLIENT module index */
#define MOD_HTTPC      16
/** PROVISIONING module index */
#define MOD_PROV       17
/** SPI module index */
#define MOD_SPI        18
/** PSM module index */
#define MOD_PSM        19
/** TTCP module index */
#define MOD_TTCP       20
/** DIAGNOSTICS module index */
#define MOD_DIAG       21
/** Unused module index */
#define MOD_UNUSED_2    22
/** WPS module index */
#define MOD_WPS        23
/** WLAN module index */
#define MOD_WLAN        24
/** USB module index */
#define MOD_USB        25
/** WIFI driver module index */
#define MOD_WIFI        26
/** Critical error module index */
#define MOD_CRIT_ERR    27
/** Last module index .Applications can define their own modules beyond this */
#define MOD_ERR_LAST	50

enum sys_errno 
{
	SYS_OK=0,       /*!<  value indicating success (no error) */
	SYS_FAIL=-1,     /* 1 */ /*!< Generic  code indicating failure */
	SYS_E_PERM=-2,   /* 2: Operation not permitted */
	SYS_E_NOENT=-3,  /* 3: No such file or directory */
	SYS_E_SRCH=-4,   /* 4: No such process */
	SYS_E_INTR=-5,   /* 5: Interrupted system call */
	SYS_E_IO=-6,     /* 6: I/O error */
	SYS_E_NXIO=-7,   /* 7: No such device or address */
	SYS_E_2BIG=-8,   /* 8: Argument list too long */
	SYS_E_NOEXEC=-9, /* 9: Exec format error */
	SYS_E_BADF=-10,   /* 10: Bad file number */
	SYS_E_CHILD=-11,  /* 11: No child processes */
	SYS_E_AGAIN=-12,  /* 12: Try again */
	SYS_E_NOMEM=-13,  /* 13: Out of memory */
	SYS_E_ACCES=-14,  /* 14: Permission denied */
	SYS_E_FAULT=-15,  /* 15: Bad address */
	SYS_E_NOTBLK=-16, /* 16: Block device required */
	SYS_E_BUSY=-17,   /* 17: Device or resource busy */
	SYS_E_EXIST=-18,  /* 18: File exists */
	SYS_E_XDEV=-19,   /* 19: Cross-device link */
	SYS_E_NODEV=-20,  /* 20: No such device */
	SYS_E_NOTDIR=-21, /* 21: Not a directory */
	SYS_E_ISDIR=-22,  /* 22: Is a directory */
	SYS_E_INVAL=-23,  /* 23: Invalid argument */
	SYS_E_NFILE=-24,  /* 24: File table overflow */
	SYS_E_MFILE=-25,  /* 25: Too many open files */
	SYS_E_NOTTY=-26,  /* 26: Not a typewriter */
	SYS_E_TXTBSY=-27, /* 27: Text file busy */
	SYS_E_FBIG=-28,   /* 28: File too large */
	SYS_E_NOSPC=-29,  /* 29: No space left on device */
	SYS_E_SPIPE=-30,  /* 30: Illegal seek */
	SYS_E_ROFS=-31,   /* 31: Read-only file system */
	SYS_E_MLINK=-32,  /* 32: Too many links */
	SYS_E_PIPE=-33,   /* 33: Broken pipe */
	SYS_E_DOM=-34,    /* 34: Math argument out of domain of func */
	SYS_E_RANGE=-35,  /* 35: Math result not representable */

	/*  generic error codes */
	SYS_E_CRC=-36,    /* 36: Error in CRC check */
	SYS_E_UNINIT=-37,  /* 37: Module is not yet initialized */
	SYS_E_TIMEOUT=-38, /* 38: Timeout occurred during operation */
	    /* Defined for Hostcmd specific API*/
    SYS_E_INBIG=-39,   /* 39: Input buffer too big */
    SYS_E_INSMALL=-40, /* 40: A finer version for WM_E_INVAL, where it clearly specifies that input is much smaller than minimum requirement */
    SYS_E_OUTBIG=-41,  /* 41: Data output exceeds the size provided */
};

#endif 
