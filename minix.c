/*----------------------------------------------------------------- 
File: minix.c
Description: This file contains code for manipulating the contents of
             a Minix directory.
------------------------------------------------------------------*/

#include "minix.h"
#include "fat.h"

// Global data structures - initialised by initMinixFS
int minixfd;  // file discriptor of open Minix file system
struct minix_super_block minixSB;  // the Minix super block
unsigned char *imap; // inode map
unsigned char *zmap; // zone, data block, map

//*************** Prototypes of local functions **********************
// See minix.h for the prototype functions of entry points (i.e. functions
// available to other modules
// Functions to support opening file system and reading data structures
unsigned char *loadIMAP(void);
unsigned char *loadZMAP(void);

//************************************************************
// Functions for opening and closing the Minix File system
//************************************************************

/*-----------------------------------------------------------------
Function: initMinixFS(fd)

Parameters: int fd - file descriptor of open file system 

Global variables/structures:
    int minixfd - file discriptor of open Minix file system
    struct minix_super_block minixSB  - the super block
    unsigned char *imap - inode map
    unsigned char *zmap - zone, data block, map

Description: Reads and displays the minix super block. Also
             sets up the global data variables/structures.
-----------------------------------------------------------------*/
int initMinixFS(int fd)
{
    int n;
    int retcd = OK;
    // initialise minixfd
    minixfd = fd;
    // Get the super block
    if(lseek(fd,BLOCK_SIZE,SEEK_SET) == -1) /* move to super block */
    {
       printf("Could not seek to super-block\n");
       retcd = ERR1;
    }
    else
    {
       n = read(fd,&minixSB,sizeof(struct minix_super_block));
       if(n != sizeof(struct minix_super_block))
       {
          printf("Could not read super-block (%d,%d)\n",n,sizeof(struct minix_super_block));
	  retcd = ERR1;
       }
       else
       {
           /* Printout the contents */
          printf("------------SUPER Block - Minix Version 1--------------\n");
          printf("Number of inodes %d\n",minixSB.s_ninodes);
          printf("Number of blocks %d\n",minixSB.s_nzones);
          printf("Number of IMAP Blocks %d\n",minixSB.s_imap_blocks);
          printf("Number of MAP Blocks %d\n",minixSB.s_zmap_blocks);
          printf("First data block %d\n",minixSB.s_firstdatazone);
          printf("Zone size %d (should always be 0)\n",minixSB.s_log_zone_size);
          printf("Maximum size of file %d\n",minixSB.s_max_size);
          printf("Magic number %x\n",minixSB.s_magic);
          printf("State %d\n",minixSB.s_state);
          printf("Number of data blocks %d\n",minixSB.s_zones);
          printf("-----------------------------------------\n\n");
       }
    }

    // Load the maps
    imap = loadIMAP();  // Inode Map
    if(imap == NULL) retcd = ERR1;
    else
    {
       zmap = loadZMAP(); // Block/zone map
       if(zmap == NULL) 
       {
	  retcd = ERR1;
          free(imap);
       }
    }
    return(retcd);
}

/*-----------------------------------------------------------------
Function: closeMinixFS()

Parameters: none

Global variables:
    struct minix_super_block minixSB  - the super block
    unsigned char *imap - inode map
    unsigned char *zmap - zone, data block, map

Description: Saves maps, frees up allocated memory and closes the file.
-----------------------------------------------------------------*/
void closeMinixFS()
{
   int n;
   int imapsize = minixSB.s_imap_blocks*BLOCK_SIZE; // size of imap
   int zmapsize = minixSB.s_zmap_blocks*BLOCK_SIZE; // size of zmap

   // save maps and free allocated memory to maps
   // IMAP
   if(lseek(minixfd,2*BLOCK_SIZE,SEEK_SET) == -1) /* move to imap */
          printf("Could not seek to IMAP\n");
   else
   {
       n = write(minixfd,imap,imapsize);
       if(n != imapsize) printf("Could not write IMAP (%d,%d)\n",n,imapsize);
   }
   free(imap);
   // ZMAP
   if(lseek(minixfd,(2+minixSB.s_imap_blocks)*BLOCK_SIZE,SEEK_SET) == -1) /* move to zmap */
     printf("Could not seek to ZMAP\n");
   else
   {
      n = write(minixfd,zmap,zmapsize);
      if(n != zmapsize)
         printf("Could not write ZMAP (%d,%d)\n",n,zmapsize);
   }
   free(zmap);
   // close file
   close(minixfd);
}

