#include "qc_funciones.h"
#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

t_handshake_qc_master* generarHandshake() {
    t_handshake_qc_master* handshake = malloc(sizeof(*handshake));
    handshake->archivo_configuracion = strdup(nombre_archivo_configuracion);
    handshake->prioridad = prioridad;
    return handshake;
}

t_paquete* generarPaquete (t_handshake_qc_master* handshake) {
    t_paquete* paquete = malloc(sizeof(*paquete));
    paquete->codigo_operacion = HANDSHAKE_QC_MASTER;
    paquete->datos = serializar_handshake_qc_master(handshake);
    return paquete;
}

char* confirmarRecepcion (int conexion_master) {
    char* confirmacion = recibir_string(conexion_master);
    if(confirmacion == NULL){
        fprintf(stderr, "Error al recibir confirmacion de recepecion\n");
        return EXIT_FAILURE;
    }
    printf("Confirmacion de recepecion: %s\n", confirmacion);
    return confirmacion;
}

void limpiarMemoria(char* confirmacion, t_handshake_qc_master* handshake) {
    free(confirmacion);
    free(handshake->archivo_configuracion);
    free(handshake);
}