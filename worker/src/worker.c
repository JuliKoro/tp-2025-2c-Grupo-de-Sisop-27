#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include "funciones.h"



int main(int argc, char* argv[]) {
    saludar("worker");

    if(argc < 3){
        fprintf(stderr, "Error al iniciar modulo worker.\n");
        return EXIT_FAILURE;
    }

    char* nombre_config = argv[1];
    char* id_worker = argv[2];

    char* confirmacion = malloc(sizeof(char));


    worker_conf* worker_conf = get_configs_worker(nombre_config);

    printf("WORKER INICIALIZADO: %s", id_worker);

    

    // Conectar con Storage.
    int conexion_storage = crear_conexion(worker_conf->ip_storage, worker_conf->puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }

    t_handshake_worker_storage* handshakeStorage = generarHandshakeStorage(id_worker);
    
    t_paquete* paquete = generarPaqueteStorage(HANDSHAKE_WORKER_STORAGE, handshakeStorage);
    
    enviar_paquete(conexion_storage, paquete);

    confirmacion = confirmarRecepcion(conexion_storage);
    
    limpiarMemoriaStorage(confirmacion, handshakeStorage);

    // Conectar con Master.
    int conexion_master = crear_conexion(worker_conf->ip_storage, worker_conf->puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }
    t_handshake_worker_master* handshakeMaster = generarHandshake(id_worker);

    t_paquete* paquete = generarPaqueteMaster(HANDSHAKE_WORKER_STORAGE, handshakeMaster);
    
    enviar_paquete(conexion_master, paquete);

    confirmacion = confirmarRecepcion(conexion_master);
    
    limpiarMemoriaMaster(confirmacion, handshakeMaster);

    

    return 0;
}