/*-----------------------------------------------------------------
Function: loadIMAP

Global variables/structures:
    struct minix_super_block minixSB  - the super block
    unsigned char *imap - inode map

Description: Allocates memory for the Inode map (address saved in imap)
             and loads the Inode map from the disk to the allocated memory.
-----------------------------------------------------------------*/
unsigned char *loadIMAP()
{
    int imapsize = minixSB.s_imap_blocks*BLOCK_SIZE; // size of imap
    int n; // number of bytes read
    unsigned char *map; // working pointer variable

    // Allocate memory
    map = malloc(minixSB.s_imap_blocks*BLOCK_SIZE);
    if(map == NULL)
       fprintf(stderr,"Could not allocate memory for IMAP\n");
    else
    {  // Read in the IMAP
       if(lseek(minixfd,2*BLOCK_SIZE,SEEK_SET) == -1) // move to imap on disk
       {
          printf("Could not seek to IMAP\n");
	  free(map);
          map = NULL;
       }
       else
       {
           n = read(minixfd,map,imapsize);  // reads imap from disk to memory
           if(n != imapsize)
           {
             printf("Could not read IMAP (%d,%d)\n",n,imapsize);
	     free(map);
	     map = NULL;
           }
       }
    }
    return(map);
}

/*-----------------------------------------------------------------
Function: loadZMAP

Global variables/structures:
    struct minix_super_block minixSB  - the super block
    unsigned char *zmap - zone map

Description: Allocates memory for the Zone (i.e. data block) map 
             (address saved in zmap) and loads the Inode map from 
	     the disk to the allocated memory.
-----------------------------------------------------------------*/
unsigned char *loadZMAP()
{
    int zmapsize = minixSB.s_zmap_blocks*BLOCK_SIZE; // size of zmap
    int n; // number of bytes read
    unsigned char *map;  // working pointer variable

    // Allocate memory
    map = malloc(minixSB.s_zmap_blocks*BLOCK_SIZE);
    if(map == NULL)
       fprintf(stderr,"Could not allocate memory for ZMAP\n");
    else
    {  // Read in the ZMAP
       if(lseek(minixfd,(2+minixSB.s_imap_blocks)*BLOCK_SIZE,SEEK_SET) == -1) /* move to zmap on disk */
       {
          printf("Could not seek to ZMAP\n");
	  free(map);
          map = NULL;
       }
       else
       {
           n = read(minixfd,map,zmapsize);  // read map from the disk into the memory
           if(n != zmapsize)
           {
             printf("Could not read ZMAP (%d,%d)\n",n,zmapsize);
	     free(map);
             map = NULL;
           }
       }
    }
    return(map);
}

//************************************************************
// Functions for Manipulating Minix Directories
//************************************************************

/*-----------------------------------------------------------------
Function: openMinixDirectory

Parameters: char *dirPathName - full path name of directory to open
            int *numRecs - used to return number of records in
	                   the directory
            int *inoNum - used to return inode number
            int *parentInoNum - used to return the parent inode number
            int *inoPtr - used to return contents of inode.

Description: Finds a Minix Directory Table on the Minix file system
             (hard drive) and reads it into a "struct dentry" array
	     (see minix.h). Note that memory is allocated
	     for storing the contents of the directory table by
             getMinixDirTable(); this allocated memory is freed in 
	     closeMinixDirectory.
-----------------------------------------------------------------*/
struct dentry *openMinixDirectory(char *dirPathName, int *numRecs, 
                                  int *inoNum, int *parentInoNum,
				  struct minix_inode *inoPtr)
{
   struct dentry *dirPtr = NULL;  // to indicate error
   *inoNum = findInodeFromPath(dirPathName, inoPtr, parentInoNum);
   if(*inoNum == ERR1)
      fprintf(stderr,"Could not open directory %s\n",dirPathName);
   else
   {
      dirPtr = getMinixDirTable(inoPtr, numRecs);
      if(dirPtr == NULL)
         fprintf(stderr,"Error in reading %s\n",dirPathName);
   }
   return(dirPtr); 
}

/*-----------------------------------------------------------------
Function: closeMinixDirectory

Parameters: struct dentry *dirPtr - pointer to directory table
            int numRecs - number of records
            struct minix_inode *inoPtr - pointer to inode of dir table 

Description: Writes Minix directory table to disk and frees allocated memory.
-----------------------------------------------------------------*/
void closeMinixDirectory( struct dentry *dirPtr, int numRecs,
                          int inoNum, struct minix_inode *inoPtr)
{
   // Gets the size of the directory table from the number
   // of records
   inoPtr->i_size = sizeof(struct dentry)*numRecs;
   saveMinixDirTable(inoPtr, dirPtr, numRecs);
   saveInode(inoNum,inoPtr);
   free(dirPtr);  // frees allocated memory
}

