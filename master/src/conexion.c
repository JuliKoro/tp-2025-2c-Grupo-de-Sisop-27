#include "conexion.h"
#include "m_funciones.h"


void* atender_query_control(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_query_control_ptr = thread_args_ptr->fd_conexion;


    t_handshake_qc_master* handshake = deserializar_handshake_qc_master(paquete_ptr->datos);
    log_info(logger_master, "Handshake recibido de Query Control con archivo de configuracion: %s y prioridad: %d", handshake->archivo_query, handshake->prioridad);
    log_info(logger_master, "Achivo de configuracion: %s", handshake->archivo_query);
    log_info(logger_master, "Prioridad: %d", handshake->prioridad);

    enviar_string(*conexion_query_control_ptr, "Master dice: Handshake recibido");

    destruir_paquete(paquete_ptr);
    free(handshake->archivo_query);
    free(handshake);
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