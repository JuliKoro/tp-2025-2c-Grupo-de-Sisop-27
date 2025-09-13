#ifndef QC_FUNCIONES_H
#define QC_FUNCIONES_H

#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

t_paquete* generarPaquete(t_handshake_qc_master*);
t_handshake_qc_master* generarHandshake(void);
char* confirmarRecepcion (int);
void limpiarMemoria(char*, t_handshake_qc_master*);

#endif