/*-----------------------------------------------------------------
Function: scanMinixSubDirectories

Parameters: char *path - name of the subdirectory (with no prefix)
            struct dentry *tbl - pointer to a directory 
	                         table (array of entries)
            struct minix_inode *inoPtr - pointer to memory to store inode
            int parentInoNum  - for passing parent inode number to next iteration
            int *parentInoNum  - for returning the parent inode number of 
	                         leaf directory

Returns: inode number where directory is stored and the parent inode number
         in *parentInoNum.

Description: Recursive function that scans directory to find inode and contents
             of a directory.  Fills in the memory pointed by dirTablePtr
	     with the directory table named "path".  Returns the
	     inode number where the directory table is stored.
-----------------------------------------------------------------*/
int scanMinixSubDirectories(char *path, struct dentry *tbl, 
		            int numrecords, struct minix_inode *inoPtr,
			    int parentInoNum, int *retParentInoNum)
{
   char subDirName[BUFSIZ]; // for loading in the subdirectory name
   char *pt=subDirName; // pointer to copy name
   int ix; // index for seaching through table
   int retcd = ERR1;  // for return code
   struct minix_inode ino;
   struct dentry *nextTbl;

   // Get name to search in directory table (and remove from head of path)
   while(*path!='/' && *path!='\0') *pt++=*path++;
   *pt='\0';  // terminates string
   if(*path == '/') path++; // skips the '/'
   
   // Search for sub directory
   for(ix = 0 ; ix < numrecords ; ix++)
   {
      if((strncmp(subDirName, tbl[ix].name, strlen(subDirName)) == 0)) // found it
      {
         readInode(tbl[ix].ino, &ino); // get inode
         if(*path == '\0') // if at end of path, then found directory 
	 {
            memcpy(inoPtr, &ino, sizeof(struct minix_inode));  // Copy root inode
	    *retParentInoNum = parentInoNum;
	    retcd = tbl[ix].ino;
	 }
	 else // otherwise need to find next subdirectory in path
	 {
            nextTbl = getMinixDirTable(&ino, &numrecords);
            // the following is a recursive function
            retcd = scanMinixSubDirectories(path, nextTbl, numrecords, inoPtr, 
	                                    tbl[ix].ino, retParentInoNum);
            free(nextTbl);  // frees allocated memory
	 }
         break;  // leave the loop
      }
   }
   if(ix == numrecords) // did not find the name in the table
   {
      printf("Could not find subdirectory %s\n", subDirName);
      retcd = ERR1;
   }
   return(retcd);
}

/*-----------------------------------------------------------------
Function: getMinixDirTable

Parameters: struct minix_inode *inoPtr - pointer to inode
            int *numRecords - pointer used to return number of records

Returns:   Address of directory table array (memory must be freed using free).
           NULL - error occured

Description: Finds the directory table and loads it into an array
             of struct dentry elements.  Allocates necessary memory
             for the table.  
-----------------------------------------------------------------*/
struct dentry *getMinixDirTable(struct minix_inode *inoPtr, int *numRecords)
{
   int numDataBlocks;  // number of data blocks occupied by directory table
   struct dentry *dirTablePtr;
   int i,j;  // for counting records and blocks
   char datablock[BLOCK_SIZE];  // for loading data block

     /* Determine size of the directory table */
   if(inoPtr->i_size%BLOCK_SIZE) numDataBlocks =  1 + inoPtr->i_size/BLOCK_SIZE;
   else numDataBlocks = inoPtr->i_size/BLOCK_SIZE;
   *numRecords = inoPtr->i_size/sizeof(struct dentry);
   // Allocate memory for the table
   dirTablePtr = malloc(7*BLOCK_SIZE);  // allocate maximum amount of memory
   if(dirTablePtr != NULL)
   {
       // zero memory
       memset(dirTablePtr,0,7*BLOCK_SIZE);
       /* Read the contents */
       for(i=0 ; i<*numRecords ; i++)
       {       
	   j=i%(BLOCK_SIZE/sizeof(struct dentry)); // determines index into datablock
	   if(j == 0) /* need more records */
	   {
	      if(getDataBlock(i/(BLOCK_SIZE/sizeof(struct dentry)), inoPtr, datablock) == ERR1) 
	      {
	         free(dirTablePtr);
	         dirTablePtr = NULL;
                 i=*numRecords;  // to break the loop
	      }
	   }
	   if(i!= *numRecords) 
	   {
	      memcpy( dirTablePtr+i, datablock+(j*sizeof(struct dentry)), sizeof(struct dentry));
	   }
	}
   }
   return(dirTablePtr);
}

