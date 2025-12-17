#include <utils/hello.h>
#include <utils/mensajeria.h>
#include "conexion.h"
#include "m_funciones.h"

t_log* logger_master =NULL;
master_conf* master_config = NULL;
int identificadorQueryGlobal = 0;
int nivelMultiprogramacion = 0;

int main(int argc, char* argv[]) {
    //Arrancamos configs y logger
    // INICIO MASTER
    if(argc != 2){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* nombre_config = argv[1];

    master_config = get_configs_master(nombre_config);    
    logger_master = iniciarLoggerMaster(master_config->log_level);
    
    //Pasamos el puerto de int a char para levantar el server
    char puerto_escucha[10];
    sprintf(puerto_escucha, "%d", master_config->puerto_escucha);
    int socket_servidor = iniciar_servidor(puerto_escucha);
    if(socket_servidor == -1){
        log_error(logger_master, "Error al iniciar el servidor en el puerto %s", puerto_escucha);
        return EXIT_FAILURE;
    }
    //iniciamos listas y semaforos
    inicializarListasYSemaforos();

    //Iniciamos la logica de recepcion en un hilo aparte, para poder probar cosas en main
    pthread_t threadReceptor;
    pthread_create(&threadReceptor, NULL, iniciar_receptor, &socket_servidor);
    pthread_detach(threadReceptor);

    //Hilo Planificador (Corto Plazo)
    pthread_t threadPlanificador;
    pthread_create(&threadPlanificador, NULL, iniciar_planificador, NULL);
    // No hacemos detach si queremos esperar al final, pero como es un servicio continuo:
    pthread_detach(threadPlanificador);

    log_info(logger_master, "Master iniciado correctamente. Esperando conexiones...");

    //Tests de manejo de listas
    /*log_debug(logger_master, "Duermo 10 segundos para que se conecte un QC y mande una query");
    sleep(10); 
    t_query* query1 = list_get(listaQueriesReady, 0);
    if(query1 != NULL) {
        actualizarEstadoQuery(query1, Q_EXEC);
        sleep(5);
        actualizarEstadoQuery(query1, Q_EXIT);
    }*/

    while(1) {
        // El main puede seguir ejecutando otras cosas aquí
        sleep(10000); // Simula que el main está haciendo otras tareas
    }

    
    return 0;
}