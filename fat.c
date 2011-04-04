/*-----------------------------------------------------------------
File: fat.c
Description: This file contains code for reading structures from
             the FAT file system. Some functions are provided
	     to display the contents of these structures for
	     debugging purposes.
------------------------------------------------------------------*/

#include "fatDefn.h"
#include "errno.h"
#include <ctype.h>

/* some global data */
struct fat_boot_sector fbs;  // FAT Boot Sector
unsigned short *fatPtr;  // pointer to the FAT Table
int fatfd;  // File descriptor for FAT file system

// Prototypes of local functions
void removeTrailingSpace(char *);

/*-----------------------------------------------------------------
Function: readFatBoot(fd)

Parameters: int fd - file descriptor of open file system 

Description: Displays all data from the boot sector
             Also fills the structure fbs (global).
             
	     There are a number of programming subtleties used
	     to get values from the structure.  
	     For example the member sector_size in "struct fat_boot_sector"
	     is declared as an array of type u8 with 2 elements.  To get
	     an integer value of 2 bytes long, must somehow obtain a short
	     value. Given that fbs.sector_size represents the ADDRESS of 
	     the array, the trick is to translate this address to 
	     an address of type short with a cast as follows:
	               (short *) fbs.sector_size
	     But we want the short value, so we add the indirection
	     operator to get:
	               *(short *) fbs.sector_size
             The same must be done for dir_entries and sectors:
                       *(short *) fbs.dir_entries
                       *(short *) fbs.sectors)
             The key here is to remember that in C, the name of
	     an array produces an address. Note that these
	     expressions are also used in the "defines" found at the
	     start of the file to create symbolic constants.

	     Also calls readFatTable to read the Fat table.  Saves
	     the fd for other functions.

-----------------------------------------------------------------*/
int readFatBoot(int fd)
{
     int n;
     char string[BUFSIZ];  // creates large buffer

     fatfd = fd;  // save for other functions.
     if(lseek(fd, 0, SEEK_SET)==-1) perror("readFatBoot");  // move to start of FS
     n = read(fd,&fbs,sizeof(struct fat_boot_sector)); // reads in the boot sector
     if(n != sizeof(struct fat_boot_sector))
     {
         printf("Could not read bootsector (%d,%d)\n",n,sizeof(struct fat_boot_sector));
	 return(ERR1);
     }
     /* Printout the contents */
    printf("------------Boot Sector - FAT 16--------------\n");
    strncpy(string, fbs.system_id, 8);
    printf("System id: %s\n",string);
    printf("Sector Size: %hd\n", *(short *) fbs.sector_size); // 2 byte int
    printf("Cluster Size: %hhd\n", fbs.cluster_size); // 1 byte int
    printf("Nmber of Reserved Sectors: %hd\n", fbs.reserved); // 2 byte int
    printf("Number of FATs: %hhd\n", fbs.fats); // 1 byte int
    printf("Max Number of Root Directory Entries: %hd\n", *(short *) fbs.dir_entries); // 2 byte int
    printf("Total number of sectors: %hd\n", *(short *) fbs.sectors); // 2 byte int
    printf("Media code: %hhx\n", fbs.media); // 1 byte hex
    printf("Number of sectors per FAT: %hd\n",  fbs.fat_length); // 2 byte int
    printf("Number of sectors per track: %hd\n",  fbs.secs_track); // 2 byte int
    printf("Number of heads: %hd\n", fbs.heads); // 2 byte int
    printf("Total number of sectors(if previous is 0): %d\n", fbs.total_sect); // 4 byte int
    printf("-----------------------------------------\n\n");
    // Also read the FAT table
    return(readFatTable());
}
/*-----------------------------------------------------------------
Function: readFatTable

Parameters: None

Description: Reads in the FAT table.  Also
             allocates memory using malloc for saving the FAT table.
             Note that the FAT table is represented
             as an array of short's, that is 2 byte integers.
             It is assumed that fbs has been setup, i.e.
	     a call to readFatBoot has been made.
-----------------------------------------------------------------*/
int readFatTable( )
{
   // FAT Tables contain two byte entries
   // Setup config info from boot sector
   int sectorSize = (*(short *)fbs.sector_size); // size in bytes
   int fatSize = fbs.fat_length * sectorSize; // size in bytes
   fatPtr = (unsigned short*) malloc(fatSize); // allocates memory for FAT Table
   if(fatPtr == NULL)
   {
      perror("malloc");
      return(ERR1);
   }
   if(lseek(fatfd, sectorSize, SEEK_SET)==-1) perror("readFatTable"); // seek to first FAT table
   if(read(fatfd, fatPtr, fatSize)==-1) perror("readFatTable");  // Reads in the FAT table from the disk
   return(OK);
}

