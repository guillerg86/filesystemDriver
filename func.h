/****************************************************************
*	@File:		func.h
*	@Author: 	Guille Rodríguez 	LS26151
*	@Date:		2014/2015
****************************************************************/

#ifndef FUNC_H_
#define FUNC_H_


// INCLUDES
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 	// Para los exit
#include <unistd.h>		// Para los lseek
#include <sys/types.h>	// Para los lseek
#include <fcntl.h>		// Para los O_RDONLY
#include <ctype.h>		// Para la funcion toLower
#include <time.h>		// Para la funcion ctime

// CONSTANTS
#define SCREEN 1
#define KEYBOARD 0
#define TRUE 0
#define FALSE 1
#define FILE_ERROROPEN -1
#define BUFFSIZE 255

#define PARAM_INFO 10
#define PARAM_COPY 11
#define PARAM_FIND 12
#define PARAM_ABOUT 20
#define PARAM_CAT 13
#define PARAM_UNKW -1

#define FAT_DIR_LENGTH 32
#define FAT_DIR_MAXENTRIES 512

#define FILE_NOT_FOUND 0

#define DATA_TYPE_FILE	1
#define DATA_TYPE_DIR	2
#define DATA_TYPE_NOTFOUND 0

#define PART_TYPE_FAT16 100
#define PART_TYPE_EXT2	200
#define PART_TYPE_UNKW	-1

#define EXT_SUPERBLOCK_START_POS 1024
#define EXT_SUPERBLOCK_SIZE 1024
#define EXT_INODE_SIZE 128
#define EXT_DIRECT_BLOCK_POINTERS 12


// STRUCTS
typedef struct {
	char volume_name[17];					// s_volume_name ( 16bytes )
	char volume_type[5];					// s_magic
	unsigned int inode_size;				// s_inode_size
	unsigned int inode_total;				// s_inodes_count
	unsigned int inode_first;				// s_first_ino
	unsigned int inode_group;				// s_inodes_per_group
	unsigned int inode_free;				// s_free_inodes_count
	unsigned int block_total;				// s_blocks_count
	unsigned int block_reserved;			// s_r_blocks_count
	unsigned int block_free;				// s_free_blocks_count
	unsigned long block_size;				// s_log_block_size
	unsigned int block_first;				// s_first_data_block
	unsigned int block_group;				// s_blocks_per_group
	unsigned int frags_group;				// s_frags_per_group
	time_t last_check;						// s_lastcheck
	time_t last_mount;						// s_mtime
	time_t last_write;						// s_wtime
} Ext;

typedef struct {
	char volume_type[8];				
	char volume_name[8];
	char volume_label[12];
	unsigned char fat_tables;
	unsigned short int sectors_size;
	unsigned char sectors_cluster;
	unsigned short int sectors_reserved;
	unsigned short int sectors_maxrootentr;
	unsigned short int sectors_perfat;

} Vfat;



typedef struct {
	unsigned long i_mode;
	unsigned long i_uid;
	unsigned long i_size;
	unsigned long i_atime;
	unsigned long i_ctime; 
	unsigned long i_mtime;
	unsigned long i_dtime;
	unsigned long i_gid;
	unsigned long i_links_count;
	unsigned long i_blocks;
	unsigned long i_flags;
	unsigned long i_osd1;
	unsigned long i_block[15];
	unsigned long i_generation;
	unsigned long i_file_acl;
	unsigned long i_dir_acl;
	unsigned long i_faddr;
	unsigned long i_osd2;
} InodeEntry;


typedef struct {
	char name[12];
	char ext[4];
	unsigned char attr;
	unsigned short int reserved;
	unsigned short int time;
	unsigned short int date;
	unsigned short int firstblock;
	unsigned int size;
	unsigned long extTime;
} FatDirectory;
typedef struct {
	unsigned int inode;
	unsigned short int rec_len;
	unsigned char name_len;
	unsigned char file_type;
	char file_name[BUFFSIZE];	// De momento lo haremos sin mallocs.
	
} ExtDirectory;

// DEFINICION DE FUNCIONES

// Logic functions
void loadPartitionInfo(char *);
unsigned int findFileinVolume(char *, char *, int );
int checkPartType(int);
int execMode(char *);

// Helper functions
void strToLower(char *);
void strToUpper(char *);

// Print functions
void printExt2Info(Ext);
void printFatInfo(Vfat);
void printMsg(char *);
void printlnMsg(char *);
void printAbout(void);
void printAuthor(void);
void printErrNoParams(void);
void printErrFileopen(char *);
void printErrHeader(void);
void printErrFooter(void);
void printErrCopy(void);
void printErrUnknowPartition(void);
void printHelp(void);
void printFileNotFound(void);
void printDirectoryFound(void);
void printFatFileInfo(int, int, Vfat, int );
void printExtFileInfo(int, Ext, unsigned long, int);
void printFatNoSpaceForFile(void);
void printFatNoFreeRootDirectory(void);
void printErrCopyItsDirectory(void);


// FASE 1
Vfat loadFatInfo(int);
Ext loadExtInfo(int);

// FASE 2
void calculateFatStartDataBlock(Vfat fat);
int findInFat(int, Vfat, unsigned char *, char *);
unsigned int getFatStartRootDirectory(Vfat fat);
int fatStrLen(char *);
unsigned long findInExt(int, Ext , unsigned char *, char *);
unsigned long getInodeTableSeek(int , Ext );
ExtDirectory loadDirectoryEntry(int, unsigned long);
InodeEntry loadInodeData(int, unsigned long);
unsigned long extSearchInSubfolder(int, Ext, char *, char *, unsigned int);
unsigned long getInodeSeekPosition(int, Ext, unsigned int );

// FASE 3
void extShowContent(int, Ext , unsigned int );
void readChar (int , unsigned long , char *);


// FASE 4
FatDirectory loadDirectoryEntryFat(int,unsigned int );
FatDirectory newFatDirectoryEntry(void);
void copyFileFromExtToFat(char *,char *, char *);
int searchFreeRootDirInFat(int, Vfat);
int calculateNecesaryClusters(unsigned long , int );
unsigned long fatTableStartSeek(Vfat);
char* getExtensionFromExtFilename(char *);
unsigned long getFatFirstDataSector(Vfat);
void fatShowContent(int , Vfat , int );
void mapFatClustersInArray(int , FatDirectory , int[] , int , Vfat );
int mapFatClustersFree(int , int mappingArray[], int mapArraySize, Vfat );
FatDirectory convertFromExtToFatDirEntry(ExtDirectory , InodeEntry );
int fatSaveEntryInRooTDirectory(int , Vfat , FatDirectory , int );
int readBlock (int , unsigned long , char *, unsigned long, unsigned long, unsigned long );




#endif 
