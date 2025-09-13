#ifndef CONEXION_H
#define CONEXION_H

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

void atender_query_control(t_buffer*);
void atender_worker(t_buffer*);

#endif