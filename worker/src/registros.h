/**
 * @file registros.h
 * @brief Registros y flags del módulo Worker
 * 
 * Este archivo define los registros y flags globales utilizados
 * para la ejecución de queries en el Worker.
 * 
 * Componentes principales:
 * - **Registros**: Variables que mantienen el estado de ejecución de la query actual
 *   - PC (Program Counter): Indica la instrucción actual a ejecutar
 *   - Path de Query: Ruta del archivo de la query en ejecución
 * 
 * - **Flags**: Variables booleanas que controlan el flujo de ejecución
 *   - query_en_ejecucion: Indica si hay una query activa
 *   - desalojar_query: Señal para desalojar la query actual
 * 
 * @warning Estas variables globales deben ser protegidas con mutex en entornos
 *          multihilo para evitar condiciones de carrera
 * 
 * @author Grupo de Sisop 27
 * @date 2C2025
 */

#ifndef REGISTROS_H_
#define REGISTROS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include <commons/log.h>

#include <utils/configs.h>
#include <utils/estructuras.h>

// VARIABLES GLOBALES

/**
 * @brief Logger global del módulo Worker
 * 
 * @note Esta variable es compartida entre todos los archivos del Worker
 */
extern t_log* logger_worker;

/**
 * @brief Variable global para el ID del Worker
 */
extern uint32_t id_worker;                    // ID del Worker

// REGISTROS

/**
 * @brief Variable global para el PC (Program Counter)
 * 
 * Registro que mantiene el contador de programa, indicando la próxima
 * instrucción a ejecutar de la query actual.
 * 
 * @note Si se usan hilos, hay que proteger la variable con mutex
 */
extern uint32_t pc_actual;

/**
 * @brief Variable global para el path de la Query actual
 * 
 * Registro que almacena la ruta del archivo de la query que está siendo
 * ejecutada actualmente.
 * 
 * @note Si se usan hilos, hay que proteger la variable con mutex
 * @note Debe liberarse la memoria cuando se termine de usar
 */
extern char* path_query;

/**
 * @brief Variable global para el ID de la Query actual
 * 
 * Registro que almacena el identificador único de la query que está siendo
 * ejecutada actualmente.
 * 
 * @note Si se usan hilos, hay que proteger la variable con mutex
 */
extern uint32_t id_query;

// FLAGS
/**
 * @brief Flag que indica si hay una query en ejecución
 * 
 * Variable booleana que señala si el Worker está ejecutando actualmente
 * una query. Es utilizada para evitar asignaciones múltiples y coordinar
 * la ejecución entre hilos.
 * 
 * @note Debe ser protegida con mutex en entornos multihilo
 */
extern volatile bool query_en_ejecucion; // Flag: hay una Query activa?

/**
 * @brief Flag para solicitar el desalojo de la query actual
 * 
 * Variable booleana que indica si se debe desalojar la query en ejecución.
 * Es chequeada por el hilo_query_interpreter para detener la ejecución
 * cuando el Master solicita un desalojo.
 * 
 * @note Debe ser protegida con mutex en entornos multihilo
 */
 extern volatile bool desalojar_query;             // Flag para desalojo (chequeada en interpreter)
//bool hay_query_activa = false;            // Flag para saber si hay una query activa (chequeada en master)
#endif
