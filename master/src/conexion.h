#ifndef CONEXION_H
#define CONEXION_H

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include <pthread.h>


/**
 * @brief Inicia el hilo que atiende las conexiones entrantes, esto es para que el main pueda seguir con su logica
 */
void* iniciar_receptor(void* socket_servidor);

extern bool desconexion_query;

/**
 * @brief Atiende la conexion de un query control
 * @param thread_args
 * @return void
 */
void* atender_query_control(void* thread_args);

/**
 * @brief Atiende la conexion de un worker
 * @param thread_args
 * @return void
 */
void* atender_worker(void* thread_args);

#endif