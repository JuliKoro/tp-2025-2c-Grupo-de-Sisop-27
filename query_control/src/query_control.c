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

    // INICIALIZACION DEL QUERY CONTROL
    if(inicializar_qc(nombre_archivo_configuracion, nombre_archivo_query, prioridad) == EXIT_FAILURE){
        log_error(logger_qc, "Error al inicializar el Query Control");
        log_warning(logger_qc, "Abortando Query Control");
        return EXIT_FAILURE;
    }

    // CONEXION CON MASTER
    int conexion_master = conexion_qc(nombre_archivo_query, prioridad);
    if(conexion_master == EXIT_FAILURE){
        log_error(logger_qc, "Error al conectar con el Master");
        log_warning(logger_qc, "Abortando Query Control");
        return EXIT_FAILURE;
    }
    
    // ESCUCHA DE MENSAJES DEL MASTER
    while(1) {
        // Mantener la conexión abierta para detectar desconexiones y recibir mensajes
        t_paquete* paquete = recibir_paquete(conexion_master);
        if(paquete == NULL) {
            log_info(logger_qc, "El Master se ha desconectado.");
            break;
        }

        switch(paquete->codigo_operacion) {
            case OP_READ: // Mensaje leido/operacion READ (Worker -> Master & Master -> QC)
                t_msj_leido* info_leida = deserializar_lectura(paquete->datos);

                log_info(logger_qc, "## Lectura realizada: File %s:%s, contenido: %s",
                         info_leida->file_name, info_leida->tag_name, info_leida->lectura);
                
                free(info_leida->file_name);
                free(info_leida->tag_name);
                free(info_leida->lectura);
                break;
                
            case OP_RESULTADO_QUERY: // Resultado de la Query
                t_resultado_query* resultado = deserializar_resultado_query(paquete->datos);
                if(resultado->estado >= 0) {
                    log_info(logger_qc, "## Query %d finalizada correctamente en PC: %d", 
                             resultado->id_query, resultado->pc_final);
                } else {
                    log_warning(logger_qc, "## Query %d abortada en PC: %d. Cod. Error: %d - %s", 
                                resultado->id_query, resultado->pc_final, resultado->estado, 
                                resultado->mensaje_error);
                }
                // TODO: Liberar resultado
                if(resultado->mensaje_error) free(resultado->mensaje_error);
                break;
            default:
                log_warning(logger_qc, "Operación desconocida recibida del Master: %d", paquete->codigo_operacion);
                break;
        }
    }

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

    // Log Obligatorio - Conexión al master
    log_info(logger_qc, "## Conexión al Master exitosa. IP: %s, Puerto: %s", 
             qc_configs->ip_master, puerto_master);

    // HANDSHAKE CON MASTER (archivo_query, prioridad)
    t_handshake_qc_master* handshake = generarHandshake(nombre_qc, prioridad);

    t_paquete* paquete = generarPaquete(handshake);

    // Envio de Query
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

    // Log Obligatorio - Envío de Query
    log_info(logger_qc, "## Solicitud de ejecución de Query: %s, prioridad: %d", 
             nombre_qc, prioridad);
    
    limpiarMemoria(handshake);
    return conexion_master;
}