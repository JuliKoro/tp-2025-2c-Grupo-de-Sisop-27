#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

void atender_worker(t_buffer*);

int main(int argc, char* argv[]) {
    saludar("master");

    storage_conf* storage_conf = get_configs_storage("storage.config")
    printf("PUERTO_ESCUCHA: %d\n", storage_conf->puerto_escucha);
    printf("PUNTO_MONTAJE: %s\n", storage_conf->punto_montaje);
    printf("LOG_LEVEL: %s\n", storage_conf->log_level);
    

    char puerto_escucha[10];
    sprintf(puerto_escucha, "%d", storage_conf->puerto_escucha);
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
            case HANDSHAKE_WORKER_STORAGE:
                pthread_create(&thread, NULL, (void*) atender_worker(paquete->datos), fd_conexion_ptr);
                pthread_detach(thread);
                break;
            case default:
                fprintf(stderr, "Error: el paquete no es de tipo HANDSHAKE_QC_MASTER\n");
                destruir_paquete(paquete);
                return EXIT_FAILURE;
        }
    }
    return 0;
}


void atender_worker(t_buffer* datos) {
    t_handshake_worker_master* handshake = deserializar_handshake_worker_master(datos);
    printf("ID WORKER: %s\n", handshake->id_worker);

    enviar_string(conexion_query_control, "Storage dice: Handshake recibido");

    destruir_paquete(paquete);
    free(handshake->id_worker);
    free(handshake);
}