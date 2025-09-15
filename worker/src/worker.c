#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include <string.h>
#include "funciones.h"

t_log* logger_worker = NULL;

int main(int argc, char* argv[]) {
    saludar("worker");

    
    if(argc < 3){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion> <id_worker>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* nombre_config = argv[1];
    uint32_t id_worker = atoi(argv[2]);

    worker_conf* worker_conf = get_configs_worker(nombre_config);
    char puerto_storage[10];
    char puerto_master[10];
    sprintf(puerto_storage, "%d", worker_conf->puerto_storage);
    sprintf(puerto_master, "%d", worker_conf->puerto_master);

    //Iniciar logger
    logger_worker = iniciarLoggerWorker(argv[2], worker_conf->log_level);

    log_info(logger_worker, "Iniciado worker %d", id_worker);
 
    

    // Conectar con Storage.
    int conexion_storage = crear_conexion(worker_conf->ip_storage, puerto_storage);
    if(conexion_storage == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }

    t_handshake_worker_storage* handshakeStorage = generarHandshakeStorage(id_worker);
    
    t_paquete* paquete = generarPaqueteStorage(HANDSHAKE_WORKER_STORAGE, handshakeStorage);
    
    enviar_paquete(conexion_storage, paquete);

    confirmarRecepcion(conexion_storage);
    
    limpiarMemoriaStorage(handshakeStorage);

    // Conectar con Master.
    int conexion_master = crear_conexion(worker_conf->ip_master, puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }
    t_handshake_worker_master* handshakeMaster = generarHandshakeMaster(id_worker);

    paquete = generarPaqueteMaster(HANDSHAKE_WORKER_MASTER, handshakeMaster);
    
    enviar_paquete(conexion_master, paquete);

    confirmarRecepcion(conexion_master);
    
    limpiarMemoriaMaster(handshakeMaster);

    

    return 0;
}

