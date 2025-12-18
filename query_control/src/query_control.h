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

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

#include "qc_funciones.h"

/**
 * @brief Logger del módulo Query Control
 */
extern t_log* logger_qc;

/**
 * @brief Configuración del módulo Query Control
 */
extern query_control_conf* qc_configs;


#endif /* QC_H_ */