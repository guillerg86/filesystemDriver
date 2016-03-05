/****************************************************************
*	@File:		func.c
*	@Author: 	Guille Rodríguez 	LS26151
*	@Date:		2014/2015
****************************************************************/
#include "func.h"
#include <stdint.h>

/**********************************************
* @Nombre: 	execMode
* @Def: 	Funcion encargada de mirar el parametro que se le pasa al programa
* @Arg:	 	In: *arg: Recibe el texto del segundo parametro. 
* @Ret:  	Devuelve un integer indicando el modo de ejecución al programa principal
**********************************************/
// LOGIC FUNCTIONS
int execMode(char *arg) {
	
	char option[BUFFSIZE];
	bzero(option,BUFFSIZE);
	
	strcpy(option,arg);
	strToUpper(option);
	
	if ( strcmp(option,"/INFO") == 0 ) {
		// Entering in SHOW INFO MODE
		return PARAM_INFO;
	}
	if ( strcmp(option,"/ABOUT") == 0) {
		// Entering in ABOUT MODE		
		return PARAM_ABOUT;
	} 
	if ( strcmp(option,"/FIND") == 0) {
		// Entering in FIND MODE
		return PARAM_FIND;
	}
	if ( strcmp(option,"/CAT") == 0 ){
		return PARAM_CAT;
	}
	if ( strcmp(option,"/COPY") == 0) {
		return PARAM_COPY;
	}
	
	return PARAM_UNKW;
	
}
// FUNCIONES FASE 4
/**********************************************
* @Nombre: 	updateFatTable
* @Def: 	Funcion encargada de actualizar la tabla FAT del sistema de archivos
* @Arg:	 	In: int fdFat - Filedescriptor del fichero FAT
* @Arg:	 	In: Vfat struct - Estructura con los datos de la Vfat
* @Arg:	 	In: int clusters_to_update[] - Array de clusters y entradas de Fat libres
* @Arg:	 	In: int cluster_array_size - Tamaño del array
**********************************************/
void updateFatTable(int fdFat, Vfat fatinfo, int clusters_to_update[], int cluster_array_size) {
	unsigned long fatTableSeek = fatTableStartSeek(fatinfo);
	unsigned long offset = fatTableSeek;
	unsigned short int finalBlock = 0xFFFF;
	lseek(fdFat,fatTableSeek,SEEK_SET);
	if ( cluster_array_size > 1 ) {
		int i = 0;
		for (i = 1; i<cluster_array_size; i++) {
			// Ir a la posicion de la fat table indicada
			offset = fatTableSeek + (clusters_to_update[i-1] * 2);
			lseek(fdFat,offset,SEEK_SET);
			write(fdFat,&clusters_to_update[i],sizeof(unsigned short int));
		} 
		offset = fatTableSeek + (clusters_to_update[(cluster_array_size - 1)] * 2);
		lseek(fdFat,offset,SEEK_SET);
		write(fdFat,&finalBlock,sizeof(unsigned short int));
		// El ultimo lo marcamos como FFFF indicando que ese es el ultimo bloque de datos para el archivo indicado.
	} else if ( cluster_array_size == 1 ){
		offset=fatTableSeek + (clusters_to_update[0] * 2);
		lseek(fdFat,offset,SEEK_SET);
		write(fdFat,&finalBlock,sizeof(unsigned short int));
	}
}
/**********************************************
* @Nombre: 	writeFat
* @Def: 	Escribe el contenido del array que se le pasa, al cluster de fat.
* @Arg:	 	In: int fdFat - Filedescriptor del fichero FAT
* @Arg:	 	In: Vfat struct - Estructura con los datos de la Vfat
* @Arg:	 	In: char caracter[] - Array que contiene la informacion a guardar en la Fat
* @Arg:	 	In: int clusternumber - Contiene el numero del cluster donde guardar la información 
* @Arg:	 	In: int bytes_to_write - Contiene los bytes a guardar
**********************************************/
void writeFat(int fdFAT, Vfat fatinfo, char caracter[], int clusternumber, int bytes_to_write ) {
	// Calcular la posicion del firstSectorOfCluster
	unsigned long firstSectorOfCluster = 0;
	unsigned long offset = 0;
	firstSectorOfCluster = ( ( clusternumber - 2 ) * fatinfo.sectors_cluster ) + getFatFirstDataSector(fatinfo);
	offset = firstSectorOfCluster * fatinfo.sectors_size;
	//printf("Guardando en bloque offset: %lu \n",offset);
	lseek(fdFAT,offset,SEEK_SET);
	write(fdFAT,caracter,bytes_to_write);
}

/**********************************************
* @Nombre: 	readBlock
* @Def: 	Funcion encargada de leer un bloque de informacion de Ext
* @Arg:	 	In: int fd - Filedescriptor del fichero de sistema de archivos
* @Arg:	 	In: unsigned long offset - Offset donde posicionar el cursor para leer la informacion
* @Arg:	 	In: char *caracter - Array donde guardar la informacion
* @Arg:	 	In: unsigned long blocksize - Tamaño del bloque unsig long tamaño del bloque int array de clusters y entradas de la fat libres,
* @Arg:	 	In: unsigned long readedBytes - Bytes que llevamos leidos del fichero
* @Arg:	 	In: unsigned long filesize - Bytes totales del archivo 
* @Ret:  	Out: int nbytes 
**********************************************/
int readBlock (int fd, unsigned long offset, char *caracter, unsigned long blocksize, unsigned long readedbytes, unsigned long filesize) {
	unsigned long resta = filesize - readedbytes; 
	int bytes_to_read = 0;
	
	if ( resta >= blocksize) {
		bytes_to_read = blocksize;
	} else {
		bytes_to_read = resta;
	}
	
	int nbytes = 0;
	bzero(caracter,blocksize);
	lseek(fd,offset,SEEK_SET);
	nbytes = read(fd,caracter,bytes_to_read);
	return nbytes;
}