/*-----------------------------------------------------------------
Function: saveMinixDirTable

Parameters: struct minix_inode *inoPtr - pointer to inode
            struct dentry *dirTablePtr - pointer to directory table
            int numRecords - number of records to save

Description: Saves the directory table. Allocates new data blocks if necessary.
-----------------------------------------------------------------*/
void saveMinixDirTable(struct minix_inode *inoPtr, 
                      struct dentry *dirTablePtr, 
		      int numRecords)
{
   int sizeTable;  // size of the directory table
   int numRequired;  // number of data blocks required to store table
   int i;  // for counting records and blocks
   // Find number of blocks required to save
   sizeTable = (numRecords * sizeof(struct dentry));
   if(sizeTable%BLOCK_SIZE) numRequired =  1 + sizeTable/BLOCK_SIZE;
   else numRequired = sizeTable/BLOCK_SIZE;
   if(numRequired > 7)
      fprintf(stderr,"Current version does not allocate more than 7 data blocks\n");
   else
   {
       // Save contents onto the disk
       for(i=0 ; i<numRequired ; i++)
       {
          // Get a new data block if not allocated
          if(inoPtr->i_zone[i]==0) inoPtr->i_zone[i] = findFreeDataBlock();
          saveDataBlock(i, inoPtr, ((char *)dirTablePtr)+(i*BLOCK_SIZE) );
       }
   }
}

//************************************************************
// Functions for manipulating inodes
//************************************************************

/*-----------------------------------------------------------------
Function: findInodeFromPath

Parameters: char *path  - full path name of directory/file
	    struct minix_inode *inoPtr - pointer to location for loading inode
	    int *parentInoNum - the inode number of the parent.

Returns:  Inode number or ERR1 when and error occurs. and sets *parentInoNum to
          the value of the parent inode number.

Description: Finds the inode of dir/file and loads it into the struct minix_inode
             referenced by inoPtr.  Retures OK if all went well and ERR1 upon
	     detection of an error. If the directory is not the
	     root directory, the recursive function scanMinixSubDirectories
	     is called to find it.
-----------------------------------------------------------------*/
int findInodeFromPath(char *path, struct minix_inode *inoPtr, int *parentInodeNum)
{
   struct minix_inode ino;
   int inodeNum = 1;  // set do root directory inode number
   int numrecords;
   struct dentry *rootdir;
   readInode(inodeNum, &ino); // get root inode
   if(strcmp(path, "/") == 0)
   {
      memcpy(inoPtr, &ino, sizeof(struct minix_inode));  // Copy root inode
      *parentInodeNum = 1;  // root parent
   }
   else
   {
      if(*path == '/') path++; // skip over the initial /
      rootdir = getMinixDirTable(&ino, &numrecords);
      // the following is a recursive function
      inodeNum = scanMinixSubDirectories(path, rootdir, numrecords, inoPtr, 1, parentInodeNum);
      free(rootdir);  // frees allocated memory
   }
   return(inodeNum);
}

/*-----------------------------------------------------------------
Function: findFreeInode

Global Variables:
   int minixfd - file descriptor of open fs.
   unsigned char *imap - inode map

Returns: ERR1 (-1) - error encountered.
         inode number - otherwise.

Description: 
        Finds a free inode using bit map, sets the bit,
	and returns inode number.
-----------------------------------------------------------------*/
short findFreeInode()
{
   int bytenum, bitnum;
   short inodenum;
   // first find byte with a zero
   for(bytenum=0; bytenum<minixSB.s_ninodes/8 ; bytenum++)
      if(imap[bytenum] != 0xff) break;
   // find bit that is clear
   for(bitnum=0 ; bitnum<8 ; bitnum++)
      if((imap[bytenum] & (1<<bitnum))==0) break;
   // Compute and check block number
   inodenum = bytenum*8 + bitnum;
   if(inodenum > minixSB.s_ninodes)
   {
      fprintf(stderr,"No free inodes\n");
      inodenum = ERR1;
   }
   else
   {
      // set the bit
      bitnum = 1<<bitnum;
      imap[bytenum] |= bitnum;
   }
   return(inodenum);
}

