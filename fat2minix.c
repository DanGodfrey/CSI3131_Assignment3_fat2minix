/*-----------------------------------------------------------------
File: fat2minix.c
Description: This file contains code for reading the minix
	     file system. SB, Maps, Inode, Directory Tables.

	     Synopsis:

	     fat2minix <fat dev file> <minix dev file>

	     where <fat dev file> is the device file that contains the
	     FAT file system (example /dev/hdb1) with content.

	     and <minix dev file> contains the empty minix file system.
Student Name:
Student Number:
------------------------------------------------------------------*/
#include 	"fat2minix.h"
#include 	"fat.h"
#include 	"minix.h"
/*----------------------------------------------
The following global variables are accessed.
(defined in the fat.c module, see also fat.h)
They are initialised by readFatBoot
struct fat_boot_sector fbs;  // FAT Boot Sector
unsigned short *fatPtr;  // pointer to the FAT Table
int fatfd;  // File descriptor for FAT file system
--------------------------------------------------------*/ 
// Function Prototypes
int copyFatDir(void);
void copyDirEntries(char *, struct msdos_dir_entry *, int);
void processSubDirectory(struct msdos_dir_entry *, char *);
// Three functions to complete
void createMinixDir(struct dentry *, char *,struct msdos_dir_entry *);
void createMinixFile(struct dentry *, struct msdos_dir_entry *);
void addContentsToMinix(struct msdos_dir_entry *, struct minix_inode *);
// Some utility functions
char *getFatDataBlock(int, int , char *);
unsigned getMinixTimeFromFat(struct msdos_dir_entry *);

/*-----------------------------------------------------------------
Function: main()

Parameters: int argc - number of command line arguments
	   char **argv - pointers to command line arguments

Description: 
	Command synopsis: fat2minix <fat file> <minix file>
	<fat file> is the filename of the hard drive partition where
	       the FAT physical file system is located.
	<minix file> is the filename of the hard drive partition where
	       the minix physical file system is located.
------------------------------------------------------------------*/
int main(int argc, char **argv)
{
   int fd1;   /* file descriptor for minix file system */
   int fd2;   /* file descriptor for FAT file system */

   if(argc != 3)
   {
      printf("Usage: fat2minix <fat device> <minix device>\n");
      return(ERR1);
   }
   
   fd2 = open(argv[1],O_RDONLY);  /* open FAT fs for reading */
   if(fd2 == -1)
   {
      printf("Could not open %s\n",argv[1]);
      return(ERR1);
   }

   fd1 = open(argv[2],O_RDWR);  /* open minix fs for reading and writing */
   if(fd1 == -1)
   {
      close(fd2);
      printf("Could not open %s\n",argv[1]);
      return(ERR1);
   }

   if(readFatBoot(fd2) == ERR1)
   {
      printf("Error in reading FAT Boot Sector or FAT Table - terminating\n");
   }
   else if(initMinixFS(fd1) == ERR1)
   {
      printf("Error in initiallising Minix file system - terminating\n");
   }
   else
   {
      printf("Scanning the FAT Directory\n");
      copyFatDir(); 
   }
   close(fd2);
   closeMinixFS();
   return(OK);
}

/*-----------------------------------------------------------------
Function: copyFatDir

Parameters: none.

Global variables:  
         int fatfd - File descriptor to FAT File System
         struct fat_boot_sector fbs  - FAT Boot Sector - fat.c module

Description: Copies the contents of the FAT directory to the Minix
             directory. This function reads in the FAT root directory
             and calls the recursive routine copyDirEntries to recurse
             down the FAT directory structure for copying to the
	     Minix file system.
-----------------------------------------------------------------*/
int copyFatDir()
{
   // Config info from the boot sector
   int sectorSize = (*(short *)fbs.sector_size); // in bytes
   int maxRootEntries = *(short *) fbs.dir_entries; // number of directory entries
   int rootDirSize = sizeof(struct msdos_dir_entry)*maxRootEntries; // in bytes
   // allocate memory to store root directory
   struct msdos_dir_entry *rootdir = (struct msdos_dir_entry *) malloc(rootDirSize); 
   if(rootdir == NULL)
   {
      perror("copyFatDir-malloc");
      return(ERR1);
   }
   // Read in root directory
   lseek(fatfd,sectorSize*(1+fbs.fats*fbs.fat_length) ,SEEK_SET);
   read(fatfd, rootdir, rootDirSize);
   // Loop through the root directory
   copyDirEntries("/", rootdir, maxRootEntries);   // note that rootdir represents an address
   free(rootdir);  // free the allocated memory
   return(OK);
}

