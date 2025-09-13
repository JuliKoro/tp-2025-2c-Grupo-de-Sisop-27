#include "funciones.h"
#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

t_handshake_worker_storage* generarHandshakeStorage(uint32_t id_worker) {
    t_handshake_worker_storage* handshake = malloc(sizeof(*handshake));
    handshake->id_worker = id_worker;
    return handshake;
}

t_handshake_worker_master* generarHandshakeMaster(uint32_t id_worker) {
    t_handshake_worker_master* handshake = malloc(sizeof(*handshake));
    handshake->id_worker = id_worker;
    return handshake;
}

t_paquete* generarPaqueteStorage (e_codigo_operacion codigoOperacion, t_handshake_worker_storage* handshake) {
    t_paquete* paquete = malloc(sizeof(*paquete));
    paquete->codigo_operacion = codigoOperacion;
    paquete->datos = serializar_handshake_worker_storage(handshake);
    return paquete;
}

t_paquete* generarPaqueteMaster (e_codigo_operacion codigoOperacion, t_handshake_worker_master* handshake) {
    t_paquete* paquete = malloc(sizeof(*paquete));
    paquete->codigo_operacion = codigoOperacion;
    paquete->datos = serializar_handshake_worker_master(handshake);
    return paquete;
}

void limpiarMemoriaStorage(t_handshake_worker_storage* handshake) {
    free(handshake);
}

void limpiarMemoriaMaster(t_handshake_worker_master* handshake) {
    free(handshake);
}

void confirmarRecepcion (int socket_conexion) {
    char* confirmacion = recibir_string(socket_conexion);
    if(confirmacion == NULL){
        fprintf(stderr, "Error al recibir confirmacion de recepecion\n");
        return;
    }
    printf("Confirmacion de recepecion: %s\n", confirmacion);
    free(confirmacion);
    return;
}