/*------------------------------------------------------------------
Function: readInode(ino_num, ino)

Parameters: ino_num - number of inode to read (index)
	    ino - pointer for saving contents of inode.
	    
Returns: ERR1 - error in reading the inode.
         OK - successful

Description: Reads in an inode.
-----------------------------------------------------------------*/
int readInode(int ino_num, struct minix_inode *ino)
{
     int retcd = OK;

     if(seekToInode(ino_num)==ERR1)
     {
	perror("readInode");
        retcd = ERR1;
     }
     else if(read(minixfd,ino,sizeof(struct minix_inode)) != sizeof(struct minix_inode)) 
     {
	perror("readInode");
        retcd = ERR1;
     }
     return(retcd);
}

/*------------------------------------------------------------------
Function: saveInode(ino_num, ino)

Parameters: ino_num - number of inode to save (index)
	    ino - pointer for saving contents of inode.
	    
Returns: ERR1 - error in saving the inode.
         OK - successful

Description: saves an inode.
-----------------------------------------------------------------*/
int saveInode(int ino_num, struct minix_inode *ino)
{
     int retcd = OK;

     if(seekToInode(ino_num)==ERR1) 
     {
	perror("saveInode (seek)");
        retcd = ERR1;
     }
     else if(write(minixfd,ino,sizeof(struct minix_inode)) != sizeof(struct minix_inode)) 
     {
        printf("Error writing inode %d\n",ino_num);
	perror("saveInode (write)");
        retcd = ERR1;
     }
     return(retcd);
}

/*------------------------------------------------------------------
Function: seekInode(ino_num)

Parameters: ino_num - pointer for saving contents of inode.

Global Variables:
     int minixfd - file descriptor of open file system
     struct minix_super_block minixSB;  // super block
	    
Returns: ERR1 - error in reading the inode.
         OK - successful

Description: Seeks to the inode "ino_num".
-----------------------------------------------------------------*/
int seekToInode(int ino_num)
{
     int start = (2+minixSB.s_imap_blocks+minixSB.s_zmap_blocks)*BLOCK_SIZE;

     if(lseek(minixfd,(start+((ino_num-1)*INODE_SIZE)),SEEK_SET) == -1) 
     {
         perror("seekToInode");
         return(ERR1);
     }
     else return(OK);
}

//************************************************************
// Functions for Manipulating Data Blocks (zones)
//************************************************************

/*-----------------------------------------------------------------
Function: findFreeDataBlock

Global Variables:
   int minixfd - file descriptor of open fs.
   unsigned char *zmap - zone map

Returns: ERR1 (-1) - error encountered.
         Data block number.

Description: 
        Finds a free data block using bit map, sets the bit,
	and returns block number.
-----------------------------------------------------------------*/
int findFreeDataBlock()
{
   int bytenum, bitnum;
   int blocknum;

   // first find byte with a zero
   for(bytenum=0; bytenum<TOTALDATABLOCKS/8 ; bytenum++)
      if(zmap[bytenum] != 0xff) break;
   // find bit that is clear
   for(bitnum=0 ; bitnum<8 ; bitnum++)
      if((zmap[bytenum] & (1<<bitnum))==0) break;
   // Compute and check block number
   blocknum = bytenum*8 + bitnum + FIRSTZONE;
   if(blocknum > TOTALBLOCKS)
   {
      fprintf(stderr,"No free data blocks\n");
      blocknum = ERR1;
   }
   else
   {
      // set the bit
      bitnum = 1<<bitnum;
      zmap[bytenum] |= bitnum;
   }
   return(blocknum);
}

/*-----------------------------------------------------------------
Function: getDataBlock(i, ino, datablk)

Parameters: i - number of data block to read.
            ino - pointer to inode structure
            datablk - pointer to buffer for loading data block

Global Variables:
   int minixfd - file descriptor of open fs.

Description: Load the ith data block found in the file (directory)
             referenced by the inode "ino".  The data block
             is loaded into the buffer referenced by "datablk".
             The size of the block is given by BLOCK_SIZE.

Returns: ERR1 - error encountered.
         OK - Data block loaded.
-----------------------------------------------------------------*/
int getDataBlock(int i, struct minix_inode *ino, char *datablk)
{  
    if(seekToDataBlock(i,ino) == ERR1) 
    {
       perror("getDataBlock");
       return(ERR1);
    }
    if(read(minixfd,datablk,BLOCK_SIZE) != BLOCK_SIZE) 
    {
       perror("getDataBlock");
       return(ERR1);
    }
    return(OK);
}


