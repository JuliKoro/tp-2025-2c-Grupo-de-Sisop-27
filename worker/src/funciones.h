#ifndef QC_FUNCIONES_H
#define QC_FUNCIONES_H

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

t_handshake_worker_storage* generarHandshakeStorage(char*);
t_handshake_worker_master* generarHandshakeMaster(char*);
t_paquete* generarPaqueteStorage(int, t_handshake_worker_storage*);
t_paquete* generarPaqueteMaster(int, t_handshake_worker_master*);
void limpiarMemoriaStorage(char*, t_handshake_worker_storage*);
void limpiarMemoriaMaster(char*, t_handshake_worker_master*);
void confirmarRecepcion (int);

#endif