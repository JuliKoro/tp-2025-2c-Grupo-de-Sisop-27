/**
 * @file w_conexiones.h
 * @brief Módulo de conexiones del Worker (Scokets)
 * 
 * Este archivo contiene las funciones necesarias para establecer y gestionar
 * las conexiones del Worker con los módulos Storage y Master del sistema.
 * 
 * Funcionalidades principales:
 * - Generación de handshakes para establecer comunicación inicial
 * - Serialización de estructuras de handshake en paquetes
 * - Gestión de memoria de las estructuras de comunicación
 * - Confirmación de recepción de mensajes
 * 
 * El archivo fuente w_conexiones.c implementa estas funciones que son utilizadas
 * durante la inicialización del Worker para establecer las conexiones con:
 * - Storage: Para operaciones de lectura/escritura de datos
 * - Master: Para recibir asignaciones y comandos de queries
 * 
 * @author Grupo de Sisop 27
 * @date 2C2025
 */

#ifndef W_CONEXIONES_H
#define W_CONEXIONES_H

#include <utils/hello.h>
#include <utils/mensajeria.h>

#include "registros.h"

/**
 * @brief Socket de conexión con el módulo Storage
 */
extern int conexion_storage;

/**
 * @brief Socket de conexión con el módulo Master
 */
extern int conexion_master;

/**
 * @brief Establece las conexiones del Worker con otros módulos
 * 
 * Crea y configura las conexiones de red con los módulos Storage y Master.
 * Realiza el handshake inicial con ambos módulos para establecer la comunicación.
 * 
 * @return 0 si las conexiones se establecieron correctamente
 * @return EXIT_FAILURE si ocurre algún error en las conexiones
 * 
 * @note Utiliza las IPs y puertos configurados en el archivo de configuración
 */
int conexiones_worker();

/**
 * @brief Logger global del módulo Worker
 * 
 * @note Esta variable es compartida entre todos los archivos del Worker
 */
extern t_log* logger_worker;

/**
 * @brief Genera un handshake entre worker y storage
 * @param id_worker el id del worker
 * @return el *t_handshake_worker_storage resultante
 */
t_handshake_worker_storage* generarHandshakeStorage(uint32_t id_worker);

/**
 * @brief Genera un handshake entre worker y master
 * @param id_worker el id del worker
 * @return el *t_handshake_worker_master resultante
 */
t_handshake_worker_master* generarHandshakeMaster(uint32_t id_worker);

/**
 * @brief Genera un paquete con el handshake entre worker y storage
 * @param codigoOperacion el codigo de operacion
 * @param handshake el *t_handshake_worker_storage a serializar
 * @return el *t_paquete resultante
 */
t_paquete* generarPaqueteStorage(e_codigo_operacion, t_handshake_worker_storage*);

/**
 * @brief Genera un paquete con el handshake entre worker y master
 * @param codigoOperacion el codigo de operacion
 * @param handshake el *t_handshake_worker_master a serializar
 * @return el *t_paquete resultante
 */
t_paquete* generarPaqueteMaster(e_codigo_operacion codigoOperacion, t_handshake_worker_master* handshake);

/**
 * @brief Libera la memoria del handshake
 * @param handshake el *t_handshake_worker_storage a liberar  
 * @return void
 */
void limpiarMemoriaStorage(t_handshake_worker_storage* handshake);

/**
 * @brief Libera la memoria del handshake
 * @param handshake el *t_handshake_worker_master a liberar  
 * @return void
 */
void limpiarMemoriaMaster(t_handshake_worker_master* handshake);

/**
 * @brief Confirma la recepcion del handshake
 * @param socket_conexion la conexion con el modulo
 * @return void
 */
void confirmarRecepcion (int socket_conexion);

/**
 * @brief Realiza el handshake entre worker y storage
 * @param socket_storage el socket de conexion con storage
 * @param id_worker el id del worker
 * @return el *t_tam_pagina resultante
 */
t_tam_pagina* handshake_worker_storage(int socket_storage, int id_worker);

#endif