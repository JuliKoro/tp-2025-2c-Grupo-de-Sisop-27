#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include "conexion.h"

int main(int argc, char* argv[]) {
    saludar("master");

    master_conf* master_conf = get_configs_master("master.config");
    printf("PUERTO_ESCUCHA: %d\n", master_conf->puerto_escucha);
    printf("ALGORITMO_PLANIFICACION: %s\n", master_conf->algoritmo_planificacion);
    printf("TIEMPO_AGING: %d\n", master_conf->tiempo_aging);
    printf("LOG_LEVEL: %s\n", master_conf->log_level);
    
    char puerto_escucha[10];
    sprintf(puerto_escucha, "%d", master_conf->puerto_escucha);
    int socket_servidor = iniciar_servidor(puerto_escucha);

    if(socket_servidor == -1){
        fprintf(stderr, "Error al iniciar el servidor\n");
        return EXIT_FAILURE;
    }

    while (1) {
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
            case HANDSHAKE_QC_MASTER:
                thread_args = malloc(sizeof(t_thread_args));
                thread_args->paquete = paquete;
                thread_args->fd_conexion = fd_conexion_ptr;
                pthread_create(&thread, NULL, atender_query_control, (void*) thread_args);
                pthread_detach(thread);
                break;
            case HANDSHAKE_WORKER_MASTER:
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