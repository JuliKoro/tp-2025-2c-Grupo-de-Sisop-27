#ifndef S_FUNCIONES_H
#define S_FUNCIONES_H   

#include <utils/loggeo.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <commons/string.h>
#include <utils/configs.h>

extern t_log* logger_storage;

extern storage_conf* storage_config;

extern superblock_conf* superblock_config;


/**
 * @brief Copia un archivo de origen a destino
 * @param origen el nombre del archivo de origen
 * @param destino el nombre del archivo de destino
 * @return void
 */
void copiarArchivo(char* origen, char* destino); 

/**
 * @brief Crea un archivo en la ruta especificada
 * @param path la ruta del archivo a crear
 * @return void
 */
void crearArchivo(char* path);

/**
 * @brief Usa los valores del archivo superblock.config para crear los bloques fisicos necesarios. 
 * @return void
 */
void crearBloques();

/**
 * @brief Inicializa el punto de montaje. Si no existe lo crea. Si existe lo borra y crea uno nuevo. 
 * @param path el path del punto de montaje
 * @return void
 */
void inicializarPuntoMontaje(char* path);
#endif