/**********************************************
* @Nombre: 	copyFileFromExtToFat
* @Def: 	Funcion encargada convertir una entrada de directory de Ext a Fat
* @Arg:	 	In: int fd - Filedescriptor del fichero de sistema de archivos
* @Arg:	 	In: unsigned long offset - Offset donde posicionar el cursor para leer la informacion
* @Arg:	 	In: char *caracter - Array donde guardar la informacion
* @Arg:	 	In: unsigned long blocksize - Tamaño del bloque unsig long tamaño del bloque int array de clusters y entradas de la fat libres,
* @Arg:	 	In: unsigned long readedBytes - Bytes que llevamos leidos del fichero
* @Arg:	 	In: unsigned long filesize - Bytes totales del archivo 
**********************************************/
void copyFileFromExtToFat(char *extFile,char *fatFile, char *filename) {
	Vfat fat;
	Ext ext;
	int fdExt, fdFat;
	int partType = PART_TYPE_UNKW;
	int freeFatRootDirEntry = -1;
	int clusterSize = 0; 
	int clustersNeeded = 0;
	int i =0;
	int boolResult = -1;
	
	ExtDirectory extdir;
	InodeEntry inode;
	
	char buffer[BUFFSIZE];
	bzero(buffer,BUFFSIZE);
	
	unsigned long seekExtDirEntry;
	unsigned char filetype = -1;	
	
	fdExt = open(extFile, O_RDONLY);
	if ( fdExt == FILE_ERROROPEN ) {
		printErrFileopen(extFile);
		exit(EXIT_FAILURE);
	}
	
	partType = checkPartType(fdExt);
	if ( partType != PART_TYPE_EXT2 ) {
		close(fdExt);
		printErrUnknowPartition();
		exit(EXIT_FAILURE);
	}
	
	partType = PART_TYPE_UNKW;
	fdFat = open(fatFile, O_RDWR);
	if ( fdFat == FILE_ERROROPEN ) {
		close(fdExt);
		printErrFileopen(fatFile);
		exit(EXIT_FAILURE);
	}
	
	partType = checkPartType(fdFat);
	if ( partType != PART_TYPE_FAT16 ) {
		close(fdExt);
		close(fdFat);
		printErrUnknowPartition();
		exit(EXIT_FAILURE);
	}
	// Los ficheros son correctos y del sistema de archivos que corresponde --> Cargamos la informacion de los sistemas de archivos
	fat = loadFatInfo(fdFat);
	ext = loadExtInfo(fdExt);
	
	clusterSize = fat.sectors_size * fat.sectors_cluster;
	
	if ( clusterSize == 0 ) {
		printf("Fat filesystem is not formatted correctly\n");
		close(fdExt); close(fdFat);
		exit(EXIT_FAILURE);
	}
	
	freeFatRootDirEntry = searchFreeRootDirInFat(fdFat, fat);
	
	if ( freeFatRootDirEntry != PARAM_UNKW ) {
		// Hay root directory entries libres.
		seekExtDirEntry = findInExt(fdExt,ext,&filetype,filename);
		
		if ( seekExtDirEntry != FILE_NOT_FOUND ) {
			if ( filetype == DATA_TYPE_FILE ) {
				// El fichero existe y ademas es un fichero!
				extdir = loadDirectoryEntry(fdExt, seekExtDirEntry);
				inode = loadInodeData(fdExt,getInodeSeekPosition( fdExt,ext, extdir.inode ) );
				
				clustersNeeded = calculateNecesaryClusters(inode.i_size,clusterSize);

				int clustersFree[clustersNeeded];
				// Limpiamos el array 
				for (i=0; i<clustersNeeded; i++) {
					clustersFree[i] = 0;
				}
				// Buscamos los clusters vacios	
				boolResult = mapFatClustersFree(fdFat,clustersFree,clustersNeeded,fat);
				if ( boolResult == 0) {
					// Aqui viene la magia negra!
					
					// Procedemos a convertir la entrada de EXT a FAT.
					FatDirectory newFatDir = convertFromExtToFatDirEntry(extdir, inode);
					if ( clustersNeeded == 0 ) {
						newFatDir.firstblock = 0xFFFF;
					} else {
						newFatDir.firstblock = clustersFree[0];	
					}
					
					strcpy(buffer,newFatDir.name);
					if ( strlen(newFatDir.ext) != 0 ) {
						strcat(buffer,".");
						strcat(buffer,newFatDir.ext);
					}
					
				
					// Comprobamos si no existe ya el archivo 
					if ( findInFat(fdFat, fat ,&filetype, buffer) == FILE_NOT_FOUND ) {
						// Guardarmos en la rootdirectory la informacion de esta newFatEntry
						fatSaveEntryInRooTDirectory( fdFat, fat,newFatDir,  freeFatRootDirEntry);
						
						// Comienzan los bucles infinitos hasta el coredumped y mas alla!!!
							int bytes_read = 0;
							int cluster_cursor = 0;
							unsigned long readedBytes = 0;
							unsigned long sumBlock = 0;
							unsigned long sumBlock2 = 0;
							unsigned long sumBlock3 = 0;
							unsigned long sumBlock4 = 0;
							unsigned long dirOffset = 0;
							unsigned long dirOffset2 = 0;
							unsigned long dirOffset3 = 0;
							unsigned long dirOffset4 = 0;
							unsigned long blockPointer1 = 0;
							unsigned long blockPointer2 = 0;
							unsigned long blockPointer3 = 0;
							
							char carac[ext.block_size];
							int boolContinueReading = 1;
							
							//printf("\tFile size: %lu \n",inode.i_size);
							
							
							int i=0;
							for (i =0; i<12; i++) {
								bzero(carac,ext.block_size);
								sumBlock = 0; dirOffset = 0;
								if ( inode.i_block[i] == 0 ) { boolContinueReading = 0; break;} 
								dirOffset = inode.i_block[i] * ext.block_size;
								do {
									bytes_read = readBlock(fdExt,dirOffset,carac,ext.block_size,readedBytes,inode.i_size); 
									writeFat(fdFat,fat, carac, clustersFree[cluster_cursor], bytes_read );
									cluster_cursor++; sumBlock += ext.block_size; dirOffset += ext.block_size; readedBytes += bytes_read;
								} while ( sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 );
							}
							
							if ( inode.i_block[12] != 0 ) {
								
								sumBlock = 0; dirOffset = inode.i_block[12] * ext.block_size;
								do {
									lseek(fdExt,dirOffset,SEEK_SET);
									read(fdExt,&blockPointer1,sizeof(unsigned int));
									
									if ( blockPointer1 == 0 ) { boolContinueReading = 0; break;}
									sumBlock2 = 0;
									dirOffset2 = blockPointer1 * ext.block_size;
									
									do {
										bytes_read = readBlock(fdExt,dirOffset2,carac,ext.block_size,readedBytes,inode.i_size);
										writeFat(fdFat,fat, carac, clustersFree[cluster_cursor], bytes_read );
										cluster_cursor++; readedBytes += bytes_read; sumBlock2+=ext.block_size; dirOffset2 +=ext.block_size;
									} while (sumBlock2 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);

									sumBlock 	+= 4;
									dirOffset 	+= 4;
								} while (sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);
							} 
							
							if ( inode.i_block[13] != 0 ) {
								sumBlock = 0; sumBlock2 = 0; sumBlock3 = 0; 
								dirOffset = inode.i_block[13] * ext.block_size; dirOffset2 = 0; dirOffset3 = 0;
								do {
									lseek(fdExt,dirOffset,SEEK_SET);
									read(fdExt,&blockPointer1,sizeof(unsigned int));
									
									if ( blockPointer1 == 0 ) { boolContinueReading = 0; break;}
									sumBlock2 = 0;
									dirOffset2 = blockPointer1 * ext.block_size;
									
									do {
										lseek(fdExt,dirOffset2,SEEK_SET);
										read(fdExt,&blockPointer2,sizeof(unsigned int));
										
										if ( blockPointer2 == 0 ) { boolContinueReading = 0; break;} 
										sumBlock3 = 0;
										dirOffset3 = blockPointer2 * ext.block_size;
										
										do {
											bytes_read = readBlock(fdExt,dirOffset3,carac,ext.block_size,readedBytes,inode.i_size);
											writeFat(fdFat,fat, carac, clustersFree[cluster_cursor], bytes_read );
											cluster_cursor++; readedBytes += bytes_read; sumBlock3 += ext.block_size; dirOffset3 += ext.block_size;
										} while (sumBlock3 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);
										
										sumBlock2	+=4; 
										dirOffset2 	+=4;
									} while (sumBlock2 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);

									sumBlock 	+= 4;
									dirOffset 	+= 4;
								} while (sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);
							}	
							
							if ( inode.i_block[14] != 0 ) {
								sumBlock = 0; sumBlock2 = 0; sumBlock3 = 0; 
								dirOffset = inode.i_block[14] * ext.block_size; dirOffset2 = 0; dirOffset3 = 0;
								do {
									lseek(fdExt,dirOffset, SEEK_SET);
									read(fdExt,&blockPointer1,sizeof(unsigned int));
									
									if ( blockPointer1 == 0 ) { boolContinueReading = 0; break; }
									sumBlock2 = 0;
									dirOffset2 = blockPointer1 * ext.block_size;
									
									do {
										lseek(fdExt, dirOffset2, SEEK_SET);
										read(fdExt, &blockPointer2, sizeof(unsigned int));
										
										if ( blockPointer2 == 0 ) { boolContinueReading = 0; break; }
										sumBlock3 = 0;
										dirOffset3 = blockPointer2 * ext.block_size;
										
										do {
											lseek(fdExt, dirOffset3, SEEK_SET);
											read(fdExt,&blockPointer3, sizeof(unsigned int));
											
											if ( blockPointer3 == 0 ) { boolContinueReading = 0; break; }
											dirOffset4 = blockPointer3 * ext.block_size;
											// Aqui ya estamos en el puntero que apunta al bloque de datos!
											sumBlock4 = 0;
											do {
												bytes_read = readBlock(fdExt,dirOffset3,carac,ext.block_size,readedBytes,inode.i_size);
												writeFat(fdFat,fat, carac, clustersFree[cluster_cursor], bytes_read );
												cluster_cursor++; readedBytes += bytes_read; sumBlock4 += ext.block_size; dirOffset4 += ext.block_size; 
											} while ( sumBlock4 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 );
											
											sumBlock3 	+= 4;
											dirOffset3 	+= 4;
										} while (sumBlock3 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);

										sumBlock2 +=4;
										dirOffset2 +=4;
									} while ( sumBlock2 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 );

									sumBlock 	+= 4;
									dirOffset 	+= 4;
								} while ( sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 ); 
							}
						// Final de los bucles
							
						// Mapear en la fat table las entradas	
							updateFatTable(fdFat, fat, clustersFree, clustersNeeded);
						// Pintamos conforme el archivo se ha guardado correctamente!!
							printf("\nFile copied correctly (%lu bytes) \n",inode.i_size);
					} else {
						printf("\nA file with that name already exists, the copy will not take place!\n");
					}
				} else {
					printf("\n\nNo free space in filesystem to save this file.\n\n");
				}
			} else {
				printErrCopyItsDirectory();
			} 
		} else {
			printFileNotFound();
		}
		
	} else {
		printFatNoFreeRootDirectory();
	}

	// Cerrarmos los ficheros!!!!!
	close(fdExt); close(fdFat);
}
/**********************************************
* @Nombre: 	fatSaveEntryInRooTDirectory
* @Def: 	Funcion encargada de guardar la entrada de directorio en Fat
* @Arg:	 	In: int fd - Filedescriptor del fichero de sistema de archivos
* @Arg:	 	In: Vfat fatinfo - Estructura que guarda la informacion de la tabla Fat
* @Arg:	 	In: FatDirectory fatdir - Estructura que contiene los datos para guardar en la root directory
* @Arg:	 	In: int freeRootEntry - Numero de entrada de la root directory disponible para guardar los datos
**********************************************/
int fatSaveEntryInRooTDirectory(int fdFAT, Vfat fatinfo, FatDirectory fatdir, int freeRootEntry) {
	// Encontrar la seek position de la primera root entry libre
	unsigned int rootDirectory = getFatStartRootDirectory(fatinfo);
	unsigned long offset = rootDirectory + (freeRootEntry * 32);
	char buffer[255]; bzero(buffer,255);
	
	lseek(fdFAT,offset,SEEK_SET);
	int i = 0;
	for ( i=0; i<32; i++) {
		write(fdFAT,"",1);
	}

	lseek(fdFAT,offset,SEEK_SET);
	write(fdFAT,fatdir.name,strlen(fatdir.name));				lseek(fdFAT,offset+8,SEEK_SET);
	write(fdFAT,fatdir.ext,3);									lseek(fdFAT,offset+8+3,SEEK_SET);
	write(fdFAT,&fatdir.attr,1);								lseek(fdFAT,offset+8+3+1+10,SEEK_SET);
	write(fdFAT,&fatdir.extTime,sizeof(unsigned int));			lseek(fdFAT,offset+8+3+1+10+4,SEEK_SET);
	write(fdFAT,&fatdir.firstblock,sizeof(unsigned short int)); lseek(fdFAT,offset+8+3+1+10+4+2,SEEK_SET);
	write(fdFAT,&fatdir.size, sizeof(unsigned int));
	
	return 0;
}

