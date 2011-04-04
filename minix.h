/*-----------------------------------------------------------------
File: minix.h
Description: Contains definitions and include files for the 
             minix module.
------------------------------------------------------------------*/

#ifndef MINIX_H_DEF
#define MINIX_H_DEF
/* Include files */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/minix_fs.h>

/* Definitions */
#define OK 0
#define ERR1 -1
#define TRUE 1
#define FALSE 0 

/***************Disk Layout ***********************************************/
/* Boot 0, SB 1, IMAP 2-3, ZMAP 4-11, ITABLE 12-523, Data ZONES 524-65535 */
/**************************************************************************/
/* Define some basic numbers */
/* some definitions use minixSB global data variable */
#define BLOCK_SIZE 1024  /* size of blocks */
#define DIRENTRYSIZE (30+2)
#define INODE_SIZE sizeof(struct minix_inode)
#define TOTALBLOCKS ((64*1024)-1)  /* Total number of zones (blocks) - 64 K */
#define NUMITABLEBLOCKS minixSB.s_ninodes*INODE_SIZE/BLOCK_SIZE
/*First block is 0 boot block*/
#define FIRSTZONE (1+1+minixSB.s_imap_blocks+minixSB.s_zmap_blocks+NUMITABLEBLOCKS) 
#define TOTALDATABLOCKS minixSB.s_nzones-FIRSTZONE /* Total number of zones (data blocks) - 64 K */

/* Directory table */
struct dentry
{
   short ino;
   char name[30];
};

/******************* Entry Point Prototypes **********************/
// Minix File System
int initMinixFS(int);
void closeMinixFS(void);

// Functions to manipulate Minix Directories
struct dentry *openMinixDirectory(char *, int *, int *, int *, struct minix_inode *);
void closeMinixDirectory( struct dentry *, int, int, struct minix_inode *); 
int scanMinixSubDirectories(char *, struct dentry *, int, 
                            struct minix_inode *, int, int *);
struct dentry *getMinixDirTable(struct minix_inode *, int *);
void saveMinixDirTable(struct minix_inode *, struct dentry *, int);

// Functions to manipulate Inodes
int findInodeFromPath(char *, struct minix_inode *, int *);
short findFreeInode(void);
int readInode(int, struct minix_inode *);
int saveInode(int, struct minix_inode *);
int seekToInode(int);

// Functions to manipulate data blocks (zones)
int findFreeDataBlock(void);
int getDataBlock(int, struct minix_inode *, char *);
int writeDataBlock(int, char *);
int saveDataBlock(int, struct minix_inode *, char *);
int seekToDataBlock(int, struct minix_inode *);

// Functions to support debugging
void printInode(struct minix_inode *);

#endif
