#ifndef WORKER_H_
#define WORKER_H_

#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#include <commons/string.h>

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

#include "w_funciones.h"
#include "registros.h"

void inicializacion_worker(char* nombre_config, char* id_worker_str);

int conexiones_worker();

void* hilo_master(void* arg);

void* hilo_query_interpreter(void* arg);

#endif