/*-----------------------------------------------------------------
Function: saveFatTable

Parameters: None

Description: Saves the the FAT table in the file system (hard drive).
-----------------------------------------------------------------*/
int saveFatTable( )
{
   // FAT Tables contain two byte entries
   int i;
   // Setup config info from boot sector
   int sectorSize = (*(short *)fbs.sector_size); // size in bytes
   int fatSize = fbs.fat_length * sectorSize; // size in bytes
   int numFats = fbs.fats; // number of FAT tables - usually 2
   /* time to write the FATs */
   for(i=0 ; i < numFats ; i++)
   {
      if(lseek(fatfd, sectorSize+i*(fbs.fat_length * sectorSize), SEEK_SET)==-1) 
          perror("saveFatTable");
      if(write(fatfd, fatPtr, fatSize)==-1) perror("saveFatTable");  // Reads in the FAT table from the disk
   }
   return(OK);
}
/*-----------------------------------------------------------------
Function: openFatDirectory

Parameters: char *dirname - name of directory to open

Description: Finds a FAT Directory Table on the FAT file system
             (hard drive) and reads it into a FATDIR structure 
	     (see fat.h). Note that memory is allocated
	     for storing the contents of the structure and the
	     directory table (in the "table" member of the 
	     structure).
-----------------------------------------------------------------*/
FATDIR *openFatDirectory(char *dirname)
{
   FATDIR *dirTablePtr;  // Pointer to structure of FAT directory
   /* allocate memory for structure */
   dirTablePtr = malloc(sizeof(FATDIR));
   if(dirTablePtr==NULL) { perror("openFatDir"); return(NULL); }
   /* allocate memory for directory table */
   /* Assume all directory tables are no longer than a single cluster */
   dirTablePtr->table = malloc(CLUSTER_SIZE);
   /* Find the directory */
   if(getFatDirTable(dirname, dirTablePtr) == OK)
   {
      strcpy(dirTablePtr->name, dirname); // replace with full name
   }
   else
   {
      printf("Could not find FAT Directory %s\n",dirname);
      free(dirTablePtr->table);
      free(dirTablePtr);
      dirTablePtr = NULL;
   }
   return(dirTablePtr);
}
/*-----------------------------------------------------------------
Function: closeFatDirectory

Parameters: FATDIR *ptr 

Description: Writes FAT directory table to disk and releases memory.
-----------------------------------------------------------------*/
void closeFatDirectory(FATDIR *ptr)
{
   writeCluster(ptr->clusterNum, ptr->table, "closeFatDirectory");
   saveFatTable();  // save any changes made to the FAT Table
   free(ptr->table);
   free(ptr);
}
/*-----------------------------------------------------------------
Function: getFatDirTable

Parameters: char *path  - full path name of directory table to find
	    FATDIR *dirTablePtr - pointer to location for loading the table

Description: Finds the directory table and loads it into the FATDIR
             structure.  Retures OK if all went well and ERR1 upon
	     detection of an error. If the directory is not the
	     root directory, the recursive function scanSubDirectories
	     is called to find it.
-----------------------------------------------------------------*/
int getFatDirTable(char *path, FATDIR *dirTablePtr)
{
   int retcd;
   // allocate memory to store root directory
   struct msdos_dir_entry *rootdir = (struct msdos_dir_entry *) malloc(CLUSTER_SIZE); 
   if(rootdir == NULL)
   {
      perror("getFatDirTable");
      retcd = ERR1;
   }
   else
   {
      // Read in root directory
      readCluster(0,rootdir,"getFatDirTable");
      // Loop through the root directory table
      if(strcmp(path, "/") == 0)
      {
	 dirTablePtr->clusterNum = 0;
	 dirTablePtr->parentCluster = 0;
         dirTablePtr->size = CLUSTER_SIZE;
         dirTablePtr->numEntries = dirTablePtr->size/sizeof(struct msdos_dir_entry);
         memcpy(dirTablePtr->table, rootdir, CLUSTER_SIZE);  // Copy root directory table
         retcd = OK; 
      }
      else  // the following is a recursive function
         retcd = scanSubDirectories(path+1, rootdir, dirTablePtr, 0);   // name+1: skip "/"
      free(rootdir);  // frees allocated memory
   }
   return(retcd);
}
/*-----------------------------------------------------------------
Function: scanSubDirectories

Parameters: char *path - name of the subdirectory (with no prefix)
            struct msdos_dir_entry *tbl - pointer to a directory 
	                                  table (array of entries)
            FATDIR *dirTablePtr - pointer to memory 
	                    to store directory table being searched

Description: Scanning to find directory table. This is a recursive
             function. Fills in the memory pointed by dirTablePtr
	     with the directory table named "path".  Returns the
	     clusterNum where the directory table is stored.
-----------------------------------------------------------------*/
int scanSubDirectories(char *path, struct msdos_dir_entry *tbl, 
		       FATDIR *dirTablePtr, unsigned short parentCluster)
{
   int numSubDirEntries = CLUSTER_SIZE/sizeof(struct msdos_dir_entry);
   struct msdos_dir_entry subDirTable[numSubDirEntries]; // for subdirectories
   char subDirName[BUFSIZ]; // for loading in the subdirectory table
   char *pt=subDirName; // pointer to copy name
   int ix; // index for seaching through table
   int retcd;  // for return code

   // Get name to search in directory table (and remove from head of path)
   while(*path!='/' && *path!='\0') *pt++=*path++;
   *pt='\0';  // terminates string
   if(*path == '/') path++; // skips the '/'
   
   // Search for sub directory
   for(ix = 0 ; ix < numSubDirEntries ; ix++)
   {
      if( (tbl[ix].attr & ATTR_DIR) && 
          (strncmp(subDirName, tbl[ix].name, strlen(subDirName)) == 0)) // found it
      {
         if(*path == '\0') // if at end of path, then found directory 
	 {
	    dirTablePtr->clusterNum = tbl[ix].start;  // this is where its stored
            dirTablePtr->size = CLUSTER_SIZE;
            dirTablePtr->numEntries = dirTablePtr->size/sizeof(struct msdos_dir_entry);
	    dirTablePtr->parentCluster = parentCluster;
	    strcpy(dirTablePtr->name,path);
	    retcd = OK;
	    readCluster(dirTablePtr->clusterNum, dirTablePtr->table, "scanSubDirectories");
            break;  // leave the loop
	 }
	 else // otherwise need to find next subdirectory in path
	 {
	    readCluster(tbl[ix].start, subDirTable, "scanSubDirectories");
            retcd = scanSubDirectories(path, subDirTable, dirTablePtr, tbl[ix].start);
            break;  // leave the loop
	 }
      }
   }
   if(ix == numSubDirEntries) // did not find the name in the table
   {
      printf("Could not find subdirectory %s\n", subDirName);
      retcd = ERR1;
   }
   return(retcd);
}

