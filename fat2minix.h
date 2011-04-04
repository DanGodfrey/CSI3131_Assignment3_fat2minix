/*-----------------------------------------------------------------
File: fat2minix.h
Description: Contains definitions and include files for the 
             fat2minix project.
------------------------------------------------------------------*/

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
