/**
 * @file query_interpreter.h
 * @brief Query Interpreter del módulo Worker
 * 
 * Este módulo es responsable de interpretar y ejecutar las instrucciones
 * de las queries asignadas al Worker. Implementa el ciclo fetch-decode-execute
 * para procesar cada instrucción del archivo de query.
 * 
 * Instrucciones soportadas:
 * - CREATE: Crear un nuevo File:Tag
 * - TRUNCATE: Modificar el tamaño de un File:Tag
 * - WRITE: Escribir datos en memoria interna
 * - READ: Leer datos de memoria interna
 * - TAG: Crear un nuevo tag a partir de otro
 * - COMMIT: Confirmar cambios de un File:Tag
 * - FLUSH: Persistir cambios en Storage
 * - DELETE: Eliminar un File:Tag
 * - END: Finalizar la query
 * 
 * @author Grupo de Sisop 27
 * @date 2C2025
 */

#ifndef QUERY_INTERPRETER_H_
#define QUERY_INTERPRETER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#include "registros.h"
#include "w_conexiones.h"
#include "worker.h"

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
 * @brief Resultado de la ejecución de una instrucción
 */
typedef enum {
    EXEC_OK,              // Ejecución exitosa
    EXEC_ERROR,           // Error en la ejecución
    EXEC_DESALOJO,        // Se solicitó desalojo
    EXEC_FIN_QUERY        // Instrucción END ejecutada
} t_resultado_ejecucion;

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

/**
 * @brief Función principal del Query Interpreter
 * 
 * Ejecuta el ciclo fetch-decode-execute para interpretar y ejecutar
 * todas las instrucciones de la query actual, comenzando desde el PC actual.
 * 
 * @return EXEC_OK si terminó correctamente, EXEC_ERROR si hubo error,
 *         EXEC_DESALOJO si fue desalojada
 */
t_resultado_ejecucion query_interpreter();

// ============================================================================
// CICLO FETCH-DECODE-EXECUTE
// ============================================================================

/**
 * @brief Obtiene la siguiente instrucción del archivo de query
 * 
 * Lee la línea correspondiente al PC actual del archivo de query.
 * 
 * @param archivo Archivo de query abierto
 * @return String con la instrucción o NULL si hay error
 */
char* fetch_instruction(FILE* archivo);

/**
 * @brief Parsea una instrucción en string a estructura t_instruccion
 * 
 * Analiza la instrucción y extrae sus componentes (tipo, parámetros).
 * 
 * @param instruccion_str String con la instrucción
 * @return Puntero a t_instruccion parseada o NULL si hay error
 */
t_instruccion* parse_instruction(char* instruccion_str);

/**
 * @brief Ejecuta una instrucción parseada
 * 
 * Ejecuta la instrucción según su tipo, interactuando con memoria
 * interna y/o Storage según corresponda.
 * 
 * @param instruccion Instrucción a ejecutar
 * @return Resultado de la ejecución
 */
t_resultado_ejecucion execute_instruction(t_instruccion* instruccion);

// ============================================================================
// FUNCIONES DE EJECUCIÓN POR TIPO DE INSTRUCCIÓN
// ============================================================================

/**
 * @brief Ejecuta instrucción CREATE
 * @param file_name Nombre del File
 * @param tag_name Nombre del Tag
 * @return true si se ejecutó correctamente
 */
bool execute_create(char* file_name, char* tag_name);

/**
 * @brief Ejecuta instrucción TRUNCATE
 * @param file_name Nombre del File
 * @param tag_name Nombre del Tag
 * @param tamanio Nuevo tamaño del File:Tag
 * @return true si se ejecutó correctamente
 */
bool execute_truncate(char* file_name, char* tag_name, uint32_t tamanio);

/**
 * @brief Ejecuta instrucción WRITE
 * @param file_name Nombre del File
 * @param tag_name Nombre del Tag
 * @param direccion_base Dirección base donde escribir
 * @param contenido Contenido a escribir
 * @return true si se ejecutó correctamente
 */
bool execute_write(char* file_name, char* tag_name, uint32_t direccion_base, char* contenido);

/**
 * @brief Ejecuta instrucción READ
 * @param file_name Nombre del File
 * @param tag_name Nombre del Tag
 * @param direccion_base Dirección base desde donde leer
 * @param tamanio Cantidad de bytes a leer
 * @return true si se ejecutó correctamente
 */
bool execute_read(char* file_name, char* tag_name, uint32_t direccion_base, uint32_t tamanio);

/**
 * @brief Ejecuta instrucción TAG
 * @param file_origen Nombre del File origen
 * @param tag_origen Nombre del Tag origen
 * @param file_destino Nombre del File destino
 * @param tag_destino Nombre del Tag destino
 * @return true si se ejecutó correctamente
 */
bool execute_tag(char* file_origen, char* tag_origen, char* file_destino, char* tag_destino);

/**
 * @brief Ejecuta instrucción COMMIT
 * @param file_name Nombre del File
 * @param tag_name Nombre del Tag
 * @return true si se ejecutó correctamente
 */
bool execute_commit(char* file_name, char* tag_name);

/**
 * @brief Ejecuta instrucción FLUSH
 * @param file_name Nombre del File
 * @param tag_name Nombre del Tag
 * @return true si se ejecutó correctamente
 */
bool execute_flush(char* file_name, char* tag_name);

/**
 * @brief Ejecuta instrucción DELETE
 * @param file_name Nombre del File
 * @param tag_name Nombre del Tag
 * @return true si se ejecutó correctamente
 */
bool execute_delete(char* file_name, char* tag_name);

/**
 * @brief Ejecuta instrucción END
 * @return true siempre
 */
bool execute_end();

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * @brief Separa un string "FILE:TAG" en sus componentes
 * @param file_tag String en formato "FILE:TAG"
 * @param file_name Puntero donde guardar el nombre del File
 * @param tag_name Puntero donde guardar el nombre del Tag
 * @return true si se separó correctamente
 */
bool split_file_tag(char* file_tag, char** file_name, char** tag_name);

/**
 * @brief Libera la memoria de una instrucción
 * @param instruccion Instrucción a liberar
 */
void destruir_instruccion(t_instruccion* instruccion);

/**
 * @brief Convierte un tipo de instrucción a string (para logs)
 * @param tipo Tipo de instrucción
 * @return String con el nombre de la instrucción
 */
const char* tipo_instruccion_to_string(t_tipo_instruccion tipo);

/**
 * @brief Abre el archivo de query
 * @param path_query Path del archivo de query
 * @return Puntero al archivo o NULL si hay error
 */
FILE* abrir_archivo_query(char* path_query);

/**
 * @brief Cuenta la cantidad de líneas de un archivo
 * @param archivo Archivo a contar
 * @return Cantidad de líneas
 */
uint32_t contar_lineas_archivo(FILE* archivo);

#endif /* QUERY_INTERPRETER_H_ */