/**********************************************
* @Nombre: 	mapFatClustersFree
* @Def: 	Funcion encarga de obtener en un array los numeros de clusters libres para poder guardar el archivo
* @Arg:	 	In: int fd - Filedescriptor del fichero de sistema de archivos
* @Arg:	 	In: int mappingarray[] - Array donde se guardaran los numeros de los clusters libres.
* @Arg:	 	In: int mapArraySize - Tamaño del array, indica tambien cuantos clusters libres se necesitan para el archivo que queremos guardar
* @Arg:	 	In: Vfat fat - Estructura de datos de Fat
**********************************************/
int mapFatClustersFree(int fdFat, int mappingArray[], int mapArraySize, Vfat fat) {
	int i = 0; 
	int j = 0;
	int fatEntryReaded = 0;
	unsigned long offset = fatTableStartSeek(fat);
	// Sumamos los dos primeros reservados al offset para saltarlos.
	unsigned long fatLimit = fatTableStartSeek(fat) + (fat.sectors_perfat * fat.sectors_size);
	int tempCluster = 0;
	
	
	for (i=0; j<mapArraySize && offset < fatLimit; i++) {
		lseek(fdFat,offset,SEEK_SET);
		read(fdFat, &tempCluster, 2);
		if ( tempCluster == 0)	{
			mappingArray[j] = fatEntryReaded;
			j++;
		}
		offset +=2; fatEntryReaded++;
	}

	if ( j == mapArraySize ) 
		return 0;
	else 
		return -1;
}
int searchFreeRootDirInFat(int fdFAT, Vfat fatinfo) {
	int rootDirFreeEntry = -1;
	int dirEntryReaded = 0;
	FatDirectory fatdir;
	unsigned int startFileSeek = 0;
	unsigned int rootDirectory = getFatStartRootDirectory(fatinfo);
	
	startFileSeek = rootDirectory;
	
	do {
		fatdir = loadDirectoryEntryFat(fdFAT,startFileSeek);
		if (  fatdir.name[0] == (char) 0xE5 || fatdir.name[0] == (char) 0x00 ) {
			return dirEntryReaded;
		}
		
		startFileSeek += FAT_DIR_LENGTH;
		dirEntryReaded += 1;
	} while (dirEntryReaded < fatinfo.sectors_maxrootentr);

	return rootDirFreeEntry;
}
FatDirectory convertFromExtToFatDirEntry(ExtDirectory extdir, InodeEntry inode) {
	FatDirectory fat = newFatDirectoryEntry();
	char buffer[255]; bzero(buffer,255);
	memset(buffer,'\0',255);
	
	char *extension = strrchr(extdir.file_name, '.');
	if (extension == NULL) {
		strcpy(fat.ext,"");
	} else {
		sprintf(buffer,"%s",extension+1);
		strncpy(fat.ext, buffer,3);
		strToUpper(fat.ext);
	}
	// Comprobar caracteres raros
	if ( ( (extdir.name_len)-strlen(buffer) ) > 8) {
		strncpy(fat.name,extdir.file_name,6);
		fat.name[6] = '~';
		fat.name[7] = '1';
	} else {
		if ( extension == NULL )
			strncpy(fat.name,extdir.file_name, ( (extdir.name_len) - (strlen(buffer)) ) ); 
		else 
			strncpy(fat.name,extdir.file_name, ( (extdir.name_len) - (strlen(buffer)+1) ) ); // El 1 viene por el punto
	}
	fat.size = inode.i_size;
	fat.extTime = inode.i_ctime;
	fat.attr = 0x20;	//Obligamos a que es un tipo fichero
	
	strToUpper(fat.name);
	
	return fat;
}
int calculateNecesaryClusters(unsigned long extFilesize, int fatClustersize) {
	
	int clustersNecesarios = extFilesize / fatClustersize;
	float resto = extFilesize % fatClustersize;
	
	if ( resto > 0 ) {
		clustersNecesarios += 1;
	}
	
	return clustersNecesarios;
}
unsigned long fatTableStartSeek(Vfat fat) {
	unsigned long fatTableSeek;
	fatTableSeek = ( fat.sectors_reserved * fat.sectors_size );
	return fatTableSeek;
}
FatDirectory loadDirectoryEntryFat(int fdFAT,unsigned int offset) {
	
	lseek(fdFAT,offset,SEEK_SET);
	FatDirectory fatdir = newFatDirectoryEntry();
	
	// Leemos los char y limpiamos!
	read(fdFAT,&fatdir.name,8);	fatdir.name[8] = '\0';
	read(fdFAT,&fatdir.ext,3);		fatdir.ext[3] 	= '\0';
	read(fdFAT,&fatdir.attr,sizeof(unsigned char));
	read(fdFAT,&fatdir.reserved,10);
	read(fdFAT,&fatdir.time,sizeof(unsigned short int));
	read(fdFAT,&fatdir.date,sizeof(unsigned short int));
	read(fdFAT,&fatdir.firstblock,sizeof(unsigned short int));
	//lseek(fdFAT,offset+28,SEEK_SET);
	read(fdFAT,&fatdir.size,sizeof(unsigned int));
	
	return fatdir;
}
FatDirectory newFatDirectoryEntry(void) {
	FatDirectory fatdir;

	bzero(fatdir.name,	sizeof(fatdir.name));
	bzero(fatdir.ext,	sizeof(fatdir.ext));
	fatdir.attr 	= 0;
	fatdir.reserved = 0;
	fatdir.time 	= 0;
	fatdir.date		= 0;
	fatdir.size		= 0;
	fatdir.firstblock	= 0;
	fatdir.extTime = 0;
	
	return fatdir;
}
unsigned long getFatFirstDataSector(Vfat fat) {
	
	unsigned long fatFirstDataSectorSeek = fat.sectors_reserved + (fat.fat_tables * fat.sectors_perfat) + (fat.sectors_maxrootentr*32/fat.sectors_size); 
	
	
	return fatFirstDataSectorSeek;
}
// FUNCIONES FASE 3
void fatShowContent(int fd, Vfat fat, int seekDirectoryPosition) {
	FatDirectory fatdir = loadDirectoryEntryFat(fd,seekDirectoryPosition);
	int clusterSize = fat.sectors_cluster * fat.sectors_size;
	unsigned long fileReadedBytes = 0;
	int clusterReadedBytes = 0;
	char carac[2];
	unsigned long offset = 0;	
	unsigned long firstSectorOfCluster = 0;
	unsigned int blocksNeeded = fatdir.size / clusterSize;
	if ( (fatdir.size % clusterSize) > 0 ){
		blocksNeeded++;
	}

	// Aumentamos en 1 para guardar el ultimo bloque que contendra FFFF o FFF8
	blocksNeeded++;
	int blocks[blocksNeeded];
	unsigned int j = 0;
	// Limpiamos el array
	for ( j=0; j<blocksNeeded; j++) {
		blocks[j]=0;
	}
	// Cargamos en un array los clusters a visitar
	mapFatClustersInArray(fd,fatdir,blocks,blocksNeeded,fat);
	for ( j = 0; j<blocksNeeded && fileReadedBytes < fatdir.size; j++) {
		clusterReadedBytes = 0;
		firstSectorOfCluster = ( ( blocks[j] -2 ) * fat.sectors_cluster ) + getFatFirstDataSector(fat);
		offset = firstSectorOfCluster * fat.sectors_size;
		
		do {
			
			readChar(fd,offset,carac); // Carga en carac el byte leido
			printf("%c",carac[0]);
			
			offset++; clusterReadedBytes++; fileReadedBytes++;
		} while (fileReadedBytes<fatdir.size && clusterReadedBytes < clusterSize);
	}

	printf("\n");
}
void mapFatClustersInArray(int fdFat, FatDirectory fatdir, int mappingArray[], int mapArraySize, Vfat fat) {
	int i = 1;
	unsigned long nextCluster = fatdir.firstblock;
	unsigned long fatTableEntrySeek = 0;
	unsigned long fatTableLimit = fatTableStartSeek(fat) + (fat.sectors_perfat * fat.sectors_size);
	mappingArray[0] = nextCluster;
		
	do {
		fatTableEntrySeek = fatTableStartSeek(fat) + (2 * nextCluster);
		lseek(fdFat, fatTableEntrySeek,SEEK_SET);
		read(fdFat, &nextCluster, sizeof(unsigned short int));
		mappingArray[i] = nextCluster;
		i++;
	} while ( !(nextCluster >= 0xFFF8 && nextCluster <=0xFFFF) && nextCluster != 0 && fatTableEntrySeek < fatTableLimit && i < mapArraySize );	
}

