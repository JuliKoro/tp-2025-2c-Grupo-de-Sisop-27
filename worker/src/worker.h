/**
 * @file worker.h
 * @brief Módulo Worker - Nodo de ejecución de queries
 * 
 * Este archivo contiene las definiciones principales del módulo Worker, que actúa
 * como un nodo de procesamiento en el sistema distribuido. El Worker es responsable de:
 * 
 * - Gestionar memoria interna con tabla de páginas (algoritmos LRU y CLOCK-M)
 * - Recibir y ejecutar queries asignadas por el módulo Master
 * - Comunicarse con el módulo Storage para operaciones de lectura/escritura
 * - Manejar múltiples hilos para procesamiento concurrente
 * - Administrar el ciclo de vida de las queries (asignación, ejecución, desalojo)
 * 
 * El archivo fuente worker.c contiene la función main() que inicializa el módulo,
 * establece las conexiones necesarias y crea los hilos de ejecución para:
 * - Recibir instrucciones del Master (hilo_master)
 * - Ejecutar el intérprete de queries (hilo_query_interpreter)
 * 
 * @author Grupo de Sisop 27
 * @date 2C2025
 */

#ifndef WORKER_H_
#define WORKER_H_

#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#include <commons/string.h>

#include "w_conexiones.h"
#include "registros.h"
#include "tabla_de_paginas.h"
#include "memoria_interna.h" 
#include "query_interpreter.h"

/**
 * @brief Inicializa el módulo Worker
 * 
 * @param nombre_config Nombre del archivo de configuración (ej: "worker.config")
 * @param id_worker_str ID del Worker en formato string
 * 
 * @note Esta función termina el programa si falla la carga de configuración
 */
void inicializar_worker(char* nombre_config, char* id_worker_str);

/**
 * @brief Hilo que maneja la comunicación con el módulo Master
 * 
 * Este hilo se ejecuta continuamente esperando mensajes del Master, que pueden ser:
 * - OP_ASIGNAR_QUERY: Asigna una nueva query para ejecutar
 * - OP_DESALOJO_QUERY: Solicita desalojar la query actual
 * - OP_FIN_QUERY: Notifica la finalización de una query
 * 
 * El hilo actualiza el estado del Worker (PC, path de query, flags) según
 * las instrucciones recibidas.
 * 
 * @param arg Argumento del hilo (no utilizado)
 * @return NULL
 * 
 * @note Este hilo corre en un bucle infinito hasta que se cierre la conexión
 */
void* hilo_master(void* arg);

/**
 * @brief Hilo que ejecuta el Query Interpreter
 * 
 * Este hilo se encarga de parsear y ejecutar las instrucciones de la query
 * asignada. Trabaja en conjunto con el hilo_master para coordinar la ejecución.
 * 
 * @param arg Argumento del hilo (no utilizado)
 * @return NULL
 * 
 * @note Este hilo corre en un bucle infinito procesando queries
 */
void* hilo_query_interpreter(void* arg);

/**
 * @brief Desaloja la query actual
 * 
 * Realiza las operaciones necesarias para desalojar la query en ejecución:
 * - Hace flush de todas las páginas modificadas a Storage
 * - Limpia el estado del Worker (PC, ID de query, flags)
 */
void desalojar_query();

/**
 * @brief Finaliza el módulo Worker
 * 
 * Realiza la limpieza de recursos al finalizar la ejecución del Worker:
 * - Destruye semáforos y mutex
 * - Cierra conexiones de red
 * - Libera memoria utilizada
 * - Cierra el logger
 * - Libera configuraciones cargadas
 */
void finalizar_worker();

/**
 * @brief Función de prueba para ejecutar el Query Interpreter manualmente
 * 
 * Configura los registros necesarios (path, id, pc) hardcodeados y lanza
 * la ejecución de una query para probar la integración con Storage
 * sin necesidad de intervención del Master.
 * 
 * @param test_path Nombre del archivo de query a probar (ej: "AGING_1")
 */
void test_query_interpreter_con_storage(char* test_path);

/**
 * @brief Test de integración Worker-Storage con salida por consola
 * 
 * Conecta solo al Storage, simula al Master y ejecuta el script paso a paso,
 * imprimiendo el resultado de cada instrucción por pantalla.
 * 
 * @param nombre_archivo Nombre del script a probar (ej: "SCRIPT_1")
 */
void test_integracion_storage_consola(char* nombre_archivo);

#endif
