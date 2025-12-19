#include "w_conexiones.h"

int conexiones_worker(){
    //SOCKETS
    char puerto_storage[10];
    char puerto_master[10];
    sprintf(puerto_storage, "%d", worker_configs->puerto_storage);
    sprintf(puerto_master, "%d", worker_configs->puerto_master);

    // Conectar con Storage.
    conexion_storage = crear_conexion(worker_configs->ip_storage, puerto_storage);
    if(conexion_storage == -1){
        log_error(logger_worker, "Error al conectar con el modulo storage.");
        return EXIT_FAILURE;
    }

    // Handshake con Storage para recibir el tamaño de página
    t_tam_pagina* tam_pagina = handshake_worker_storage(conexion_storage, id_worker);
    if(tam_pagina == NULL){
        log_error(logger_worker, "Error en el handshake con Storage.");
        return EXIT_FAILURE;
    }
    worker_configs->tam_pagina = tam_pagina->tam_pagina;
    log_debug(logger_worker, "Handshake con Storage exitoso. Tamaño de página recibido: %d bytes", tam_pagina->tam_pagina);
    free(tam_pagina);

    // Conectar con Master.
    conexion_master = crear_conexion(worker_configs->ip_master, puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }
    // Handshake con Master
    t_handshake_worker_master* handshakeMaster = generarHandshakeMaster(id_worker);

    t_paquete* paquete = generarPaqueteMaster(HANDSHAKE_WORKER_MASTER, handshakeMaster);
    
    enviar_paquete(conexion_master, paquete);

    confirmarRecepcion(conexion_master);
    
    limpiarMemoriaMaster(handshakeMaster);

    return 0;
}

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
    // TODO: CAMBIARO EN conexiones_worker() tmb
    t_handshake_worker_storage* handshake = generarHandshakeStorage(id_worker);
    t_paquete* paquete_handshake = generarPaqueteStorage(HANDSHAKE_WORKER_STORAGE, handshake);

    if(enviar_paquete(socket_storage, paquete_handshake) == -1){
        fprintf(stderr, "Error al enviar el ID del worker en el handshake con Storage.\n");
        return NULL;
    }

    log_debug(logger_worker, "Handshake enviado al Storage con ID Worker: %d", id_worker);
    log_debug(logger_worker, "Esperando recibir tamaño de página desde Storage...");

    // Recibo la confirmacion de Storage con un paquete t_tam_pagina
    t_tam_pagina* tam_pagina_struct;
    t_paquete* paquete = recibir_paquete(socket_storage);
    if (paquete->codigo_operacion != HANDSHAKE_WORKER_STORAGE) {
        fprintf(stderr, "Error: Código de operación inesperado en el handshake con Storage.\n");
        destruir_paquete(paquete);
        return NULL;
    }
    tam_pagina_struct = deserializar_tam_pagina(paquete->datos);
    destruir_paquete(paquete);
    return tam_pagina_struct;
}

