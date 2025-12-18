/**
 * @file query_control.h
 * @brief Módulo Query Control - Controlador de ejecución de queries
 * 
 * Este módulo se encarga de gestionar la ejecución de queries
 * en el sistema distribuido. Se comunica con el módulo Master
 * para recibir asignaciones de queries y reportar su estado.
 * 
 * @author Grupo de Sisop 27
 * @date 2C2025
 */

#ifndef QC_H_
#define QC_H_

#include "qc_funciones.h"

/**
 * @brief Inicializa el módulo Query Control
 * 
 * @param nombre_config Nombre del archivo de configuración
 * @param archivo_query Nombre del archivo de query a ejecutar
 * @param prioridad Prioridad de la query
 * @return EXIT_SUCCESS si la inicialización fue exitosa, EXIT_FAILURE en caso contrario
 */
int inicializar_qc(char* nombre_config, char* archivo_query, int prioridad);

#endif /* QC_H_ */