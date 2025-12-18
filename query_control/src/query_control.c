#include "query_control.h"

t_log* logger_qc = NULL;
query_control_conf* qc_configs = NULL;

int main(int argc, char* argv[]) {

    if(argc < 4){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion> <nombre_archivo_query> <prioridad>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* nombre_archivo_configuracion = argv[1];
    char* nombre_archivo_query = argv[2];
    int prioridad = atoi(argv[3]);

    if(inicializar_qc(nombre_archivo_configuracion, nombre_archivo_query, prioridad) == EXIT_FAILURE){
        log_error(logger_qc, "Error al inicializar el Query Control");
        log_warning(logger_qc, "Abortando Query Control");
        return EXIT_FAILURE;
    }

    // Conectar Master
    int conexion_master = conexion_qc(nombre_archivo_query, prioridad);
    if(conexion_master == EXIT_FAILURE){
        log_error(logger_qc, "Error al conectar con el Master");
        log_warning(logger_qc, "Abortando Query Control");
        return EXIT_FAILURE;
    }
    
    while(1) {
        // Mantener la conexión abierta para detectar desconexiones y recibir mensajes
        t_paquete* paquete = recibir_paquete(conexion_master);
        if(paquete == NULL) {
            log_info(logger_qc, "El Master se ha desconectado.");
            break;
        }
    }
    //limpiarMemoria(handshake);

    return 0;
}

int inicializar_qc(char* nombre_config, char* archivo_query, int prioridad) {
    qc_configs = get_configs_query_control(nombre_config);
    printf("IP_MASTER: %s\n", qc_configs->ip_master);
    printf("PUERTO_MASTER: %d\n", qc_configs->puerto_master);
    printf("LOG_LEVEL: %s\n", qc_configs->log_level);

    if (qc_configs == NULL) {
        fprintf(stderr, "Error al cargar la configuracion del Query Control.\n");
        return EXIT_FAILURE;
    }

    logger_qc = iniciarLoggerQC(archivo_query, qc_configs->log_level);
    log_debug(logger_qc, "Iniciado Query Control para la query: %s con prioridad %d", archivo_query, prioridad);

    return 0;
}

int conexion_qc(char* nombre_qc, int prioridad) {
    char puerto_master[10];
    sprintf(puerto_master, "%d", qc_configs->puerto_master);

    int conexion_master = crear_conexion(qc_configs->ip_master, puerto_master);
    if(conexion_master == -1){
        log_error(logger_qc, "Error al conectar con el master");
        log_warning(logger_qc, "Abortando Query Control");
        return EXIT_FAILURE;
    }

    t_handshake_qc_master* handshake = generarHandshake(nombre_qc, prioridad);

    t_paquete* paquete = generarPaquete(handshake);

    if(!enviar_paquete(conexion_master, paquete)) {
        log_error(logger_qc, "Error al enviar el handshake al Master");
        log_warning(logger_qc, "Abortando Query Control");
        limpiarMemoria(handshake);
        return EXIT_FAILURE;
    }

    if(!confirmarRecepcion(conexion_master)) {
        log_error(logger_qc, "Error al confirmar el handshake con el Master");
        log_warning(logger_qc, "Abortando Query Control");
        limpiarMemoria(handshake);
        return EXIT_FAILURE;
    }

    return conexion_master;
}