#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>

t_paquete* generarPaquete(void);
t_handshake_qc_master* generarHandshake(void);
void confirmarRecepcion (int);
void limpiarMemoria(char*, t_handshake_qc_master*);

int main(int argc, char* argv[]) {
    saludar("query_control");

    if(argc < 3){
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

    // Conectar Master.

    char puerto_master[10];
    sprintf(puerto_master, "%d", query_control_conf->puerto_master);

    int conexion_master = crear_conexion(query_control_conf->ip_master, puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el master\n");
        return EXIT_FAILURE;
    }

    t_paquete* paquete = generarPaquete();

    t_handshake_qc_master* handshake = generarHandshake();
    
    enviar_paquete(conexion_master, paquete);

    confirmarRecepcion(conexion_master)
    
    limpiarMemoria(confirmacion, handshake);

    return 0;
}

t_paquete* generarPaquete () {
    t_paquete* paquete = malloc(sizeof(*paquete));
    paquete->codigo_operacion = HANDSHAKE_QC_MASTER;
    paquete->datos = serializar_handshake_qc_master(handshake);
    return paquete;
}

t_handshake_qc_master* generarHandshake() {
    t_handshake_qc_master* handshake = malloc(sizeof(*handshake));
    handshake->archivo_configuracion = strdup(nombre_archivo_configuracion);
    handshake->prioridad = prioridad;
    return handshake;
}

void confirmarRecepcion (int conexion_master) {
    char* confirmacion = recibir_string(conexion_master);
    if(confirmacion == NULL){
        fprintf(stderr, "Error al recibir confirmacion de recepecion\n");
        return EXIT_FAILURE;
    }
    printf("Confirmacion de recepecion: %s\n", confirmacion);
}

void limpiarMemoria(char* confirmacion, t_handshake_qc_master* handshake) {
    free(confirmacion);
    free(handshake->archivo_configuracion);
    free(handshake);
}