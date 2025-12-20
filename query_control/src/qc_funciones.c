#include "qc_funciones.h"


t_handshake_qc_master* generarHandshake(char* archivo_query, int prioridad) {
    t_handshake_qc_master* handshake = malloc(sizeof(*handshake));
    handshake->archivo_query = strdup(archivo_query);
    handshake->prioridad = prioridad;
    return handshake;
}

t_paquete* generarPaquete (t_handshake_qc_master* handshake) {
    t_paquete* paquete = malloc(sizeof(*paquete));
    paquete->codigo_operacion = HANDSHAKE_QC_MASTER;
    paquete->datos = serializar_handshake_qc_master(handshake);
    return paquete;
}

bool confirmarRecepcion (int conexion_master) {
    char* confirmacion = recibir_string(conexion_master);
    if(confirmacion == NULL){
        fprintf(stderr, "Error al recibir confirmacion de recepecion\n");
        return false;
    }
    printf("Confirmacion de recepecion: %s\n", confirmacion);
    free(confirmacion);
    return true;
}

void limpiarMemoria(t_handshake_qc_master* handshake) {
    free(handshake->archivo_query);
    free(handshake);
}

int conexion_qc(char* nombre_qc, int prioridad) {
    char puerto_master[10];
    sprintf(puerto_master, "%d", qc_configs->puerto_master);

    int conexion_master = crear_conexion(qc_configs->ip_master, puerto_master);
    if(conexion_master == -1){
        log_error(logger_qc, "Error al conectar con el master");
        log_warning(logger_qc, "Abortando Query Control");
        return -1;
    }

    // Log Obligatorio - Conexión al master
    log_info(logger_qc, "## Conexión al Master exitosa. IP: %s, Puerto: %s", 
             qc_configs->ip_master, puerto_master);

    // HANDSHAKE CON MASTER (archivo_query, prioridad)
    t_handshake_qc_master* handshake = generarHandshake(nombre_qc, prioridad);

    t_paquete* paquete = generarPaquete(handshake);

    // Envio de Query
    if(enviar_paquete(conexion_master, paquete) != 0) {
        log_error(logger_qc, "Error al enviar el handshake al Master");
        log_warning(logger_qc, "Abortando Query Control");
        limpiarMemoria(handshake);
        return -1;
    }

    if(!confirmarRecepcion(conexion_master)) {
        log_error(logger_qc, "Error al confirmar el handshake con el Master");
        log_warning(logger_qc, "Abortando Query Control");
        limpiarMemoria(handshake);
        return -1;
    }

    // Log Obligatorio - Envío de Query
    log_info(logger_qc, "## Solicitud de ejecución de Query: %s, prioridad: %d", 
             nombre_qc, prioridad);
    
    limpiarMemoria(handshake);
    return conexion_master;
}