#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <stdint.h>

/***********************************************************************************************************************/
/***                                            ESTRUCTURA DEL PAQUETE                                               ***/
/***********************************************************************************************************************/

/** 
* @brief Codigos de operacion 
* @param HANDSHAKE_QC_MASTER - Handshake entre query control y master
* 
*/
typedef enum{
    HANDSHAKE_QC_MASTER,
    HANDSHAKE_WORKER_STORAGE,
    HANDSHAKE_WORKER_MASTER
} e_codigo_operacion;

/**
* @brief Estructura para el buffer
* @param size el tamaño del buffer - uint32_t
* @param offset el offset del buffer - uint32_t
* @param stream el stream de bytes - void*
*/
typedef struct {
    uint32_t size;
    uint32_t offset;
    void* stream;
} t_buffer;

/**
* @brief Estructura para el paquete
* @param codigo_operacion - Codigo de operacion - e_codigo_operacion
* @param datos - Datos del paquete, un stream de bytes - t_buffer*
*/
typedef struct {
    e_codigo_operacion codigo_operacion;
    t_buffer* datos;
} t_paquete;

/***********************************************************************************************************************/
/***                                        ESTRUCTURAS DE DATOS                                                      ***/
/***********************************************************************************************************************/

/**
 * @brief Estructura para los argumentos de las funciones de atencion de conexiones. 
 * @param paquete el paquete handshake recibido
 * @param fd_conexion el fd del socket de conexion con el modulo
 */
typedef struct {
    t_paquete* paquete;
    int* fd_conexion;
} t_thread_args;

/**
* @brief Estructura para el handshake entre query control y master
* @param archivo_configuracion el nombre del archivo de configuracion - char*
* @param prioridad la prioridad de la query - int
*/
typedef struct {
    char* archivo_configuracion;
    uint32_t prioridad;
} t_handshake_qc_master;

/**
 * @brief Estructura para el handshake entre worker y storage
 * @param id_worker el id del worker - uint32_t
 */
typedef struct {
    uint32_t id_worker;
} t_handshake_worker_storage;

/**
 * @brief Estructura para el handshake entre worker y master
 * @param id_worker el id del worker - uint32_t
 */
typedef struct {
    uint32_t id_worker;
} t_handshake_worker_master;



#endif