/*-----------------------------------------------------------------
Function: printFatDirEntries

Parameters: int fd - file descriptor to file system

Description: Displays the contents of the directory table.
             Note that a pointer variable can be used like an
             array name when it points to an array.
             Note also that pointer arithment allows you to compute
             the address of an element in an array. Thus,
             dirTblPtr+i gives the address of the ith element in the
             array referenced by dirTblPtr (note that dirTblPtr[i] 
             gives the value of the element, in this case the structure,
             it is equivalent to *(dirTblPtr+i).
-----------------------------------------------------------------*/
void printFatDirEntries(FATDIR *tPtr)
{
    int i;
    printf("Here are the contents of FAT directory %s\n",tPtr->name);
    // Loop through the directory table
    for(i = 0 ; i < tPtr->numEntries ; i++)
    {
       if(tPtr->table[i].name[0]==(char)0x00) { }   // available
       else if(tPtr->table[i].name[0]==(char)0x05) { }   // deleted
       else if(tPtr->table[i].name[0]==(char)0xE5) { }   // deleted
       else if(tPtr->table[i].name[0]==(char)0x2E)  // dot or dotdot
	         displayFatDirEntry(tPtr->table+i);
       else 
	  displayFatDirEntry(tPtr->table+i);  // valid entry
    }
}

