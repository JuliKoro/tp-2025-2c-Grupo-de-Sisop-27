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
typedef enum {
    HANDSHAKE_QC_MASTER,
    HANDSHAKE_WORKER_STORAGE,
    HANDSHAKE_WORKER_MASTER,
    OP_ASIGNAR_QUERY, // Asignación de nueva Query
    OP_DESALOJAR_QUERY, // Solicitud de desalojo
    OP_FIN_QUERY, // Notificación de fin de Query
    OP_SOL_INSTRUCCION, // Solicitud de instrucción (Worker->Storage)
    OP_RESP_INSTRUCCION, // Respuesta de instrucción (Storage->Worker)
    //INST_CREATE,
    //INST_TRUNCATE,
    //INST_WRITE,
    //INST_READ,
    //INST_TAG,
    //INST_COMMIT,
    //INST_FLUSH,
    //INST_DELETE,
    //INST_END,
    //INST_UNKNOWN
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
    char* archivo_query;
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

/**
 * @brief Estructura para enviar el tamaño de página/bloque
 * @param tam_pagina el tamaño de la página en bytes - uint32_t
 * @note Utilizado en el handshake entre worker y storage
 */
typedef struct {
    uint32_t tam_pagina;      // Tamaño de cada página en bytes
} t_tam_pagina;

/**
 * @brief Estructura para la asigancion de una nueva query
 * @param id_query el ID de la query
 * @param path_query el path de la query - char*
 * @param pc el valor inicial del program counter - uint32_t
 * @note Utilizado para caundo el master asigna una nueva query al worker
 */
typedef struct {
    uint32_t id_query;   // ID de la query
    char* path_query;  // PATH (string, serializado con longitud)
    uint32_t pc;       // PC (binario, 4 bytes)
} t_asignacion_query;

// ESTRUCTURAS PARA SOLICITUD DE INSTRUCCIONES (WORKER - STORAGE)


/***********************************************************************************************************************/
/***                                   ESTRUCTURAS PARA INSTRUCCIONES WORKER->STORAGE                                    ***/
/***********************************************************************************************************************/

// ============================================================================
// TIPOS DE INSTRUCCIONES
// ============================================================================

/**
 * @brief Enumeración de los tipos de instrucciones soportadas
 */
typedef enum {
    INST_CREATE,
    INST_TRUNCATE,
    INST_WRITE,
    INST_READ,
    INST_TAG,
    INST_COMMIT,
    INST_FLUSH,
    INST_DELETE,
    INST_END,
    INST_UNKNOWN
} t_tipo_instruccion;

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * @brief Estructura que representa una instrucción parseada
 */
typedef struct {
    t_tipo_instruccion tipo;
    char* file_name;           // Nombre del File
    char* tag_name;            // Nombre del Tag
    char* file_name_dest;      // Nombre del File destino (para TAG)
    char* tag_name_dest;       // Nombre del Tag destino (para TAG)
    uint32_t direccion_base;   // Dirección base (para WRITE/READ)
    uint32_t tamanio;          // Tamaño (para TRUNCATE/READ)
    char* contenido;           // Contenido (para WRITE)
    char* instruccion_raw;     // Instrucción completa sin parsear
} t_instruccion;

/**
 * Create: crear File:Tag
 */
typedef struct {
    char* file_name;
    char* tag_name;
} t_create;

/**
 * Truncate: truncar archivo a tamaño
 */
typedef struct {
    char* file_name;
    char* tag_name;
    uint32_t size;
} t_truncate;

/**
 * Write: escribir contenido en un offset
 */
typedef struct {
    char* file_name;
    char* tag_name;
    uint32_t offset;
    uint32_t size;
    void* content;  // puntero buffer de datos
} t_write;

/**
 * Read: leer contenido desde offset tamaño
 */
typedef struct {
    char* file_name;
    char* tag_name;
    uint32_t offset;
    uint32_t size;
} t_read;

/**
 * Tag: crear tag temporal (o permanente)
 */
typedef struct {
    char* file_name_origen;
    char* tag_name_origen;
    char* file_name_destino;
    char* tag_name_destino;
} t_tag;

/**
 * Commit: confirmar un tag
 */
typedef struct {
    char* file_name;
    char* tag_name;
} t_commit;

/**
 * Flush: enviar todos los cambios de File:Tag a FS
 */
typedef struct {
    char* file_name;
    char* tag_name;
} t_flush;

/**
 * Delete: eliminar tag o file
 */
typedef struct {
    char* file_name;
    char* tag_name;
} t_delete;

/**
 * @brief Estructura genérica para solicitud de instruccion serializada entre Worker y Storage.
 */
typedef struct {
    t_tipo_instruccion tipo; /**< Tipo de instrucción a ejecutar */
    uint32_t longitud_datos;         /**< Longitud en bytes del buffer de datos */
    void* datos;                     /**< Puntero al buffer serializado con datos específicos */
} t_solicitud_instruccion;

#endif
