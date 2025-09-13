#include "conexion.h"

void* atender_query_control(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_query_control_ptr = thread_args_ptr->fd_conexion;


    t_handshake_qc_master* handshake = deserializar_handshake_qc_master(paquete_ptr->datos);
    printf("Archivo de configuracion: %s\n", handshake->archivo_configuracion);
    printf("Prioridad: %d\n", handshake->prioridad);

    enviar_string(*conexion_query_control_ptr, "Master dice: Handshake recibido");

    destruir_paquete(paquete_ptr);
    free(handshake->archivo_configuracion);
    free(handshake);
    return NULL;
}

void* atender_worker(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_query_control_ptr = thread_args_ptr->fd_conexion;

    t_handshake_worker_master* handshake = deserializar_handshake_worker_master(paquete_ptr->datos);
    printf("ID WORKER: %d\n", handshake->id_worker);

    enviar_string(*conexion_query_control_ptr, "Master dice: Handshake recibido");

    destruir_paquete(paquete_ptr);
    free(handshake);
    return NULL;
}