/*-----------------------------------------------------------------
Function: displayFatDirEntry(struct msdos_dir_entry *de)

Parameters: struct msdos_dir_entry *de - pointer to a directory entry

Description: Displays the contents of the directory entry
-----------------------------------------------------------------*/
void displayFatDirEntry(struct msdos_dir_entry *de)
{
   char str1[BUFSIZ], str2[BUFSIZ];  // working buffers
   char name[100];  // Dos name
   // Get the DOS name - leave spaces in
   strncpy(str1,de->name,8); str1[8]='\0'; // copies 8 characters
   strncpy(str2,de->ext,3); str2[3]='\0'; // copies 3 characters
   sprintf(name,"%s.%s",str1, str2);
   printf("Directory Entry: %s\n", name);
   // Other attributes of directory entry
       /* Attribute bits */
   printf("   Attribute bits: (%02x)", de->attr);
   if(de->attr&ATTR_RO) printf(" Read only");
   if(de->attr&ATTR_HIDDEN) printf(", Hidden");
   if(de->attr&ATTR_SYS) printf(", System");
   if(de->attr&ATTR_VOLUME) printf(", Volume label");
   if(de->attr&ATTR_DIR) printf(", Subdirectory");
   if(de->attr&ATTR_ARCH) printf(", Archive");
   // Ignore the other bits
   printf("\n");
       /* Case */
   printf("   Case information: %x\n", de->lcase);
       /* Dates and times >> is the shift bit right operation */
   printf("   Create date/time (ms): %02d:%02d:%02d  /  ", 
                        (1980+(de->cdate>>9)),
			((de->cdate&0x01e0)>>5),
			(de->cdate&0x001f));
   printf("%02d:%02d:%02d (%d)\n", (de->ctime>>11), 
                             ((de->ctime&0x07ef)>>5), 
                             (de->ctime&0x001f), 
                             de->ctime_ms);
   printf("   Last access date: %02d:%02d:%02d\n", 
                        (1980+(de->adate>>9)),
			((de->adate&0x01e0)>>5),
			(de->adate&0x001f));
   printf("   Last modified date/time: %02d:%02d:%02d  / ", 
                        (1980+(de->date>>9)),
			((de->date&0x01e0)>>5),
			(de->date&0x001f));
   printf("%02d:%02d:%02d\n", (de->time>>11), 
                             ((de->time&0x07ef)>>5), 
                             (de->time&0x001f)); 
       /* content information */
   printf("   Start Cluster: %x, Size: %d\n", de->start, de->size);
}

/*-----------------------------------------------------------------
Routine: printFatTable

Arguments: unsigned short *fatTblPtr;

Description: Display the contents of the FAT table.
-----------------------------------------------------------------*/
void printFatTable(unsigned short *fatTblPtr, int numEntries) 
{
   // FAT Tables contain two byte entries
   int i;
   char str1[BUFSIZ];
   char line[BUFSIZ];
   char lastline[BUFSIZ];
   int flag; 

   // print the contents of the tables
   *lastline = '\0';
   flag = TRUE;
   printf("------------Fat Table: \n");
   for(i=0 ; i<numEntries; i++)
   {
	 if((i+1)%16 != 0)
	 {
	    sprintf(str1," %04x",fatTblPtr[i]);
	    strcat(line,str1);  // builds a line to print
	 }
	 else 
	 {
	    sprintf(str1," %04x",fatTblPtr[i]);
	    strcat(line,str1);
	    if(strcmp(line,lastline)) // print line only if different from last
	    {
	       printf("%04x: %s\n",i-15,line);
	       strcpy(lastline,line);
	       *line = '\0';  // erases last line
	       flag = TRUE;
	    }
	    else if(flag) // prints only one * for duplicates
	    { 
	       *line = '\0';  // erases last line
	       printf("*\n"); 
	       flag=FALSE;
	    }
	    else // duplicate line
	       *line = '\0';  // erases last line
	 }
   }
   printf("------------End Fat Table----------------\n");
}