/*-----------------------------------------------------------------
Function: copyDirEntries

Parameters: char *name - name of directory
            struct msdos_dir_entry *dirTblPtr - pointer to a directory Table
	                                        consists of an array pointers to structures
            int numEntries - number of entries in the directory table

Description: Recursive function that copies files and directories to the 
             Minix file system for each valid entry in the FAT directory 
	     table referenced by dirTblPtr.  Recusion occurs when calling 
	     processSubDirectory that calls copyDirEntries. 

	     The perspective of this function is to scan a single FAT directory
	     table.  It opens the corresponding Minix directory (using the name
	     parameter) which should be empty. As files and subdirectories are
	     created, the Minix directory table is updated.
	     
	     Two main loops are used in this function.  The first run through the 
	     the FAT directory table calling creatMinixDir and creatMinixFile for 
	     creating the subdirectories and files respectively.  The opened Minix
	     directory table is also updated.  The second loop scans the directory
	     table again to call processSubDirectory for each directory found in
	     the table.

	     Notes on pointer variables and pointer arithmetic:
             - A pointer variable can be used like an
               array name when it points to an array.
             - Pointer arithmetic allows you to compute
               the address of an element in an array. 
	     - Thus, dirTblPtr+i gives the address of the ith element 
	       in the array referenced by dirTblPtr (note that dirTblPtr[i] 
               gives the value of the element, the expression
               is equivalent to *(dirTblPtr+i).
-----------------------------------------------------------------*/
void copyDirEntries(char *name, struct msdos_dir_entry *dirTblPtr, int numEntries)
{
    int i;
    struct dentry *minixDirTable;
    char filename[100];
    int numRecords;
    struct minix_inode ino;
    int inodeNum;
    int parentInodeNum;

    // Open the Minix Directory
    minixDirTable = openMinixDirectory(name,&numRecords, &inodeNum, &parentInodeNum, &ino);
    if(minixDirTable == NULL) printf("Error in opening minix directory %s\n", name);
    else
    {
       // Loop through the directory table and add entries
       for(i = 0 ; i < numEntries; i++)
       {
          if(dirTblPtr[i].attr == (char)0x0f)  // 0xf indicates LFN name
	      /* Ignore - not used in this assignment */;
          else if(dirTblPtr[i].name[0]==(char)0x00) { }   // available
          else if(dirTblPtr[i].name[0]==(char)0x05) { }   // deleted
          else if(dirTblPtr[i].name[0]==(char)0xE5) { }   // deleted
          else if(dirTblPtr[i].name[0]==(char)0x2E ||     // dot or dotdot
                  dirTblPtr[i].attr&ATTR_DIR)             // directory - assume name with no extension
          {
             if(getFatName(dirTblPtr+i, filename) != NULL)
	     {
	        if(strcmp(".",filename) == 0)  // only need to update Minix dir table
	        {
	          minixDirTable[numRecords].ino = inodeNum;
		  strcpy(minixDirTable[numRecords].name,".");
	        }
	        else if(strcmp("..",filename) == 0)  // only need to update Minix dir table
	        {
	          minixDirTable[numRecords].ino = parentInodeNum;
		  strcpy(minixDirTable[numRecords].name,"..");
	        }
	        else createMinixDir(minixDirTable+numRecords,   
		                    filename,dirTblPtr+i);
	     }
             numRecords++; // increase number of records
	     ino.i_nlinks++; // increase number of sub-directories
	     ino.i_size += sizeof(struct dentry); // increase size of directory table
          }
          else // Assume a file - first char in name is not one of the above values and
          {
             // ATTR_DIR does not have directory bit set
	     createMinixFile(minixDirTable+numRecords,dirTblPtr+i);
             numRecords++; // increase number of records
	     ino.i_size += sizeof(struct dentry); // increase size of directory table
          }
       }
       closeMinixDirectory(minixDirTable, numRecords, inodeNum, &ino);

       // Now recurse into subdirectories by calling processSubDirectory
       // that will call copyDirEntries
       for(i = 0 ; i < numEntries; i++)
       {
          if(dirTblPtr[i].attr == (char)0x0f)  // 0xf indicates LFN name
	        /* ignore no long directory names */; 
          else if(dirTblPtr[i].name[0]==(char)0x00) { }   // available
          else if(dirTblPtr[i].name[0]==(char)0x05) { }   // deleted
          else if(dirTblPtr[i].name[0]==(char)0xE5) { }   // deleted
          else if(dirTblPtr[i].name[0]==(char)0x2E) { }// dot or dotdot
          else if(dirTblPtr[i].attr&ATTR_DIR)  // Directory
             processSubDirectory(dirTblPtr+i, name); // recursion - will call copyDirEntries
       }
    }
}

