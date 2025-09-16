#ifndef QC_FUNCIONES_H
#define QC_FUNCIONES_H

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include <utils/loggeo.h>

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

#endif