/*-----------------------------------------------------------------
Function: readCluster   writeCluster

Parameters:  clusterNum - the number of the cluster to read/write
	     buffer - pointer to memory location into which 
                      data read is stored or data written is located 
	     errStr - string to be include in error messages 
	              (typically the name of the calling function)

Description: Reads (readCluster) a cluster from memory or
             writes (writeCluster) a cluster to memory.
-----------------------------------------------------------------*/
void readCluster(int clusterNum, void *buffer, char *errStr)
{
   char errorString[BUFSIZ];
   *(char *)buffer = '\0';  // set to null char
   sprintf(errorString,"readCluster (from %s)",errStr);
   if(clusterNum == 0) // seek to root directory
   {
      if(lseek(fatfd, ROOTDIR_POS, SEEK_SET)==-1)
           perror(errorString); // seek to directory table
   }
   else
   {
      if(lseek(fatfd, DATA_POS + (clusterNum-2)*CLUSTER_SIZE, SEEK_SET)==-1)
           perror(errorString); // seek to directory table
   }
   if(read(fatfd, buffer, CLUSTER_SIZE)==-1) 
       perror(errorString); // seek to directory table
}

void writeCluster(int clusterNum, void *buffer, char *errStr)
{
   char errorString[BUFSIZ];
   sprintf(errorString,"writeCluster (from %s)",errStr);
   if(clusterNum == 0) // seek to root directory
   {
      if(lseek(fatfd, ROOTDIR_POS, SEEK_SET)==-1)
           perror(errorString); // seek to directory table
   }
   else
   {
      if(lseek(fatfd, DATA_POS + (clusterNum-2)*CLUSTER_SIZE, SEEK_SET)==-1)
           perror(errorString); // seek to directory table
   }
   if(write(fatfd, buffer, CLUSTER_SIZE)==-1) 
   {
       perror(errorString); // seek to directory table
   }
}

/*-----------------------------------------------------------------
Function: getFatName

Parameters:  struct msdos_dir_entry *fatDir - Fat Directory Entry.
             char *name;  pointer to buffer to fill with name.

Returns:  char * - address passed in name.

Description: Extracts name from fat directory and removes space
             padding.
-----------------------------------------------------------------*/
char *getFatName(struct msdos_dir_entry *fatDir, char *name)
{
   char buffer[100];  // working buffer
   char *pt;  // pointer to traverse the name
   // Let's create the name
   *name = '\0';  // empty the string
   memcpy(buffer,fatDir->name,8); 
   buffer[8]='\0'; // copies 8 characters
   removeTrailingSpace(buffer);
   strcat(name,buffer);
   memcpy(buffer,fatDir->ext,3); 
   buffer[3]='\0'; // copies 3 characters
   removeTrailingSpace(buffer);
   if(*buffer != '\0')
   {
      strcat(name,".");
      strcat(name,buffer);
   }
   // convert uppercase to lower case
   for(pt=name ; *pt!='\0' ; pt++)  *pt = tolower(*pt); 
   // All done
   return(name);
}

/*-----------------------------------------------------------------
Function: removeTrailingSpaces

Parameters:  char *str - pointer to string

Description: Removes any trailing spaces in the string, if
             string contains only spaces, string is emptied.
-----------------------------------------------------------------*/
void removeTrailingSpace(char *str)
{
   char *pt=str;
   // find end of string
   while(*pt) pt++;  // *pt not '\0'
   pt--; // point to last char
   while(*pt==' ' && pt>=str) // stop if reaches start of string
   {
     *pt = '\0';  // remove space
     pt--;
   }
}

/*-----------------------------------------------------------------
Function: getDate, getTime, getmsTime

Parameters:  time_t time - UNIX time value ( number  of  seconds  elapsed
             since 00:00:00 on January 1, 1970, Coordinated Universal Time (UTC))

Description: Converts UNIX time to date and time format for storing
             into FAT directory entry.  Note that for milli-second
	     value provide only 100ms or zero.
-----------------------------------------------------------------*/
unsigned short getDate(time_t time)
{
  struct tm *t;
  t = localtime(&time);

  return( (t->tm_year-80)<<9 | (t->tm_mon+1)<<5 | t->tm_mday); // year = tm_year+1900-1980
}

unsigned short getTime(time_t time)
{
  struct tm *t;
  t = localtime(&time);
  return( t->tm_hour<<11 | t->tm_min<<5 | t->tm_sec/2);
}

char getmsTime(time_t time)
{
  struct tm *t;
  t = localtime(&time);
  return( 100*(t->tm_sec%2) ); // 100 * 10ms or zero
}

