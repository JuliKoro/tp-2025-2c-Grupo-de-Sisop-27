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
#include <commons/bitarray.h>
#include <pthread.h>
#include <sys/mman.h>
#include <commons/crypto.h>

extern t_log* g_logger_storage;

extern storage_conf* g_storage_config;

extern superblock_conf* g_superblock_config;

extern t_bitarray* g_bitmap;

extern t_config* g_hash_config;

extern pthread_mutex_t g_mutexBitmap;

extern pthread_mutex_t g_mutexHashIndex;
/**
 * @brief Inicializa los semaforos usados en storage, por ahora solo el de bitmap
 */
void inicializarSemaforos();

/**
 * @brief Muestra el bitmap actual en el log, en modo debug
 */
void mostrarBitmap();
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
 * @param tamanio el tamanio que debe tener el archivo, 0 si no sabemos
 * @return void
 */
void crearArchivo(char* path, off_t tamanio);

/**
 * @brief Usa los valores del archivo superblock.config para crear los bloques fisicos necesarios. 
 * @return void
 */
void crearBloques();

/**
 * @brief Escribe el archivo metadata.config en la ruta especificada
 * @param path la ruta del archivo metadata.config
 * @param tamanio el tamanio del archivo
 * @param blocks el array de bloques asignados al archivo
 * @param cantidadBloques la cantidad de bloques asignados al archivo
 * @param estado el estado del archivo
 * @return void
 */
void escribirMetadataConfig(char* path, int tamanio, int* blocks, int cantidadBloques, char* estado);

/**
 * @brief Escribe datos en el bloque especificado
 * @param numeroBloque el numero de bloque donde se escribiran los datos
 * @param datos los datos a escribir
 * @param tamanioDatos el tamanio de los datos a escribir
 * @return void
 */
void escribirBloque(int numeroBloque, void* datos, size_t tamanioDatos);

/**
 * @brief Inicializa el t_bitarray, creando el archivo bitmap.bin en la ruta especificada y mapeandolo a memoria.
 * @param path la ruta donde se encuentra el archivo bitmap.bin
 * @return void
 */
void inicializarBitmap(char* path);

/**
 * @brief Inicializa la variable t_config* g_hash_config con el archivo blocks_hash_index.config
 */
void inicializarHashIndex();

/**
 * @brief calcula el hash del bloque especificado en el archivo blocks_hash_index.config
 * @param numeroBloque el numero de bloque del cual se calculara el hash
 * @return el hash calculado en formato string
 */
char* calcularHash(int numeroBloque);

/**
 * @brief Escribe el hash del bloque especificado en el g_hash_config
 * @param numeroBloque el numero de bloque del cual se escribira el hash
 */
void escribirHashindex(int numeroBloque);

/**
 * @brief Crea un hard link para el archivo especificado en el tag especificado
 * @param nombreFile el nombre del file
 * @param nombreTag el nombre del tag
 */
void crearHardLink(char* nombreFile, char* nombreTag);

/**
 * @brief Inicializa el punto de montaje. Si no existe lo crea. Si existe lo borra y crea uno nuevo. 
 * @param path el path del punto de montaje
 * @return void
 */
void inicializarPuntoMontaje(char* path);
#endif