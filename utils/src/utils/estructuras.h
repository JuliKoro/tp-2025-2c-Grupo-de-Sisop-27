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
    OP_flag_desalojo_query, // Solicitud de desalojo
    OP_FIN_QUERY, // Notificación de fin de Query
    OP_INST_OK, // Notificación de éxito
    OP_ERROR, // Notificación de error
    OP_CREATE,
    OP_TRUNCATE,
    OP_WRITE,
    OP_READ, // Tanto como para solicitud como para respuesta
    OP_TAG,
    OP_COMMIT,
    OP_FLUSH,
    OP_DELETE,
    OP_END,
    OP_RESULTADO_QUERY
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

/***********************************************************************************************************************/
/***                                   ESTRUCTURAS PARA INSTRUCCIONES WORKER->STORAGE                                    ***/
/***********************************************************************************************************************/

/**
 * @brief Resultado de la ejecución de una instrucción
 */
typedef enum {
    EXEC_DESALOJO = 2,        // Se solicitó desalojo
    EXEC_FIN_QUERY = 1,        // Instrucción END ejecutada
    EXEC_OK = 0,              // Ejecución exitosa
    EXEC_ERROR = -1,           // Error en la ejecución
    ERROR_FILE_NO_EXISTE = -2,
    ERROR_YA_EXISTE = -3,
    ERROR_ESPACIO_INSUFICIENTE = -4,
    ERROR_ESCRITURA_NO_PERMITIDA = -5, // Archivo en estado COMMITED
    ERROR_FUERA_DE_LIMITE = -6,
    ERROR_CONEXION = -7,
    ERROR_TAMANIO_INVALIDO = -8,
    ERROR_MEMORIA_INTERNA = -9
} t_resultado_ejecucion;

/***********************************************************************************************************************/
/***                                                      ESTRUCTURAS PARA QUERIES                                  ***/
/***********************************************************************************************************************/
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

/**
 * @brief Estructura para notificar el resultado de ejecución de una Query
 * @param id_query el ID de la query ejecutada - uint32_t
 * @param estado el estado final de la query - t_resultado_ejecucion
 * @param pc_final el valor del program counter donde terminó la ejecución - uint32_t
 * @param mensaje_error mensaje descriptivo del error (NULL si no hubo error) - char
 * @note Enviada desde Worker → Master → Query Control
 */
typedef struct {
    uint32_t id_query;                    // ID de la query ejecutada
    t_resultado_ejecucion estado;         // Estado final (EXEC_OK, EXEC_ERROR, etc.)
    uint32_t pc_final;                    // PC donde terminó (útil para desalojos)
    char* mensaje_error;                  // Mensaje descriptivo del error (NULL si OK)
} t_resultado_query;

// ============================================================================
// TIPOS DE INSTRUCCIONES
// ============================================================================

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

// SOLICITUDES MEMORIA INTERNA A STORAGE

/**
 * @brief Estructura para la solicitud de lectura de memoria interna
 */
typedef struct {
    char* file_name;
    char* tag_name;
    uint32_t numero_bloque;
    uint32_t tamanio;
} t_sol_read;

// Estructura para la respuesta de lectura de memoria interna
/**
 * @brief Estructura para la respuesta de lectura de memoria interna
 * @param contenido el contenido leído - void*
 * @param tamanio el tamaño del contenido leído - uint32_t
 */
typedef struct {
    void* contenido;
    uint32_t tamanio;
} t_bloque_leido;

/**
 * @brief Estructura para la solicitud de lectura de memoria interna
 */
typedef struct {
    char* file_name;
    char* tag_name;
    uint32_t numero_bloque;
    void* contenido;
    uint32_t tamanio;
} t_sol_write;

#endif
