#ifndef CONEXION_H
#define CONEXION_H

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include <pthread.h>
#include "s_funciones.h"
#include <utils/estructuras.h>

/**
 * @brief Atiende la conexion de un worker
 * @param thread_args
 * @return void
 */
void* atender_worker(void* thread_args);

#endif