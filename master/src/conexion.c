#include "conexion.h"
#include "m_funciones.h"

void* iniciar_receptor(void* socket_servidor) {
    int socket_servidor_ptr = *((int*) socket_servidor); //Casteo a int* y desreferencio
    log_debug(logger_master, "Socket casteado y desreferenciado: %d, procedo a escuchar conexiones", socket_servidor_ptr);
    //Recepcion de conexiones
    while (1) {
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int)); //Esto hay que liberarlo en el hilo que atiende la conexion
        *fd_conexion_ptr = esperar_cliente(socket_servidor_ptr);
        if(*fd_conexion_ptr == -1){
            fprintf(stderr, "Error al esperar cliente\n");
            return NULL;
        }
        t_paquete* paquete = recibir_paquete(*fd_conexion_ptr);
        if(paquete == NULL){
            fprintf(stderr, "Error al recibir paquete\n");
            return NULL;
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
                return NULL;
        }
    }
return NULL;
}

void* atender_query_control(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_query_control_ptr = thread_args_ptr->fd_conexion;


    t_handshake_qc_master* handshake = deserializar_handshake_qc_master(paquete_ptr->datos);
    log_info(logger_master, "Handshake recibido de Query Control con archivo de configuracion: %s y prioridad: %d", 
        handshake->archivo_query, handshake->prioridad);
    log_info(logger_master, "Achivo de configuracion: %s", handshake->archivo_query);
    log_info(logger_master, "Prioridad: %d", handshake->prioridad);

    enviar_string(*conexion_query_control_ptr, "Master dice: Handshake recibido");

    t_query* nuevaQuery = crearNuevaQuery(handshake->archivo_query, 
        handshake->prioridad, *conexion_query_control_ptr);
    queryAReady(nuevaQuery);
    
    destruir_paquete(paquete_ptr);
    free(handshake->archivo_query);
    free(handshake);

    while(1) {
        // Mantener la conexión abierta para detectar desconexiones
        t_paquete* paquete = recibir_paquete(*conexion_query_control_ptr);
        if(paquete == NULL) {
            log_info(logger_master, "## Se desconecta un Query Control. Se finaliza la Query %d con prioridad %d. Nivel multiprogramación %d"
                , nuevaQuery->id_query, nuevaQuery->prioridad, nivelMultiprogramacion);
            //TODO: Logica de desconexion. Mandar a exit, etc...
            destruir_paquete(paquete);
            break;
        }
    }
    return NULL;
}

void* atender_worker(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_query_control_ptr = thread_args_ptr->fd_conexion;

    t_handshake_worker_master* handshake = deserializar_handshake_worker_master(paquete_ptr->datos);
    log_info(logger_master, "Worker conectado con ID: %d", handshake->id_worker);

    enviar_string(*conexion_query_control_ptr, "Master dice: Handshake recibido");

    destruir_paquete(paquete_ptr);
    free(handshake);
    return NULL;
}