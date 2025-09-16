#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include "qc_funciones.h"

t_log* logger_qc = NULL;

int main(int argc, char* argv[]) {
    saludar("query_control");

    if(argc < 4){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion> <nombre_archivo_query> <prioridad>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* nombre_archivo_configuracion = argv[1];
    char* nombre_archivo_query = argv[2];
    int prioridad = atoi(argv[3]);

    query_control_conf* query_control_conf = get_configs_query_control(nombre_archivo_configuracion);
    printf("IP_MASTER: %s\n", query_control_conf->ip_master);
    printf("PUERTO_MASTER: %d\n", query_control_conf->puerto_master);
    printf("LOG_LEVEL: %s\n", query_control_conf->log_level);

    logger_qc = iniciarLoggerQC(nombre_archivo_query, query_control_conf->log_level);



    // Conectar Master.

    char puerto_master[10];
    sprintf(puerto_master, "%d", query_control_conf->puerto_master);

    int conexion_master = crear_conexion(query_control_conf->ip_master, puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el master\n");
        return EXIT_FAILURE;
    }

    t_handshake_qc_master* handshake = generarHandshake(nombre_archivo_query, prioridad);

    t_paquete* paquete = generarPaquete(handshake);

    enviar_paquete(conexion_master, paquete);

    confirmarRecepcion(conexion_master);
    
    while(1) {
        // Mantener la conexión abierta para detectar desconexiones y recibir mensajes
        t_paquete* paquete = recibir_paquete(conexion_master);
        if(paquete == NULL) {
            log_info(logger_qc, "El Master se ha desconectado.");
            break;
        }
    }
    limpiarMemoria(handshake);

    return 0;
}

