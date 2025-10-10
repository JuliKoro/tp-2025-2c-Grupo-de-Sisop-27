#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include "conexion.h"
#include "s_funciones.h"
#include <commons/string.h>

t_log* logger_storage = NULL;

int main(int argc, char* argv[]) {
    saludar("master");

    if(argc < 3){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion> <nombre_archivo_superblock config>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char* nombre_archivo_configuracion = argv[1];
    char* nombre_archivo_superbock = argv[2];
    storage_conf* storage_conf = get_configs_storage(nombre_archivo_configuracion);
    printf("PUERTO_ESCUCHA: %d\n", storage_conf->puerto_escucha);
    printf("PUNTO_MONTAJE: %s\n", storage_conf->punto_montaje);
    printf("LOG_LEVEL: %s\n", storage_conf->log_level);

    

    char puerto_escucha[10];
    sprintf(puerto_escucha, "%d", storage_conf->puerto_escucha);
    int socket_servidor = iniciar_servidor(puerto_escucha);

    logger_storage = iniciarLoggerStorage(storage_conf->log_level);

    if(socket_servidor == -1){
        log_error(logger_storage, "Error al iniciar el servidor en el puerto %s", puerto_escucha);
        return EXIT_FAILURE;
    }

    //Logica de creacion de archivos y rutas
    log_debug(logger_storage, "Iniciando logica de creacion de archivos y rutas");
    log_debug(logger_storage, "PUNTO_MONTAJE: %s", storage_conf->punto_montaje);
    log_debug(logger_storage, "FRESH_START: %s", storage_conf->fresh_start?"true":"false");

    if(storage_conf->fresh_start){
        char* destino = string_duplicate(storage_conf->punto_montaje); //Hay que liberar destino
        string_append(&destino, "/superblock.config");
        log_debug(logger_storage, "Copiando superblock.config a %s", destino);
        inicializarPuntoMontaje(storage_conf->punto_montaje);
        copiarArchivo(nombre_archivo_superbock, destino);
    }



    while (1) {
        log_info(logger_storage, "Iniciando ciclo de espera de clientes");
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int));
        *fd_conexion_ptr = esperar_cliente(socket_servidor);
        if(*fd_conexion_ptr == -1){
            fprintf(stderr, "Error al esperar cliente\n");
            return EXIT_FAILURE;
        }
        t_paquete* paquete = recibir_paquete(*fd_conexion_ptr);
        if(paquete == NULL){
            fprintf(stderr, "Error al recibir paquete\n");
            return EXIT_FAILURE;
        }

        switch(paquete->codigo_operacion) {
            t_thread_args* thread_args;
            case HANDSHAKE_WORKER_STORAGE:
                thread_args = malloc(sizeof(t_thread_args));
                thread_args->paquete = paquete;
                thread_args->fd_conexion = fd_conexion_ptr;
                pthread_create(&thread, NULL, atender_worker, (void*) thread_args);
                pthread_detach(thread);
                break;
            default:
                fprintf(stderr, "Error: el paquete no es de tipo HANDSHAKE_<MODULO>_MASTER\n");
                destruir_paquete(paquete);
                return EXIT_FAILURE;
        }
    }
    return 0;
}
