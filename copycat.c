/****************************************************************
*	@File:		copycat.c
*	@Desc:		Programa de linea de comandos que realiza operaciones de mostrar la 
* 				informacion en la root dir de las particiones FAT16 y EXT2, buscar 
*				fichero e indicar su tamaño, mostrar el contenido del fichero y realizar 
*				una copia de un fichero del sistema EXT2 a un sistema FAT16
*	@Author: 	Guille Rodríguez González	LS26151
*	@Date:		2014/2015
****************************************************************/

#include "func.h"

int main ( int argc, char **argv) {
	char bufferStr[BUFFSIZE];
	bzero(bufferStr,BUFFSIZE);
	int exmode;
	
	
	if ( argc < 2) {
		printErrNoParams();
		exit(EXIT_FAILURE);
	}

	exmode = execMode(argv[1]);
	
	if ( exmode != PARAM_ABOUT && argc < 3 ) {
		printErrNoParams();
		exit(EXIT_FAILURE);
	}
	
	switch ( exmode ) {
		case PARAM_INFO:
			loadPartitionInfo(argv[2]);
		break;
		case PARAM_ABOUT:
			printAbout();
		break;
		case PARAM_FIND:
			if ( argc != 4 ) {
				printErrNoParams();
				exit(EXIT_FAILURE);
			}
			
			findFileinVolume(argv[2], argv[3], exmode);
		
		break;
		case PARAM_CAT:
			if ( argc != 4 ) {
				printErrNoParams();
				exit(EXIT_FAILURE);
			}
			findFileinVolume(argv[2], argv[3], exmode);
		break;
		case PARAM_COPY:
			if ( argc != 5 ) {
				printErrCopy();
				exit(EXIT_FAILURE);
			}
			
			copyFileFromExtToFat(argv[2],argv[3],argv[4]);
		break;
		default:
			printHelp();
			exit(EXIT_FAILURE);
		break;
		
	}

	return EXIT_SUCCESS;
}