/*-----------------------------------------------------------------
Function: processSubDirectory

Parameters: struct msdos_dir_entry *de - pointer to a directory entry
                                        that references sub-directory
            char *curMinixPath - current path to subdirectory

Global Variables:
       int fd;  // the file system file descriptor
       struct fat_boot_sector fbs;  // FAT Boot Sector
       unsigned short *fatPtr;  // pointer to the FAT Table


Description: Displays the contents of the directory entry
-----------------------------------------------------------------*/
void processSubDirectory(struct msdos_dir_entry *de, char *curMinixPath)
{
   char fatName[100];
   char minixName[100];
   unsigned short clusterNum;  // current cluster number
   // Get config values from boot sector
   int sectorSize = (*(short *)fbs.sector_size); // in bytes
   int maxRootEntries = *(short *) fbs.dir_entries; // number of entries
   int clusterSize = sectorSize * fbs.cluster_size; // in bytes
   // number of subdirectories in the cluster
   int numSubDirEntries = clusterSize/sizeof(struct msdos_dir_entry);
   struct msdos_dir_entry subDir[numSubDirEntries];  // sub directory table
   int flag;               // to control reading clusters
   // offset before FAT data region (in bytes)
   int toFATDataRegion = (1 + fbs.fat_length*(fbs.fats)) * sectorSize + 
                      (maxRootEntries*sizeof(struct msdos_dir_entry));
   // Build the name of the directory
   getFatName(de,fatName); 
   if(strcmp(curMinixPath,"/")==0) sprintf(minixName,"/%s",fatName);
   else sprintf(minixName,"%s/%s",curMinixPath,fatName);
   // Setup a cluster
   flag = TRUE; // keep reading clusters
   // Read all sectors of the directory using FAT table
   clusterNum = de->start;  // first cluster
   while(flag)
   {
     lseek(fatfd, (clusterNum-2)*clusterSize + toFATDataRegion, SEEK_SET);
     if(read(fatfd, subDir, clusterSize)>0)  // reads the cluster
     { // not the best error checking
          copyDirEntries(minixName, subDir, numSubDirEntries);   // note that subDir represents an address
     }
     if(fatPtr[clusterNum] == fatPtr[1]) flag = FALSE; // End of the chain
     else clusterNum = fatPtr[clusterNum]; // gets next cluster number
   }
}

