#ifndef QC_FUNCIONES_H
#define QC_FUNCIONES_H

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>


/**
* @brief Genera un handshake entre query control y master
* @param archivo_configuracion el nombre del archivo de configuracion
* @param prioridad la prioridad de la query
* @return el *t_handshake_qc_master resultante
*/
t_handshake_qc_master* generarHandshake(char* archivo_configuracion, int prioridad);


/**
* @brief Genera un paquete con el handshake entre query control y master
* @param handshake el *t_handshake_qc_master a serializar
* @return el *t_paquete resultante
*/
t_paquete* generarPaquete(t_handshake_qc_master*);

/**
* @brief Confirma la recepcion del handshake entre query control y master
* @param conexion_master la conexion con el master
* @return void
*/
void confirmarRecepcion (int conexion_master);

/**
* @brief Libera la memoria de la confirmacion y el handshake
* @param handshake el *t_handshake_qc_master a liberar  
* @return void
*/
void limpiarMemoria(t_handshake_qc_master*);

#endif