void extShowContent(int fd, Ext ext, unsigned int inodeNumber) {
	InodeEntry inode = loadInodeData(fd, getInodeSeekPosition(fd, ext, inodeNumber));
	
	unsigned long readedBytes = 0;
	unsigned long sumBlock = 0;
	unsigned long sumBlock2 = 0;
	unsigned long sumBlock3 = 0;
	unsigned long sumBlock4 = 0;
	unsigned long dirOffset = 0;
	unsigned long dirOffset2 = 0;
	unsigned long dirOffset3 = 0;
	unsigned long dirOffset4 = 0;
	unsigned long blockPointer1 = 0;
	unsigned long blockPointer2 = 0;
	unsigned long blockPointer3 = 0;
	
	char carac[2];
	int boolContinueReading = 1;
	
	//printf("\tFile size: %lu \n",inode.i_size);
	
	
	int i=0;
	for (i =0; i<12; i++) {
		bzero(carac,2);
		sumBlock = 0; dirOffset = 0;
		if ( inode.i_block[i] == 0 ) { boolContinueReading = 0; break;} 
		dirOffset = inode.i_block[i] * ext.block_size;
		//printf("Primer bloque en %lu \n",dirOffset);
		do {
			readChar(fd,dirOffset,carac); // Carga en carac el byte leido
			printf("%c",carac[0]);
			
			readedBytes += 1; sumBlock += 1; dirOffset += 1;
		} while ( sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 );
		//printf("Bytes leidos %lu \n",readedBytes );
	}
	
	if ( inode.i_block[12] != 0 ) {
		
		sumBlock = 0; dirOffset = inode.i_block[12] * ext.block_size;
		do {
			lseek(fd,dirOffset,SEEK_SET);
			read(fd,&blockPointer1,sizeof(unsigned int));
			
			if ( blockPointer1 == 0 ) { boolContinueReading = 0; break;}
			sumBlock2 = 0;
			dirOffset2 = blockPointer1 * ext.block_size;
			
			do {
				readChar(fd,dirOffset2,carac);
				printf("%c",carac[0]);
				
				
				readedBytes +=1; sumBlock2+=1; dirOffset2 +=1;
			} while (sumBlock2 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);

			sumBlock 	+= 4;
			dirOffset 	+= 4;
		} while (sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);
	} 
	
	if ( inode.i_block[13] != 0 ) {
		sumBlock = 0; sumBlock2 = 0; sumBlock3 = 0; 
		dirOffset = inode.i_block[13] * ext.block_size; dirOffset2 = 0; dirOffset3 = 0;
		do {
			lseek(fd,dirOffset,SEEK_SET);
			read(fd,&blockPointer1,sizeof(unsigned int));
			
			if ( blockPointer1 == 0 ) { boolContinueReading = 0; break;}
			sumBlock2 = 0;
			dirOffset2 = blockPointer1 * ext.block_size;
			
			do {
				lseek(fd,dirOffset2,SEEK_SET);
				read(fd,&blockPointer2,sizeof(unsigned int));
				
				if ( blockPointer2 == 0 ) { boolContinueReading = 0; break;} 
				sumBlock3 = 0;
				dirOffset3 = blockPointer2 * ext.block_size;
				
				do {
					readChar(fd,dirOffset3,carac);
					printf("%c",carac[0]);
					

					sumBlock3 	+= 1; readedBytes += 1; dirOffset3 	+= 1;
				} while (sumBlock3 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);
				
				sumBlock2	+=4; 
				dirOffset2 	+=4;
			} while (sumBlock2 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);

			sumBlock 	+= 4;
			dirOffset 	+= 4;
		} while (sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);
	}	
	
	if ( inode.i_block[14] != 0 ) {
		sumBlock = 0; sumBlock2 = 0; sumBlock3 = 0; 
		dirOffset = inode.i_block[14] * ext.block_size; dirOffset2 = 0; dirOffset3 = 0;
		do {
			lseek(fd,dirOffset, SEEK_SET);
			read(fd,&blockPointer1,sizeof(unsigned int));
			
			if ( blockPointer1 == 0 ) { boolContinueReading = 0; break; }
			sumBlock2 = 0;
			dirOffset2 = blockPointer1 * ext.block_size;
			
			do {
				lseek(fd, dirOffset2, SEEK_SET);
				read(fd, &blockPointer2, sizeof(unsigned int));
				
				if ( blockPointer2 == 0 ) { boolContinueReading = 0; break; }
				sumBlock3 = 0;
				dirOffset3 = blockPointer2 * ext.block_size;
				
				do {
					lseek(fd, dirOffset3, SEEK_SET);
					read(fd,&blockPointer3, sizeof(unsigned int));
					
					if ( blockPointer3 == 0 ) { boolContinueReading = 0; break; }
					dirOffset4 = blockPointer3 * ext.block_size;
					// Aqui ya estamos en el puntero que apunta al bloque de datos!
					sumBlock4 = 0;
					do {
						readChar(fd,dirOffset3,carac);
						printf("%c",carac[0]);

						sumBlock4 += 1; dirOffset4 += 1; readedBytes +=1;
					} while ( sumBlock4 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 );
					
					sumBlock3 	+= 4;
					dirOffset3 	+= 4;
				} while (sumBlock3 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1);

				sumBlock2 +=4;
				dirOffset2 +=4;
			} while ( sumBlock2 < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 );

			sumBlock 	+= 4;
			dirOffset 	+= 4;
		} while ( sumBlock < ext.block_size && readedBytes < inode.i_size && boolContinueReading == 1 ); 
	}
	
	
}
void readChar (int fd, unsigned long offset, char *caracter) {
	char tmp;
	
	
	lseek(fd,offset,SEEK_SET);
	read(fd,&tmp,sizeof(char));
	caracter[0] = tmp;
	caracter[1] = '\0';
}
// FUNCIONES FASE 2
unsigned int getFatStartRootDirectory(Vfat fat) {
	unsigned int rootDirectory = 0;
	rootDirectory = ( (fat.sectors_reserved * fat.sectors_size) + (fat.fat_tables * fat.sectors_perfat * fat.sectors_size) ); 
	return rootDirectory;
} 
int fatStrLen(char *string) {
	// Despues de comprobar que el strlen no devuelve correctamente la longitud y ya que en FAT no pueden haber espacios.
	int fakeSize = strlen(string);
	int i = 0;
	
	for ( i = 0; i<fakeSize; i++) {
		if (string[i]==' ' || string[i]=='\0' ) {
			break;
		}
		if ( (string[i]=='+') || (string[i] == ',') || (string[i] == ';') || (string[i] == '=') || (string[i] == '[') || (string[i] == ']') ) {
			// Entrada no valida, contiene caracteres no validos para FAT
			// por lo tanto devolvemos 0 como si no hubieramos encontrado nada.
			return 0;
		}
	}

	return i;
}
void printFatFileInfo(int fd,int seekPosition,Vfat fat, int PARAM ) {
	if ( PARAM == PARAM_CAT ) {		
		fatShowContent(fd,fat,seekPosition);
	} else {
		FatDirectory fatdir = loadDirectoryEntryFat(fd,seekPosition);
		printf("\n\tFile found! Size: %d bytes\n" , fatdir.size);
		printf("\n");
	}
}
int findInFat(int fd, Vfat fat ,unsigned char *filetype, char *filename ) {
	char name[10];
	char extension[4];
	char findFileName[14];		// Nombre . extension
	
	int startFileSeek = 0;
	int nameLength = 0;
	int exteLength = 0;
	unsigned char atributes = 0;
	// Obtenemos posicion del rootDirectory
	unsigned int rootDirectory = getFatStartRootDirectory(fat);
	unsigned int filesize = 0;
	
	// Nos movemos hasta la posicion del fichero
	lseek(fd, rootDirectory, SEEK_SET);
	startFileSeek = rootDirectory;
	
	
	// Como en el root directory tenemos en un HDD maximo 512 entradas
	// y cada entrada son 32 bytes, buclaremos de 1 en 1 para encontrar
	// el archivo indicado en el directorio root. Una vez tengamos la posicion
	// devolveremos esta.
	int i=0;
	
	for ( i=0; i<fat.sectors_maxrootentr; i++ ) {
		bzero(name,10);	
		bzero(extension,4);
		bzero(findFileName,14);
		
		read(fd,name,8);
		//printf("Found: [%s] == [%s] \n",name,filename);
		nameLength = fatStrLen(name);	name[nameLength] = '\0';
		strcat(findFileName,name);
		
		
		if ( nameLength > 0 ) {
			read(fd,extension,3);
			exteLength = fatStrLen(extension); 
			extension[exteLength] = '\0';		

			if ( exteLength > 0 ) {
				strcat(findFileName,".");			
				strcat(findFileName,extension);
			}
			
			read(fd,&atributes,sizeof(unsigned char));
				
			// Comprobamos si coincide el nombre.ext con el que ha introducido el usuario
			if ( strcmp(findFileName, filename) == 0 ) {
				// Obtenemos el tamaño
				lseek(fd,16,SEEK_CUR);
				read(fd,&filesize,sizeof(unsigned int));
				
				if ( (atributes & 0x20) == 0x20 ) {
					*filetype = DATA_TYPE_FILE;
					return startFileSeek;
				} else if ( (atributes & 0x10) == 0x10 ) {
					*filetype = DATA_TYPE_DIR;
					return startFileSeek;
				}
				
				
				// Todos los otros ficheros no los mostramos, puesto que no son visibles, o bien son el volume label, o otro tipo de datos.

			} else {
				lseek(fd,20,SEEK_CUR);	
			}
		} else {
			// Saltamos 32-8 = 24 bytes
			lseek(fd,24,SEEK_CUR);
		}
		// Debido a que cuando leemos datos en el fichero, movemos el cursor, nos guardamos el inicio  
		// de la posicion de la entrada por si lo encontramos, devolver directamente sin hacer cambios.
		startFileSeek = startFileSeek + 32;
	}

	return FILE_NOT_FOUND;
}
unsigned long getInodeTableSeek(int fd, Ext ext) {
	
	unsigned long GroupDescriptorTablePosition = 0;
	unsigned long inodeTablePosition = 0;
	unsigned long inodeLseekPosition = 0;
	
	// Documentacion --> PAG 11
	if ( ext.block_first == 0 ) {
		GroupDescriptorTablePosition = ext.block_size;
	} else {
		GroupDescriptorTablePosition = EXT_SUPERBLOCK_START_POS + EXT_SUPERBLOCK_SIZE; // 1024 + 1024 
	}
		
	lseek(fd,GroupDescriptorTablePosition+4+4, SEEK_SET);
	read(fd,&inodeTablePosition, 4 );
	inodeLseekPosition = inodeTablePosition * ext.block_size;
	
	return inodeLseekPosition;
}
unsigned long findInExt(int fd, Ext ext, unsigned char *filetype, char *filename) {
	/*	
	unsigned long inodeTablePosition = getInodeTableSeek(fd,ext);
	unsigned long offset = inodeTablePosition + EXT_INODE_SIZE;
	unsigned long sumBlock = 0;
	int blockPoint = 0;
	unsigned long returnValue = 0;
	

	unsigned long dirOffset = 0;

	InodeEntry inode = loadInodeData(fd, offset); 
	ExtDirectory extdir;
	
	
	for ( blockPoint = 0; blockPoint < EXT_DIRECT_BLOCK_POINTERS; blockPoint++ ) {
		
		if ( inode.i_block[blockPoint] == 0 ) {
			return -1;
		}
		
		dirOffset = inode.i_block[blockPoint] * ext.block_size;
		
		do {
		
			extdir = loadDirectoryEntry(fd, dirOffset );
			if ( extdir.inode != 0) {
				// Comprobamos si el nombre coincide y si es una entrada de directorio valida!
				if ( strcmp(extdir.file_name, filename) == 0 ) {
					if ( extdir.file_type == DATA_TYPE_FILE ) {
						// printf("Es un fichero: %s \n",extdir.file_name);
						*filetype = DATA_TYPE_FILE;
						return dirOffset;	// Devolvemos el offset de la dirEntry, para mostrar podremos volver a leer el size y para copiar podremos mirar el inode.
					}	
					if ( extdir.file_type == DATA_TYPE_DIR) {
						*filetype = DATA_TYPE_DIR;
						return dirOffset;
					}
					
				} else {
					if ( extdir.file_type == DATA_TYPE_DIR ) {
						if ( strcmp(extdir.file_name,".") != 0 && strcmp(extdir.file_name,"..") != 0 ) {
							printf(">> Entrando en el directorio %s \n",extdir.file_name);
							
							// Entramos en el directorio y nos lo pateamos!
							returnValue = extSearchInSubfolder(fd,ext,filetype,filename,extdir.inode);
							if ( returnValue != 0 ) {
								return returnValue;
							}
						}
					}
				}
			}

			dirOffset += extdir.rec_len;
			sumBlock  += extdir.rec_len;
			//printf("Cantidad del bloque leida %lu del %s \n",sumBlock,extdir.file_name);
			//printf("Valor de sumblock %lu - valor de offset %lu \n",sumBlock, dirOffset);
		} while (sumBlock < ext.block_size);
	}
	// Comprobar el puntero indirecto
	
	// Comprobar los punteros indirectos dobles
	
	// Comprobar los punteros indirectos triples
	
	// Si llegamos a este punto... es porque no lo hemos encontrado.
	*/
	return extSearchInSubfolder(fd,ext,filetype,filename,2);
}
unsigned long extSearchInSubfolder(int fd, Ext ext, char *filetype, char *filename, unsigned int inodeNumber) {
	unsigned long retorno = 0;
	unsigned long dirOffset = 0;
	unsigned long dirOffset2 = 0;
	unsigned long dirOffset3 = 0;
	unsigned long dirOffset4 = 0;
	int blockPoint = 0;
	unsigned long blockPointer1 = 0;
	unsigned long blockPointer2 = 0;
	unsigned long blockPointer3 = 0;
	InodeEntry inode;
	ExtDirectory extdir;
	unsigned long sumBlock = 0;
	unsigned long sumBlock2 = 0;
	unsigned long sumBlock3 = 0;
	unsigned long sumBlock4 = 0;
	unsigned long offset;

	offset = getInodeSeekPosition(fd,ext, inodeNumber);
	inode = loadInodeData(fd, offset);
/*
	int inodeN = inodeNumber;
	if ( inodeN == 2 ) {
		int tmpInt = 0;
		for ( tmpInt = 0; tmpInt < 15; tmpInt++ ) {
			printf("%lu ",inode.i_block[tmpInt]);
			
		}
		printlnMsg("");
		printlnMsg("Inode number == 2");
	}
*/
	// Primeros 12 bloques del inodo
	for (blockPoint = 0; blockPoint<12; blockPoint++) {
		sumBlock = 0;
		if ( inode.i_block[blockPoint] == 0 ) {
			return 0;
		}
		
		dirOffset = inode.i_block[blockPoint] * ext.block_size;
		
		do {
		
			extdir = loadDirectoryEntry(fd, dirOffset );
			// Comprobamos si el nombre coincide y si es una entrada de directorio valida!
			if ( extdir.inode != 0 ) {
				if ( strcmp(extdir.file_name, filename) == 0 ) {
					if ( extdir.file_type == DATA_TYPE_FILE ) {
						//printf("Es un fichero: %s \n",extdir.file_name);
						*filetype = DATA_TYPE_FILE;
						return dirOffset;	// Devolvemos el offset de la dirEntry, para mostrar podremos volver a leer el size y para copiar podremos mirar el inode.
					}	
					if ( extdir.file_type == DATA_TYPE_DIR) {
						//printf("Es un directorio: %s \n",extdir.file_name);
						*filetype = DATA_TYPE_DIR;
						return dirOffset;
					}
					
				} else {
					// Solo nos interesan los directorios en esta zona
					if ( extdir.file_type == DATA_TYPE_DIR) {
						
						if ( strcmp(extdir.file_name,".") != 0 && strcmp(extdir.file_name,"..") != 0 ) {
							//printf(">> Entrando en el directorio %s \n",extdir.file_name);
							
							// Entramos en el directorio y nos lo pateamos!
							retorno = extSearchInSubfolder(fd,ext,filetype,filename,extdir.inode);
							if ( retorno != 0 ) {
								return retorno;
							}
						}
					} 
				}
			}

			dirOffset += extdir.rec_len;
			sumBlock  += extdir.rec_len;
			//printf("Valor de sumblock %lu - valor de offset %lu \n",sumBlock, dirOffset);
		} while (sumBlock < ext.block_size );
		
	}
	
	// Primer puntero indirecto

	if ( inode.i_block[12] != 0 ) {
		sumBlock = 0;
		dirOffset = inode.i_block[12] * ext.block_size;
		// Ahora estamos posicionado en el bloque que contendra punteros a los bloques de informacion
		do {
			lseek(fd,dirOffset,SEEK_SET);
			read(fd,&blockPointer1,sizeof(unsigned int));

			// No hay mas informacion guardada en este bloque
			if ( blockPointer1 == 0 ) {
				return 0;
			}
			dirOffset2 = blockPointer1 * ext.block_size;

			sumBlock2 = 0;
			do {
				extdir = loadDirectoryEntry(fd, dirOffset2);
				if ( extdir.inode != 0 ) {
					if ( strcmp(extdir.file_name, filename) == 0 ) {
						if ( extdir.file_type == DATA_TYPE_FILE ) {
							*filetype = DATA_TYPE_FILE;
							return dirOffset2;	// Devolvemos el offset de la dirEntry, para mostrar podremos volver a leer el size y para copiar podremos mirar el inode.
						}	
						if ( extdir.file_type == DATA_TYPE_DIR) {
							*filetype = DATA_TYPE_DIR;
							return dirOffset2;
						}
						
					} else {
						// Solo nos interesan los directorios en esta zona
						if ( extdir.file_type == DATA_TYPE_DIR) {
							
							if ( strcmp(extdir.file_name,".") != 0 && strcmp(extdir.file_name,"..") != 0 ) {
								//printf(">> Entrando en el directorio %s \n",extdir.file_name);
								
								// Entramos en el directorio y nos lo pateamos!
								retorno = extSearchInSubfolder(fd,ext,filetype,filename,extdir.inode);
								if ( retorno != 0 ) {
									return retorno;
								}
							}
						} 
					}
				}
				dirOffset2 += extdir.rec_len;
				sumBlock2  += extdir.rec_len;
			} while (sumBlock2 < ext.block_size);
			// Sumamos el puntero leido
			sumBlock += 4;
			dirOffset += 4;
		} while (sumBlock < ext.block_size);
	}

	sumBlock 		= 0;	sumBlock2		= 0;
	blockPointer1	= 0;	blockPointer2	= 0;
	dirOffset		= 0;	dirOffset2		= 0;
	
	// Buscamos dentro del puntero doble indirecto
	if ( inode.i_block[13] != 0 ) {
		sumBlock = 0;
		dirOffset = inode.i_block[13] * ext.block_size;
		
		do {
			lseek(fd,dirOffset,SEEK_SET);
			read(fd,&blockPointer1,sizeof(unsigned int));
			
			if ( blockPointer1 == 0 ) { return 0; }
			
			blockPointer2 = 0;
			dirOffset2 = blockPointer1 * ext.block_size;
			sumBlock2 = 0;

			do {
				
				lseek(fd,dirOffset2,SEEK_SET);
				read(fd,&blockPointer2, sizeof(unsigned int));
				
				if ( blockPointer2 == 0 ) { return 0;}
				sumBlock3 = 0;
				
				dirOffset3 = blockPointer2 * ext.block_size;
				
				do { 
					lseek(fd,dirOffset3,SEEK_SET);
					extdir = loadDirectoryEntry(fd, dirOffset3);
					
					if ( extdir.inode != 0 ) {
						if ( strcmp(extdir.file_name, filename) == 0 ) {
							if ( extdir.file_type == DATA_TYPE_FILE ) {
								//printf("P13 Es un fichero: %s \n",extdir.file_name);
								*filetype = DATA_TYPE_FILE;
								return dirOffset3;	// Devolvemos el offset de la dirEntry, para mostrar podremos volver a leer el size y para copiar podremos mirar el inode.
							}	
							if ( extdir.file_type == DATA_TYPE_DIR) {
								//printf("P13 Es un directorio: %s \n",extdir.file_name);
								*filetype = DATA_TYPE_DIR;
								return dirOffset3;
							}
							
						} else {
							// Solo nos interesan los directorios en esta zona
							if ( extdir.file_type == DATA_TYPE_DIR) {
								
								if ( strcmp(extdir.file_name,".") != 0 && strcmp(extdir.file_name,"..") != 0 ) {
									//printf(">> P13 Entrando en el directorio %s \n",extdir.file_name);
									
									// Entramos en el directorio y nos lo pateamos!
									retorno = extSearchInSubfolder(fd,ext,filetype,filename,extdir.inode);
									if ( retorno != 0 ) {
										return retorno;
									}
								}
							}
						}
					}
					sumBlock3 	+= extdir.rec_len;
					dirOffset3 	+= extdir.rec_len;
				} while ( sumBlock3 < ext.block_size );
				
				sumBlock2 += 4;
				dirOffset2 += 4;
			} while ( sumBlock2 < ext.block_size );
			
			sumBlock += 4;
			dirOffset += 4;
		} while ( sumBlock < ext.block_size );
	}
	// Buscamos dentro del puntero triple indirectos
	
	if ( inode.i_block[14] != 0 ) {
		sumBlock = 0; sumBlock2 = 0; sumBlock3 = 0;
		dirOffset = 0; dirOffset2 = 0; dirOffset3 = 0; dirOffset4 = 0;
		blockPoint = 0; blockPointer1 = 0; blockPointer2 = 0; blockPointer3 = 0;
		dirOffset = inode.i_block[14] * ext.block_size;
		
		do {
			lseek(fd,dirOffset,SEEK_SET);
			read(fd,&blockPointer1,sizeof(unsigned int));
			if ( blockPointer1 == 0 ) { return 0; }
			blockPointer2 = 0; sumBlock2 = 0;
			dirOffset2 = blockPointer1 * ext.block_size;
			
				do {
					lseek(fd, dirOffset2, SEEK_SET);
					read(fd, &blockPointer2, sizeof(unsigned int));
					if ( blockPointer2 == 0 ) { return 0; }
					blockPointer3 = 0; sumBlock3 = 0;
					dirOffset3 = blockPointer2 * ext.block_size;
					do {
						lseek(fd,dirOffset3, SEEK_SET);
						read(fd,&blockPointer3, sizeof(unsigned int));
						
						if ( blockPointer3 == 0) { return 0; }
						dirOffset4 = blockPointer3 * ext.block_size;
						sumBlock4 = 0;
						
						do {
							lseek(fd,dirOffset4,SEEK_SET);
							extdir = loadDirectoryEntry(fd, dirOffset4);
							if ( extdir.inode != 0 ) {
								if ( strcmp(extdir.file_name, filename) == 0 ) {
									if ( extdir.file_type == DATA_TYPE_FILE ) {
										//printf("P13 Es un fichero: %s \n",extdir.file_name);
										*filetype = DATA_TYPE_FILE;
										return dirOffset4;	// Devolvemos el offset de la dirEntry, para mostrar podremos volver a leer el size y para copiar podremos mirar el inode.
									}	
									if ( extdir.file_type == DATA_TYPE_DIR) {
										//printf("P13 Es un directorio: %s \n",extdir.file_name);
										*filetype = DATA_TYPE_DIR;
										return dirOffset4;
									}
									
								} else {
									// Solo nos interesan los directorios en esta zona
									if ( extdir.file_type == DATA_TYPE_DIR) {
										
										if ( strcmp(extdir.file_name,".") != 0 && strcmp(extdir.file_name,"..") != 0 ) {
											printf(">> P13 Entrando en el directorio %s \n",extdir.file_name);
											
											// Entramos en el directorio y nos lo pateamos!
											retorno = extSearchInSubfolder(fd,ext,filetype,filename,extdir.inode);
											if ( retorno != 0 ) {
												return retorno;
											}
										}
									} else {
										printf("Comprobando %s \n",extdir.file_name);
									}
								}
							}
							sumBlock4 	+= extdir.rec_len;
							dirOffset4 	+= extdir.rec_len;
								
							
							
						} while (sumBlock4 < ext.block_size );
						
						sumBlock3 += 4;
						dirOffset3 += 4;
					} while ( sumBlock3 < ext.block_size );
					
					sumBlock2 += 4;
					dirOffset2 += 4;
				} while ( sumBlock2 < ext.block_size ); 
			
			
			sumBlock 	+= 4;
			dirOffset 	+= 4;
		} while ( sumBlock < ext.block_size );
	} 
	
	
	
	
	
	
	return retorno;
}
ExtDirectory loadDirectoryEntry(int fd, unsigned long dataBlockSeekPos) {
	ExtDirectory extDir;
	lseek(fd, dataBlockSeekPos, SEEK_SET);
	bzero(extDir.file_name, BUFFSIZE);
	
	read( fd,&extDir.inode, sizeof(unsigned int)); 			
	read( fd,&extDir.rec_len, sizeof(unsigned short int) ); 
	read( fd,&extDir.name_len, sizeof(unsigned char) ); 	
	read( fd,&extDir.file_type, sizeof(unsigned char) );	
	read( fd,&extDir.file_name, extDir.name_len);		
	
	// Volvemos a la posicion donde estabamos
	lseek(fd, dataBlockSeekPos, SEEK_SET);
	
	return extDir;
}
unsigned long getInodeSeekPosition(int fd, Ext ext, unsigned int inodeNum ) {
	unsigned long offset = 0;
	offset = getInodeTableSeek(fd, ext) + (inodeNum - 1) * ext.inode_size;
	
	return offset;
}
InodeEntry loadInodeData(int fd, unsigned long offset) {
		
		lseek(fd,offset,SEEK_SET);
		InodeEntry inode;
		inode.i_mode = 0;
		inode.i_size = 0;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_mode, 2 ); 		offset += 2;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_uid, 2 ); 		offset += 2;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_size, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_atime, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_ctime, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_mtime, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_dtime, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_gid, 2 );			offset += 2;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_links_count, 2 );	offset += 2;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_blocks, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_flags, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_osd1, 4 );		offset += 4;
		
		
		int i = 0;
		for ( i=0; i < 15 ; i++ ) {
			lseek(fd,offset,SEEK_SET);
			inode.i_block[i] = 0;
			read( fd,&inode.i_block[i], sizeof(unsigned int) ); offset += 4;
		}
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_generation, 4 );	offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_file_acl, 4 );	offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_dir_acl, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_faddr, 4 );		offset += 4;
		
		lseek(fd,offset,SEEK_SET);
		read( fd,&inode.i_osd2, 12 );		offset += 12;
	
		return inode;
}
void printExtFileInfo( int fd,Ext ext, unsigned long dirSeek, int PARAM ) {

	unsigned long offset = 0;
	ExtDirectory extdir = loadDirectoryEntry(fd, dirSeek);
	

	offset = getInodeSeekPosition(fd, ext, extdir.inode);
		lseek(fd,offset,SEEK_SET);
	InodeEntry inode = loadInodeData(fd,offset);
	
	// Calculamos el inicio del inodo --> Inicio tabla inodo + posicion inicial inodo indicado
	//offset = getInodeTableSeek(fd, ext) + (extdir.inode - 1) * ext.inode_size;

/*	int i= 0;
	for ( i=0; i<15; i++) {
		printf("%d %lu %lu \n",i+1,inode.i_block[i],inode.i_block[i] * ext.block_size);
	} */
	if ( PARAM == PARAM_CAT ) {
		printf("\n");
		extShowContent(fd,ext,extdir.inode);
	} else {
		printf("\n");
		printf("\tFile found in inode [%u]!\n\tSize %lu Bytes\n",extdir.inode,inode.i_size);
		printf("\n");
		printf("\n\n");
	}
}
unsigned int findFileinVolume(char *filePath, char *filename, int PARAM) {

	Vfat fat;
	Ext ext;
	unsigned char filetype;
	int seekFilePosition = 0;
	unsigned long seekDirEntry;

	// Try open file
	int fd = open(filePath, O_RDONLY);
	if ( fd == FILE_ERROROPEN ) {
		printErrFileopen(filePath);
		exit(EXIT_FAILURE);
	}

	int partType = checkPartType(fd);
	switch ( partType ) {
		case PART_TYPE_FAT16: 
			// Load FAT info
			fat = loadFatInfo(fd);

			// Como en FAT el nombre de ficheros esta en mayuscula, pasamos a mayuscula el argumento introducido
			strToUpper(filename);

			seekFilePosition = findInFat(fd,fat, &filetype, filename);
			
			if ( seekFilePosition != FILE_NOT_FOUND ) {
				switch(filetype) {
					case DATA_TYPE_FILE:
						printFatFileInfo(fd,seekFilePosition,fat,PARAM);
					break;
					case DATA_TYPE_DIR:
						printDirectoryFound();
					break;
					default:
						printFileNotFound();				
					break;
				}
			} else {
				printFileNotFound();
			}

			
		break;
		case PART_TYPE_EXT2:
			ext = loadExtInfo(fd);
			if ( strcmp(filename,".") == 0 || strcmp(filename,"..") == 0 ) {
				printf("\n\tNo se realizara busqueda de un nombre reservado [ %s ] \n",filename);
				seekDirEntry = DATA_TYPE_NOTFOUND;
			} else {
				if ( strncmp(filename,".",1) == 0 ) {
					seekDirEntry = DATA_TYPE_NOTFOUND;
					//printf("No se mostraran los ficheros ocultos!\n");
				} else {
					seekDirEntry = findInExt(fd,ext,&filetype,filename);	
				}
			}
			
			if ( seekDirEntry != DATA_TYPE_NOTFOUND ) {
				switch (filetype) {
					case DATA_TYPE_FILE:
						printExtFileInfo(fd, ext, seekDirEntry, PARAM);
						
					break;
					case DATA_TYPE_DIR:
						printDirectoryFound();
					break;
					default:
						printFileNotFound();
					break;
				}
			} else {
				printFileNotFound();
			}
		
		break;
		default:
			printErrUnknowPartition();
			exit(EXIT_FAILURE);
		break;
		
	}

	// Cerramos el fichero
	close(fd);
	return 0;
}
void strToUpper(char *string) {
	int size = strlen(string);
	int i=0;
	
	for ( i=0; i<size; i++ ) {
		string[i] = toupper(string[i]);
	}
}