/*-----------------------------------------------------------------
Function: createMinixDir

Parameters: struct dentry *newDirEntry - pointer to new directory entry
            char *name - name of the subdirectory.
            struct mdos_dir_entry *fatDir - pointer to FAT directory entry

Description: Creates a sub-directory in the Minix file system. To do this
             it updates an empty entry in the directory table referenced by
	     newDirEntry, and writes all zeros in the data block where the directory
	     table is to be located (so that all is seen are empty
	     directory entries in the new table).  Note that the 
	     creation of the "." and ".." entries in the 
	     are not created by this function.  The steps taken by this function are:
             1) Determine the inode number for the directory table:
		  - Find the first available inode in the inode table 
		    (i.e. the index of the first element with the value 0). 
		    see findFreeInode()
		  - Find a free data block (see findFreeDataBlock), set bit 
		    in bit map to used. In this case you should write all 
		    zeros into the corresponding data block to set up 
		    empty directory table - call the function "saveDataBlock" 
		    with appropriate arguments.
             2) Update the directory entry and inode attributes. 
	           - Fill the empty entry referenced by newDirEntry (name and inode number).
	           - Set the inode attributes: i_mode (see stat() to determine flags), 
		     i_uid, i_gid (use getuid() and getgid()), i_time using the
                     getMinixTimeFromFat() function, i_size and i_nlinks is left at zero, these
		     attributes will be updated by copyDirEntries when the directory is opened
		     to add at least "." and ".." the 2 entries that should be present
		     for all directories in the FAT directory.
-----------------------------------------------------------------*/
void createMinixDir(struct dentry *newDirEntry, char *name,
                    struct msdos_dir_entry *fatDir) 
{

   // Some output to show progress
   printf("Create Minix directory >%s<\n",name);
   fflush(stdout);
     // Complete this function
}

/*-----------------------------------------------------------------
Function: createMinixFile

Parameters: struct dentry *newDirEntry - handle to minix directory table entry
	    struct msdos_dir_entry *fatDir - pointer to the FAT directory entry

Description: Creates a file in the Minix file system. The FAT directory entry
             gives all specifics of the file in the FAT directory (including
	     how to access the contents of the file).
	     The steps taken by this function are:
	     1) fill the empty entry referenced by newDirEntry.
             2) determine the inode number for the file:
	          - set bit in inode bit map
		  - Create inode structure
		  - update attributes using upadteMinixInode - this is used 
		    to update the inodes for both files and directories.
                  - use FAT directory entry to determine name, etc.
             3) store contents in data block(s)
	          - if the contents of the file is zero (no content)
		    set the filesize in inode to 0. No data blocks allocated???
		  - Otherwise, since the file
		    has content call the function addContentsToMinixFile to
		    store the contents of the file in the Minix file system.
             2) Update the directory entry and inode attributes. 
	           - Fill the empty entry referenced by newDirEntry (name and inode number).
	           - Set the inode attributes: i_mode (see stat() to determine flags), 
		     i_uid, i_gid (use getuid() and getgid()), i_time using the
                     getMinixTimeFromFat() function, i_size with size of the file,
		     i_nlinks to 1 (only one link to the file).
-----------------------------------------------------------------*/
void createMinixFile(struct dentry *newDirEntry, struct msdos_dir_entry *fatDir) 
{
   char name[100];
   // Some output to show progress
   getFatName(fatDir,name);
   printf("Create Minix File >%s<\n",name);
   fflush(stdout);
     // Complete this function - note that it calls addContentsToMinix
}

