#include "conexion.h"

void atender_query_control(t_buffer* datos) {
    t_handshake_qc_master* handshake = deserializar_handshake_qc_master(datos);
    printf("Archivo de configuracion: %s\n", handshake->archivo_configuracion);
    printf("Prioridad: %d\n", handshake->prioridad);

    enviar_string(conexion_query_control, "Master dice: Handshake recibido");

    destruir_paquete(paquete);
    free(handshake->archivo_configuracion);
    free(handshake);
}

void atender_worker(t_buffer* datos) {
    t_handshake_worker_master* handshake = deserializar_handshake_worker_master(datos);
    printf("ID WORKER: %s\n", handshake->id_worker);

    enviar_string(conexion_query_control, "Master dice: Handshake recibido");

    destruir_paquete(paquete);
    free(handshake->id_worker);
    free(handshake);
}