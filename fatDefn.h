/*--------------------------------------------------------------
File: fatDefn.h

Description: defninitions for FAT file system.
---------------------------------------------------------------*/

#ifndef FATDEFN_INCLUDE
#define FATDEFN_INCLUDE
/*
 * The MS-DOS filesystem constants/structures
 * subset of the /usr/include/linux/msdos_fs.h
 */
#include <asm/byteorder.h>

#define ATTR_RO      1  /* read-only */
#define ATTR_HIDDEN  2  /* hidden */
#define ATTR_SYS     4  /* system */
#define ATTR_VOLUME  8  /* volume label */
#define ATTR_DIR     16 /* directory */
#define ATTR_ARCH    32 /* archived */

#define ATTR_NONE    0 /* no attribute bits */
#define ATTR_UNUSED  (ATTR_VOLUME | ATTR_ARCH | ATTR_SYS | ATTR_HIDDEN)
	/* attribute bits that are copied "as is" */
#define ATTR_EXT     (ATTR_RO | ATTR_HIDDEN | ATTR_SYS | ATTR_VOLUME)
	/* bits that are used by the Windows 95/Windows NT extended FAT */

#define DELETED_FLAG 0xe5 /* marks file as deleted when in name[0] */

#define MSDOS_VALID_MODE (S_IFREG | S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO)
	/* valid file mode bits */

#define MSDOS_DOT    ".          " /* ".", padded to MSDOS_NAME chars */
#define MSDOS_DOTDOT "..         " /* "..", padded to MSDOS_NAME chars */


#define EOF_FAT16 0xFFF8

struct fat_boot_sector {
	__s8	ignored[3];	/* Boot strap short or near jump */
	__s8	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	__u8	sector_size[2];	/* bytes per logical sector */
	__u8	cluster_size;	/* sectors/cluster */
	__u16	reserved;	/* reserved sectors */
	__u8	fats;		/* number of FATs */
	__u8	dir_entries[2];	/* root directory entries */
	__u8	sectors[2];	/* number of sectors */
	__u8	media;		/* media code (unused) */
	__u16	fat_length;	/* sectors/FAT */
	__u16	secs_track;	/* sectors per track */
	__u16	heads;		/* number of heads */
	__u32	hidden;		/* hidden sectors (unused) */
	__u32	total_sect;	/* number of sectors (if sectors == 0) */
};

struct msdos_dir_entry {
	__s8	name[8],ext[3];	/* name and extension */
	__u8	attr;		/* attribute bits */
	__u8    lcase;		/* Case for base and extension */
	__u8	ctime_ms;	/* Creation time, milliseconds */
	__u16	ctime;		/* Creation time */
	__u16	cdate;		/* Creation date */
	__u16	adate;		/* Last access date */
	__u16   starthi;	/* High 16 bits of cluster in FAT32 */
	__u16	time,date,start;/* time, date and first cluster */
	__u32	size;		/* file size (in bytes) */
};

/* Up to 13 characters of the name */
struct msdos_dir_slot {
	__u8    id;		/* sequence number for slot */
	__u8    name0_4[10];	/* first 5 characters in name */
	__u8    attr;		/* attribute byte */
	__u8    reserved;	/* always 0 */
	__u8    alias_checksum;	/* checksum for 8.3 alias */
	__u8    name5_10[12];	/* 6 more characters in name */
	__u16   start;		/* starting cluster number, 0 in long slots */
	__u8    name11_12[4];	/* last 2 characters in name */
};

/*-----------------------------------------------------------------------
  Added definitions/includes
------------------------------------------------------------------------*/
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* Definitions */
#define OK 0
#define ERR1 -1
#define TRUE 1
#define FALSE 0 

/* structure for fat directory table */
struct fatDirTable
{
   char name[BUFSIZ];  // store name 
   unsigned short clusterNum;  // set to 0 for root directory
   unsigned short parentCluster;  // set to 0 for root directory
   struct msdos_dir_entry *table; // the directory table
   int numEntries;  // number of entries in the table
   int size;  // size in bytes
};
typedef struct fatDirTable FATDIR;

/********* Some defines that use global variables *********/
#define SECTOR_SIZE (*(short *)fbs.sector_size) // size in bytes
#define CLUSTER_SIZE (SECTOR_SIZE*fbs.cluster_size)  // in bytes
#define FAT_POS SECTOR_SIZE
#define ROOTDIR_POS (SECTOR_SIZE*(1+fbs.fats*fbs.fat_length))
#define DATA_POS (ROOTDIR_POS+((*(short *)fbs.dir_entries)*sizeof(struct msdos_dir_entry)))
#define LAST_CLUSTER fatPtr[1]  // last cluster indicator
#define NUM_FAT_ENTRIES ((fbs.fat_length*SECTOR_SIZE)/2)  // number of entries in FAT

/*-----------------------------------------------------------------------
  Function Prototypes
------------------------------------------------------------------------*/
/* fatModule.c */
int readFatBoot(int );
int readFatTable(void); 
FATDIR *openFatDirectory(char *);
void closeFatDirectory(FATDIR *);
int getFatDirTable(char *, FATDIR *);
int scanSubDirectories(char *, struct msdos_dir_entry *, FATDIR *, unsigned short);
void writeCluster(int , void *, char *);
void readCluster(int , void *, char *);
char getmsTime(time_t );
unsigned short getTime(time_t );
unsigned short getDate(time_t );
void printFatDirEntries(FATDIR *);
void displayFatDirEntry(struct msdos_dir_entry *);
void printFatTable(unsigned short *, int );
char *getFatName(struct msdos_dir_entry *, char *);

#endif