/*-----------------------------------------------------------------
Function: addContentsToMinix

Parameters:  struct msdos_dir_entry *fatDir  - pointer to FAT File Directory Entry 
	     struct minix_inode *inoPtr - pointer to file inode

Description: Add the file content to the Minix file system. 
             Search file system for free datablocks and add contents to
	     datablocks allocated to the file (update inode).
	     Allocate blocks as necessary.  The following functions are
	     available to support the creation of this function:
	     getFatDataBlock (fat2minix.c module) - gets the ith block (size BLOCK_SIZE)
	                                            in the FAT file (composed of
						    clusers of size CLUSTER_SIZE).
	     findFreeDataBlock() (minix.c module) - finds a free data block in the
	                                            Minix file system (sets the
						    bit in the zone bit map).
	     writeDataBlock(minix.c module) - to write a data block to the Minix
	                                      file system.
	     The function must be able to deal with file sizes that require
	     the indirect block (i_zone(7)). In this case it is necessary to
	     allocate an additional data block to extend the data block index
	     table. 
----------------------------------------------------------------*/
void addContentsToMinix(struct msdos_dir_entry *fatDir, struct minix_inode *inoPtr)
{

     // Complete this function 
} 

/*-----------------------------------------------------------------
Function: getFatDataBlock

Parameters:  int blockNum - Minix Block Number (Minix block of of size BLOCK_SIZE)
             struct msdos_dir_entry *fatDir  - pointer to FAT Directory Entry 
	     char *block - pointer to buffer for storing block

Global Variables:
     fatfd - file descriptor of open FAT directory.

Description: Reads a block of data from a FAT cluster into the buffer
             referenced by block. It is assumed that CLUSTER_SIZE is 
	     a multiple of BLOCK_SIZE.
----------------------------------------------------------------*/
char *getFatDataBlock(int blockNum, int clusterNum, char *block)
{
   // Assume cluster size is a multiple of block size
   int mult = CLUSTER_SIZE/BLOCK_SIZE; // multiplier of BLOCK_SIZE to get ClUSTER_SIZE
   int num = blockNum/mult;    // logical cluster number to find
   int i;  // to count clusters
   char cluster[CLUSTER_SIZE];
   char *retadr = NULL;
   
   // find physical cluster number
   for(i=0 ; i<num ; i++) 
   {
      clusterNum = fatPtr[clusterNum]; 
      if(clusterNum == fatPtr[1]) break; // at last one  
   }
   // Read in cluster
   if(clusterNum == fatPtr[1]) retadr = NULL;
   else
   {
      if(lseek(fatfd, (clusterNum-2)*CLUSTER_SIZE + DATA_POS, SEEK_SET) != -1) 
      {
         if(read(fatfd, cluster, CLUSTER_SIZE)>0)  // reads the cluster
         {
            memcpy(block, cluster+(blockNum%mult)*BLOCK_SIZE, BLOCK_SIZE);
	    retadr = block;
         }
      }
   }
   return(retadr);
}

/*-----------------------------------------------------------------
Function: getMinixTimeFromFat

Parameters: fatDirEntryPtr  - pointer to FAT directory entry

Description: Determins the Minix time the contents of the FAT directory 
             entry referenced by fatDirEntryPtr.  The function is used 
	     to update i_time in the Minix Inode.
-----------------------------------------------------------------*/
unsigned getMinixTimeFromFat(struct msdos_dir_entry *fatDirEntryPtr)
{
   struct tm curTime;
   short time, date;  // for fields in fat dir entry

   time = fatDirEntryPtr->time, 
   date = fatDirEntryPtr->date;
   curTime.tm_sec = (time&0x001f)*2;  // FAT sec: 0-29, i.e. sec/2
   curTime.tm_min = (time&0x07ef)>>5;
   curTime.tm_hour = (time>>11)&0x1f; // preserve 5 bits only in case sign bit shifted
   curTime.tm_mon = ((date&0x01e0)>>5) - 1; // FAT mon: 1 to 12
   curTime.tm_mday = date&0x001f;
   curTime.tm_mon = ((date&0x01e0)>>5) - 1; // FAT mon: 1 to 12
                                                  // Minix Month: 0 to 11
   curTime.tm_year =  (1980+(date>>9)) - 1900; // Minix year, num years since 1900
                                                   // FAT year, num years since 1980
   return(mktime(&curTime)); // convert to Unix time 
}