/*-----------------------------------------------------------------
Function: writeDataBlock

Parameters: blockNum - block number
            datablk - pointer to buffer for saving data block

Global Variables:
   int minixfd - file descriptor of open fs.

Description: Save data into the data block identified by the block number.
             The data block in the buffer "datablk" is saved.
             The size of the block is given by BLOCK_SIZE.

Returns: ERR1 - error encountered.
         OK - Data block written.
-----------------------------------------------------------------*/
int writeDataBlock(int blockNum, char *datablk)
{
    int retcd = OK; 
    if(lseek(minixfd,(blockNum*BLOCK_SIZE),SEEK_SET) == -1) 
    {
       perror("writeDataBlock");
       retcd = ERR1;
    }
    else if(write(minixfd,datablk,BLOCK_SIZE) != BLOCK_SIZE)
    {
       perror("writeDataBlock");
       retcd = ERR1;
    }
    return(retcd);
}

/*-----------------------------------------------------------------
Function: saveDataBlock(i, ino, datablk)

Parameters: i - number of data block to save.
            ino - inode structure
            datablk - pointer to buffer for saving data block

Global Variables:
   int minixfd - file descriptor of open fs.

Description: Save data into the ith data block found in the file (directory)
             referenced by the inode "ino".  The data block in the
             buffer "datablk" is saved.
             The size of the block is given by BLOCK_SIZE.

Returns: ERR1 - error encountered.
         OK - Data block saved.
-----------------------------------------------------------------*/
int saveDataBlock(int i, struct minix_inode *ino, char *datablk)
{
    int retcd = OK; 
    if(seekToDataBlock(i,ino) == ERR1)
    {
       perror("saveDataBlock");
       retcd = ERR1;
    }
    else if(write(minixfd,datablk,BLOCK_SIZE) != BLOCK_SIZE)
    {
       perror("saveDataBlock");
       retcd = ERR1;
    }
    return(retcd);
}

/*-----------------------------------------------------------------
Function: seekToDataBlock(i, ino)

Parameters: i - number of data block to read.
            ino - inode structure

Global Variables:
   int minixfd - file descriptor of open fs.

Description: Seek to the ith data block found in the file (directory)
             referenced by the inode "ino". 
             The size of the block is given by BLOCK_SIZE.

Returns: ERR1 - error encountered.
         OK - Seek completed.
-----------------------------------------------------------------*/
int seekToDataBlock(int i, struct minix_inode *ino)
{
    short indexblock[BLOCK_SIZE/2];  /* integer array - contained in single block */
    int retcd = OK;
    if(i<7)  
    {
        if(lseek(minixfd,(ino->i_zone[i]*BLOCK_SIZE),SEEK_SET) == -1) 
	{
	   perror("seekToDataBlock");
	   retcd = ERR1;
	}
    }
    else if(i<(7+BLOCK_SIZE/2))  /* use indirect block */
    {
       if(lseek(minixfd,(ino->i_zone[7]*BLOCK_SIZE),SEEK_SET) == -1)
       {
	   perror("seekToDataBlock");
	   retcd = ERR1;
       }
       else if(read(minixfd,indexblock,BLOCK_SIZE) != BLOCK_SIZE)
       {
	   perror("seekToDataBlock");
	   retcd = ERR1;
       }
       /* lets find data block in the array */ 
       else if(lseek(minixfd,(indexblock[i-7]*BLOCK_SIZE),SEEK_SET) == -1)
       {
	   perror("seekToDataBlock");
	   retcd = ERR1;
       }
    }
      /* double indirect block not implemented */
    return(retcd);
}


//************************************************************
// Functions to support Debugging
//************************************************************

/*-----------------------------------------------------------------
Function: printInode

Parameters: ino - pointer to an inode structure.

Returns: nothing.

Description: 
        Prints the contents of an inode.
-----------------------------------------------------------------*/
void printInode(struct minix_inode *ino)
{
   int i;

   printf("i_mode=%x, i_uid=%d, i_size=%d, i_time=%x(%u), i_gid=%d, i_nlinks=%d\n",
          ino->i_mode, ino->i_uid, ino->i_size, ino->i_time, ino->i_time, ino->i_gid, ino->i_nlinks );
   printf("i_zone:");
   for(i=0 ; i<9; i++) printf(" %d", ino->i_zone[i]);
   printf("\n");
   fflush(stdout);
}

