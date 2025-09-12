#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

int main(int argc, char* argv[]) {
    saludar("query_control");

    if(argc < 3){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion> <nombre_archivo_query> <prioridad>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int prioridad = atoi(argv[3]);
    char* nombre_archivo_query = argv[2];
    char* nombre_archivo_configuracion = argv[1];

    query_control_conf* query_control_conf = get_configs_query_control(nombre_archivo_configuracion);
    printf("IP_MASTER: %s\n", query_control_conf->ip_master);
    printf("PUERTO_MASTER: %d\n", query_control_conf->puerto_master);
    printf("LOG_LEVEL: %s\n", query_control_conf->log_level);

    char puerto_master[10];
    sprintf(puerto_master, "%d", query_control_conf->puerto_master);
    
    int conexion_master = crear_conexion(query_control_conf->ip_master, puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el master\n");
        return EXIT_FAILURE;
    }
    t_handshake_qc_master* handshake = malloc(sizeof(*handshake));
    handshake->archivo_configuracion = strdup(nombre_archivo_configuracion);
    handshake->prioridad = prioridad;

    t_paquete* paquete = malloc(sizeof(*paquete));
    paquete->codigo_operacion = HANDSHAKE_QC_MASTER;
    paquete->datos = serializar_handshake_qc_master(handshake);
    enviar_paquete(conexion_master, paquete);

    //espero confirmacion de recepecion
    char* confirmacion = recibir_string(conexion_master);
    if(confirmacion == NULL){
        fprintf(stderr, "Error al recibir confirmacion de recepecion\n");
        return EXIT_FAILURE;
    }
    printf("Confirmacion de recepecion: %s\n", confirmacion);
    free(confirmacion);

    free(handshake->archivo_configuracion);
    free(handshake);
    

    return 0;
} 