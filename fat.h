/*-----------------------------------------------------------------------
  Added definitions/includes
------------------------------------------------------------------------*/
#ifndef FAT_DEFN
#define FAT_DEFN

#include "fatDefn.h"
// Add external references
extern struct fat_boot_sector fbs;  // FAT Boot Sector
extern unsigned short *fatPtr;  // pointer to the FAT Table
extern int fatfd;  // File descriptor for FAT file system

#endif