// FUNCIONES FASE 1
/**********************************************
* @Nombre: 	strToLower
* @Def: 	Funcion encargada de pasar un string a minusculas, modifica directamente la string
* @Arg:	 	In: *string: Recibe el texto a modificar 
**********************************************/
void strToLower(char *string) {
	int size = strlen(string);
	int i=0;
	for ( i = 0; i<size; i++ ) {
		string[i] = tolower(string[i]);
	}
}
/**********************************************
* @Nombre: 	loadPartitionInfo
* @Def: 	Funcion encargada de abrir el fichero y realizar las acciones de muestreo
* @Arg:	 	In: *filePath: Recibe la ruta del fichero (tercer parametro)
**********************************************/
void loadPartitionInfo(char *filePath) {
	// Open partition file
	int fd = open(filePath, O_RDONLY);
	Ext ext2;
	Vfat fat;
	if ( fd == FILE_ERROROPEN ) {
		printErrFileopen(filePath);
		exit(EXIT_FAILURE);
	}
	
	
	int partType = checkPartType(fd);
	
	switch(partType) {
		case PART_TYPE_EXT2:
			ext2 = loadExtInfo(fd);
			printExt2Info(ext2);
		break;
		case PART_TYPE_FAT16:
			fat = loadFatInfo(fd);
			printFatInfo(fat);
		break;
		default:
			printErrUnknowPartition();
		break;
	}
	
	// Closing partition file
	close(fd);
}
/**********************************************
* @Nombre: 	checkPartType
* @Def: 	Funcion encargada de detectar el tipo de particion que se ha indicado
* @Arg:	 	In: fd: Recibe el file descriptor del fichero
* @Ret:  	Devuelve el tipo de particion que ha detectado
**********************************************/
int checkPartType(int fd) {
	
	// For FAT12,16,32
	char vfat16[8];
	// For EXT2,3,4
	unsigned short int s_magic;
	
		// Obtenemos el TAG para las FAT.
		bzero(vfat16,8);
		lseek(fd,54,SEEK_SET);		read(fd,&vfat16,8);
		if ( strncmp(vfat16,"FAT16",5) == 0 ) {
			return PART_TYPE_FAT16;
		}
		
/*	DETECCION FAT32		
		bzero(vfat16,8);
		lseek(fd,82,SEEK_SET);		read(fd,&vfat16,8);
		if ( strncmp(vfat16,"FAT32",5) == 0) {
			return PART_TYPE_FAT16;
		}
*/
		// Buscando EXT
		lseek(fd,(1024+56),SEEK_SET);	read(fd,&s_magic,sizeof(unsigned short int));
		if ( s_magic == 0xef53 ){	// http://man7.org/linux/man-pages/man2/statfs.2.html
			return PART_TYPE_EXT2;
		} 

		
		
	return PART_TYPE_UNKW;
}
// EXT2 FUNCTIONS
/**********************************************
* @Nombre: 	loadExtInfo
* @Def: 	Funcion encargada de cargar la información de la particion ext
* @Arg:	 	In: fd: Recibe el file descriptor del fichero
* @Ret:  	Devuelve un struct con la información de la particion
**********************************************/
Ext loadExtInfo(int fd) {
	Ext ext2;
	strcpy(ext2.volume_type,"EXT2");
	lseek(fd,(1024+0),	SEEK_SET); 	read(fd,&(ext2.inode_total),	sizeof(unsigned int));
									read(fd,&(ext2.block_total),	sizeof(unsigned int));
									read(fd,&(ext2.block_reserved),	sizeof(unsigned int));
									read(fd,&(ext2.block_free),		sizeof(unsigned int));
									read(fd,&(ext2.inode_free),		sizeof(unsigned int));
									read(fd,&(ext2.block_first),	sizeof(unsigned int));
									read(fd,&(ext2.block_size),		sizeof(unsigned int));
									
	lseek(fd,(1024+32),	SEEK_SET);	read(fd,&(ext2.block_group),	sizeof(unsigned int));
									read(fd,&(ext2.frags_group),	sizeof(unsigned int));
									read(fd,&(ext2.inode_group),	sizeof(unsigned int));
									
	lseek(fd,(1024+48),	SEEK_SET);	read(fd,&(ext2.last_write),		sizeof(unsigned int));
	lseek(fd,(1024+64),	SEEK_SET);	read(fd,&(ext2.last_check),		sizeof(unsigned int));
	lseek(fd,(1024+44),SEEK_SET);	read(fd,&(ext2.last_mount),		sizeof(unsigned int));
	
	lseek(fd,(1024+84),	SEEK_SET);	read(fd,&(ext2.inode_first),	sizeof(unsigned int));
									read(fd,&(ext2.inode_size),		sizeof(unsigned int));
	lseek(fd,(1024+120),SEEK_SET);	read(fd,&(ext2.volume_name),	16 );
	
	
	// Arreglamos los computos
	ext2.block_size =  1024 << ext2.block_size;		// 3.1.7 EXT2 DOCUMENTATION
	
	return ext2;
}
/**********************************************
* @Nombre: 	printExt2Info
* @Def: 	Funcion encargada de pintar la informacion de la variable struct ext2
* @Arg:	 	In: Ext ext2: Recibe la estructura Ext y la pinta por pantalla
**********************************************/
void printExt2Info(Ext ext2) {
	printf("---- Fylesystem  Information ---- \n");
	
	printf("\nFylesystem: %s",ext2.volume_type);
	
	printf("\n\nINODE INFO\n");
	printf("\tINODE SIZE: \t%d \n",ext2.inode_size);
	printf("\tINODE TOTAL: \t%d \n",ext2.inode_total);
	printf("\tINODE FIRST: \t%d \n",ext2.inode_first);
	printf("\tINODES GROUP: \t%d \n",ext2.inode_group);
	printf("\tINODES FREE: \t%d \n",ext2.inode_free);
	
	printf("\n\nBLOCK INFO\n");
	printf("\tBLOCK TOTAL: \t%d \n",ext2.block_total);
	printf("\tBLOCKS RESVD: \t%d \n",ext2.block_reserved);
	printf("\tBLOCKS FREE: \t%d \n",ext2.block_free);
	printf("\tBLOCK SIZE: \t%lu \n",ext2.block_size);
	printf("\tBLOCK FIRST: \t%d \n",ext2.block_first);
	printf("\tBLOCK GROUP: \t%d \n",ext2.block_group);
	printf("\tFRAGS GROUP: \t%d \n",ext2.frags_group);
	
	printf("\n\nVOLUME INFO\n");
	printf("\tVOLUM NAME: \t%s \n",ext2.volume_name);
	printf("\tLAST CHECK: \t%s ",ctime( &(ext2.last_check)  ) );
	printf("\tLAST MOUNT: \t%s ",ctime( &(ext2.last_mount)  ) );
	printf("\tLAST WRITE: \t%s ",ctime( &(ext2.last_write)  ) );
	printlnMsg("");
}
// FAT FUNCTIONS
/**********************************************
* @Nombre: 	loadFatInfo
* @Def: 	Funcion encargada de de cargar la información de la particion fat
* @Arg:	 	In: fd: Recibe el file descriptor del fichero
* @Ret:  	Devuelve un struct con la información de la particion
**********************************************/
Vfat loadFatInfo(int fd) {
	Vfat fat;
	char tmp_type[8];
	bzero(fat.volume_type,8);	
	bzero(fat.volume_label,12);
	
	
	lseek(fd,54,SEEK_SET);		read(fd,&tmp_type, 8);			strncpy(fat.volume_type,tmp_type,5);
	lseek(fd,3,	SEEK_SET);		read(fd,&(fat.volume_name), 8);
	
	lseek(fd,43,SEEK_SET);		read(fd,&(fat.volume_label),11);
	lseek(fd,11,SEEK_SET);		read(fd,&(fat.sectors_size),sizeof(unsigned short int));
	lseek(fd,13,SEEK_SET);		read(fd,&(fat.sectors_cluster),sizeof(unsigned char));
	lseek(fd,14,SEEK_SET);		read(fd,&(fat.sectors_reserved),sizeof(unsigned short int));
	lseek(fd,16,SEEK_SET);		read(fd,&(fat.fat_tables),sizeof(unsigned short int));
	lseek(fd,17,SEEK_SET);		read(fd,&(fat.sectors_maxrootentr),sizeof(unsigned short int));
	lseek(fd,22,SEEK_SET);		read(fd,&(fat.sectors_perfat),sizeof(unsigned short int));

	return fat;
}
/**********************************************
* @Nombre: 	printFatInfo
* @Def: 	Funcion encargada de pintar la informacion de la variable struct Vfat
* @Arg:	 	In: Vfat fat: Recibe la estructura Fat y la pinta por pantalla
**********************************************/
void printFatInfo(Vfat fat) {
	
	printf("---- Fylesystem  Information ---- \n");
	printlnMsg("");
	printf("Fylesystem: %s \n",fat.volume_type);
	
	printf("\n");
	printf("VOLUME NAME: \t%s \n",fat.volume_name);
	printf("VOLUME LABEL: \t%s \n",fat.volume_label);
	printf("SECTOR SIZE: \t%d \n",fat.sectors_size);
	printf("SECTORS per CLUSTER: %d \n",fat.sectors_cluster);
	printf("SECTORS RESERVED: %d \n",fat.sectors_reserved);
	printf("NUM FAT TABLES: %d \n",fat.fat_tables);
	printf("MAX ROOT ENTRIES: %d \n",fat.sectors_maxrootentr);
	printf("SECTORS per FAT: %d \n",fat.sectors_perfat);
	printf("\n\n");
	
}
// PRINT ERROR / INFO MESSAGE FUNCTIONS
void printFileNotFound(void) {
	
	printlnMsg("");
	printlnMsg("\tError: File not found.");
	printlnMsg("");
}
void printDirectoryFound(void) {
	printlnMsg("");
	printlnMsg("\tIs a directory!.");
	printlnMsg("");
}
/**********************************************
* @Nombre: 	printMsg
* @Def: 	Funcion encargada de pintar una string por pantalla
* @Arg:	 	In: *string: Recibe una string y la pinta por pantalla
**********************************************/
void printMsg(char *message) {
	write (SCREEN, message, strlen(message) );
}
/**********************************************
* @Nombre: 	printlnMsg
* @Def: 	Funcion encargada de pintar una string por pantalla
* @Arg:	 	In: *string: Recibe una string y la pinta por pantalla añadiendo un \n al final
**********************************************/
void printlnMsg(char *message) {
	
	printMsg(message);
	printMsg(" \n");
}
/**********************************************
* @Nombre: 	printErrHeader
* @Def: 	Funcion encargada de pintar una cabecera por pantalla
**********************************************/
void printErrHeader(void) {
	printlnMsg("\n--- COPYCAT SOFTWARE ERROR ---");
}
/**********************************************
* @Nombre: 	printErrFooter
* @Def: 	Funcion encargada de pintar un footer por pantalla
**********************************************/
void printErrFooter(void) {
	printlnMsg("--- COPYCAT LA SALLE 2015 ---\n");
	
}
/**********************************************
* @Nombre: 	printErrNoParams
* @Def: 	Funcion encargada de pintar un mensaje de error indicando que no se ha pasado ningun parametro o no  es correcto
**********************************************/
void printHelp(void) {
	printlnMsg("");
	printErrHeader();
	printlnMsg("");
	printlnMsg("Las opciones son: ");
	printlnMsg("\t > copycat.exe /info volume ");
	printlnMsg("\t > copycat.exe /find volume file ");
	printlnMsg("\t > copycat.exe /about ");
	printlnMsg("");
	printErrFooter();
	printlnMsg("");
}
void printErrNoParams(void) {
	
	printlnMsg("\nError: Incorrect parameters number");
	printlnMsg("\t > copycat.exe /find volume file \n");
	
}
/**********************************************
* @Nombre: 	printErrFileopen
* @Def: 	Funcion encargada de pintar un mensaje de error indicando que no se ha encontrado el fichero
* @Arg:	 	In: *string: Recibe una string que es la ruta al fichero que no ha podido abrir
**********************************************/
void printErrFileopen(char *filePath) {
	printErrHeader();
	printlnMsg("Error al abrir el fichero indicado");
	printlnMsg(filePath);
	printErrFooter();
}
/**********************************************
* @Nombre: 	printErrUnknowPartition
* @Def: 	Funcion encargada de pintar un mensaje de error indicando que no se ha reconocido el tipo de particion
**********************************************/
void printErrUnknowPartition(void) {

	printlnMsg("");
	printlnMsg("\tError: The volume is neither FAT16 or EXT2");
	printlnMsg("");

}
/**********************************************
* @Nombre: 	printAbout
* @Def: 	Funcion encargada de pintar un mensaje "about" sobre la información del programa
**********************************************/
void printAbout(void) {
	printlnMsg("");
	printlnMsg("---- LS COPYCAT SOFTWARE 2015 vfase.1 ----");
	printlnMsg("");
	printlnMsg("Aplicación encargada de leer y mostrar la información");
	printlnMsg("sobre un fichero de particiones FAT / EXT");
	printlnMsg("");
	printAuthor();
	printlnMsg("");

} 
/**********************************************
* @Nombre: 	printAbout
* @Def: 	Funcion encargada de pintar un mensaje sobre la información del autor
**********************************************/
void printAuthor(void) {	
	printlnMsg("Software desarrollado por Guille Rodríguez Gonzalez");
	printlnMsg("alumno de URL La Salle Barcelona Campus para la");
	printlnMsg("asignatura de Sistemas Operativos Avanzados (SOA)");
}
void printErrCopy(void) {
	printf("\nError: Parametros incorrectos.\n");
	printf("\t copycat /copy volumeEXT volumeFAT fileToCopy\n");
}
void printFatNoFreeRootDirectory(void) {
	printf("No free root directory entries in FAT filesystem. Can not copy the file. \n");
}
void printFatNoSpaceForFile(void) {
	printf("Not enough free space for copy this file.\n");
}
void printErrCopyItsDirectory(void) {
	printf("El fichero que has indicado es un directorio! Solo se pueden copiar ficheros! \n");
}
