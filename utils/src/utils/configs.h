#ifndef CONFIGS_H
#define CONFIGS_H

#include <commons/config.h>
#include <commons/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** 
    * @brief Estructura para la configuracion del modulo query control
    * @param ip_master - IP del master
    * @param puerto_master - Puerto del master
    * @param log_level - Nivel de log
*/
typedef struct {
    char* ip_master;
    int puerto_master;
    char* log_level;
} query_control_conf;

/**     
    * @brief Estructura para la configuracion del modulo master
    * @param puerto_escucha - Puerto de escucha
    * @param algoritmo_planificacion - Algoritmo de planificacion
    * @param tiempo_aging - Tiempo de aging
    * @param log_level - Nivel de log
*/
typedef struct {
    int puerto_escucha;
    char* algoritmo_planificacion;
    int tiempo_aging;
    char* log_level;
} master_conf;

/** 
    * @brief Estructura para la configuracion del modulo worker
    * @param ip_master - IP del master
    * @param puerto_master - Puerto del master
    * @param ip_storage - IP del storage
    * @param puerto_storage - Puerto del storage
    * @param tam_memoria - Tamanio de memoria
    * @param retardo_memoria - Retardo de memoria
    * @param algoritmo_reemplazo - Algoritmo de reemplazo
    * @param path_queries - Path de las queries
    * @param log_level - Nivel de log
*/
typedef struct {
    char* ip_master;
    int puerto_master;
    char* ip_storage;
    int puerto_storage;
    int tam_memoria;
    int retardo_memoria;
    char* algoritmo_reemplazo;
    char* path_queries;
    char* log_level;
} worker_conf;

/** 
    * @brief Estructura para la configuracion del modulo storage
    * @param puerto_escucha - Puerto de escucha
    * @param fresh_start - Fresh start
    * @param punto_montaje - Punto de montaje
    * @param retardo_operacion - Retardo de operacion
    * @param retardo_acceso_bloque - Retardo de acceso a bloque
    * @param log_level - Nivel de log
*/
typedef struct {
    int puerto_escucha;
    bool fresh_start;
    char* punto_montaje;
    int retardo_operacion;
    int retardo_acceso_bloque;
    char* log_level;
} storage_conf;

/**
 * @brief Estructura para la configuracion del superblock
 * @param fs_size - Tamaño del sistema de archivos
 * @param block_size - Tamaño de bloque
 */
typedef struct {
    int fs_size;
    int block_size;
} superblock_conf;

/**
 * @brief Estructura para la configuracion del metadata
 * @param tamanio - Tamaño del archivo
 * @param blocks - Bloques asignados al archivo, es un array de enteros
 * @param estado - Estado del archivo
 */
typedef struct {
    int tamanio;
    int* blocks;
    char* estado;
    int cantidad_blocks;
} metadata_conf;

/** 
    * @brief Esta funcion devuelve una estructura query_control_conf con la configuracion del modulo query control ya cargada
    * @param nombre_config - Nombre del archivo de configuracion
    * @return query_control_conf cargada con la configuracion del modulo query control
*/
 query_control_conf* get_configs_query_control(char* nombre_config);

/** 
    * @brief Esta funcion devuelve una estructura master_conf con la configuracion del modulo master ya cargada
    * @param nombre_config - Nombre del archivo de configuracion
    * @return master_conf cargada con la configuracion del modulo master
*/
 master_conf* get_configs_master(char* nombre_config);

/** 
    * @brief Esta funcion devuelve una estructura worker_conf con la configuracion del modulo worker ya cargada
    * @param nombre_config - Nombre del archivo de configuracion
    * @return worker_conf cargada con la configuracion del modulo worker
*/
 worker_conf* get_configs_worker(char* nombre_config);
 
/** 
    * @brief Esta funcion devuelve una estructura storage_conf con la configuracion del modulo storage ya cargada
    * @param nombre_config - Nombre del archivo de configuracion
    * @return storage_conf cargada con la configuracion del modulo storage
*/
 storage_conf* get_configs_storage(char* nombre_config);

/**
 * @brief Esta funcion devuelve una estructura superblock_conf con la configuracion del superblock ya cargada
 */
superblock_conf* get_configs_superblock(char* nombre_config);

/**
 * @brief Esta funcion devuelve una estructura metadata_conf con la configuracion del metadata ya cargada
 */
metadata_conf* get_configs_metadata(char* nombre_metadata);

/** 
    * @brief Esta funcion destruye la estructura query_control_conf, liberando la memoria asignada a los strings internos
    * @param query_control_conf - Estructura query_control_conf a destruir
*/
void destruir_configs_query_control(query_control_conf* query_control_conf);

/** 
    * @brief Esta funcion destruye la estructura master_conf, liberando la memoria asignada a los strings internos
    * @param master_conf - Estructura master_conf a destruir
*/
void destruir_configs_master(master_conf* master_conf);

/** 
    * @brief Esta funcion destruye la estructura worker_conf, liberando la memoria asignada a los strings internos
    * @param worker_conf - Estructura worker_conf a destruir
*/
void destruir_configs_worker(worker_conf* worker_conf);

/** 
    * @brief Esta funcion destruye la estructura storage_conf, liberando la memoria asignada a los strings internos
    * @param storage_conf - Estructura storage_conf a destruir
*/
void destruir_configs_storage(storage_conf* storage_conf);

/**
 * @brief Esta funcion destruye la estructura superblock_conf, liberando la memoria asignada a los strings internos
 */
void destruir_configs_superblock(superblock_conf* superblock_conf);

/**
 * @brief Esta funcion destruye la estructura metadata_conf, liberando la memoria asignada a los strings internos
 */
void destruir_configs_metadata(metadata_conf* metadata_conf);


#endif