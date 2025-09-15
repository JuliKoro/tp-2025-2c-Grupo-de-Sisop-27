#ifndef LOGGEO_H
#define LOGGEO_H

#include <commons/log.h>
#include <stdint.h>

/**
 * @brief Inicia un logger para el modulo Query Control
 * @param archivo_query - Nombre del archivo de query
 * @param log_level - Nivel de log
 */
t_log* iniciarLoggerQC(char* archivo_query, char* log_level);


/**
 * @brief Inicia un logger para el modulo Master
 * @param log_level - Nivel de log
 */
t_log* iniciarLoggerMaster(char* log_level);

/**
 * @brief Inicia un logger para el modulo Worker
 * @param id_worker - ID del worker
 * @param log_level - Nivel de log
 */
t_log* iniciarLoggerWorker(char* id_worker, char* log_level);

/**
 * @brief Inicia un logger para el modulo Storage
 * @param log_level - Nivel de log
 */
t_log* iniciarLoggerStorage(char* log_level);

#endif