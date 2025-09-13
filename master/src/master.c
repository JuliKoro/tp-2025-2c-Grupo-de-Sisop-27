#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

int main(int argc, char* argv[]) {
    saludar("master");

    master_conf* master_conf = get_configs_master("master.config");
    printf("PUERTO_ESCUCHA: %d\n", master_conf->puerto_escucha);
    printf("ALGORITMO_PLANIFICACION: %s\n", master_conf->algoritmo_planificacion);
    printf("TIEMPO_AGING: %d\n", master_conf->tiempo_aging);
    printf("LOG_LEVEL: %s\n", master_conf->log_level);
    


    //Test de handshake entre query control y master
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
        
        pthread_create(&thread, NULL, (void*) atender_cliente, fd_conexion_ptr);
        pthread_detach(thread);
    }

    
    int conexion_query_control = esperar_cliente(socket_servidor);
    if(conexion_query_control == -1){
        fprintf(stderr, "Error al esperar cliente\n");
        return EXIT_FAILURE;
    }
    t_paquete* paquete = recibir_paquete(conexion_query_control);
    if(paquete == NULL){
        fprintf(stderr, "Error al recibir paquete\n");
        return EXIT_FAILURE;
    }
    if(paquete->codigo_operacion == HANDSHAKE_QC_MASTER){
        t_handshake_qc_master* handshake = deserializar_handshake_qc_master(paquete->datos);
        printf("Archivo de configuracion: %s\n", handshake->archivo_configuracion);
        printf("Prioridad: %d\n", handshake->prioridad);

        //Envio confirmacion de recepecion
        enviar_string(conexion_query_control, "Master dice: Handshake recibido");

        destruir_paquete(paquete);
        free(handshake->archivo_configuracion);
        free(handshake);
    } else {
        fprintf(stderr, "Error: el paquete no es de tipo HANDSHAKE_QC_MASTER\n");
        destruir_paquete(paquete);
        return EXIT_FAILURE;
    }

    
    //Test de handshake entre query control y master
    
    
    
    return 0;
}
