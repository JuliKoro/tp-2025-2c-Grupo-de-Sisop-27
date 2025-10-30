#include "w_conexiones.h"
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

t_tam_pagina* handshake_worker_storage(int socket_storage, int id_worker) {
    // Hanshake con Storage para recibir el tamaño de página
    if(enviar_entero(socket_storage, id_worker) == -1){
        fprintf(stderr, "Error al enviar el ID del worker en el handshake con Storage.\n");
        return NULL;
    }

    log_debug(logger_worker, "Handshake enviado al Storage con ID Worker: %d", id_worker);
    log_debug(logger_worker, "Esperando recibir tamaño de página desde Storage...");

    // Recibo la confirmacion de Storage con un paquete t_tam_pagina
    t_tam_pagina* tam_pagina_struct;
    t_paquete* paquete = recibir_paquete(socket_storage);
    if (paquete->codigo_operacion != HANDSHAKE_WORKER_MASTER) {
        fprintf(stderr, "Error: Código de operación inesperado en el handshake con Storage.\n");
        destruir_paquete(paquete);
        return NULL;
    }
    tam_pagina_struct = deserializar_tam_pagina(paquete->datos);
    destruir_paquete(paquete);
    return tam_pagina_struct;
}