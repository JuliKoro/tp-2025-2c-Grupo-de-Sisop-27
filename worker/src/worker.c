#include "worker.h"

int conexion_storage;
int conexion_master;

worker_conf* worker_configs;
t_asignacion_query* query_asignada;

t_log* logger_worker = NULL;

uint32_t id_worker; //La hice global para que se pueda usar en el hilo master

int main(int argc, char* argv[]) {
    fprintf(stderr, "Worker ID: %s\n", argv[2]);

    // INICIO WORKER
    // Verifco que se haya pasado el ID como argumento
    if(argc != 3){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion> <id_worker>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* nombre_config = argv[1];
    id_worker = atoi(argv[2]);

    inicializacion_worker(nombre_config, argv[2]);

    // CONEXIONES
    if (conexiones_worker() == EXIT_FAILURE) {
        fprintf(stderr, "Error en las conexiones del worker.\n");
        return EXIT_FAILURE;
    }

    // INICIO MEMORIA INTERNA
    // inicio_memoria();

    // HILOS
    pthread_t thread_master, thread_query_interpreter;

    // Crear hilo para recibir quries y desalojos del master
    pthread_create(&thread_master, NULL, hilo_master, NULL);
    
    // Crear hilo para ejecutar el Query Interpreter
    pthread_create(&thread_query_interpreter, NULL, hilo_query_interpreter, NULL);

    // SEMAFOROS

    // Esperar a que los hilos terminen (en este caso, no se espera porque son hilos de ejecución continua)
    pthread_join(thread_master, NULL);
    pthread_join(thread_query_interpreter, NULL);

    // CERRAR SOCKETS
    close(conexion_storage);
    close(conexion_master);

    return 0;
}

void* hilo_master(void* arg){

    while (1)
    {
        // ASIGNACION DE QUERY
        t_paquete* paquete_recibido = recibir_paquete(conexion_master);
        if (paquete_recibido == NULL) {
            log_error(logger_worker, "Error al recibir paquete del Master.");
            break;
        }

        switch (paquete_recibido->codigo_operacion)
        {
            case OP_ASIGNAR_QUERY:
                // Chequeo explícito de query_en_ejecucion antes de asignar
                if (query_en_ejecucion) {
                    log_warning(logger_worker, "Intento de asignar nueva Query mientras hay una activa. Esperando fin...");
                    // Esperar a que termine la actual (Master no debería hacer esto, pero por seguridad)
                    //sem_wait(&sem_query_terminada);
                    //log_info(logger_worker, "Query anterior terminada. Procediendo con nueva asignación.");
                }

                // Procesar la asignación de la nueva Query (deserializacion)
                t_asignacion_query* query_asignada = deserializar_asignacion_query(paquete_recibido->datos);
                pc_actual = query_asignada->pc;
                path_query = strdup(query_asignada->path_query); // Guardar el path de la query
                //free(query_asignada->path_query);

                // Actualizar estado (setear flags)

                break;

            case OP_DESALOJAR_QUERY:
                /* code */
                break;

            case OP_FIN_QUERY:
                /* code */
                break; 
        
            default:
                log_warning(logger_worker, "Opcode desconocido recibido del Master: %d", paquete_recibido->codigo_operacion);
                break;
        }

        destruir_paquete(paquete_recibido);
    }
}

void* hilo_query_interpreter(void* arg){

    while (1)
    {
        // PARSEAR Y EJECUTAR QUERY
    }
    
}

void inicializacion_worker(char* nombre_config, char* id_worker_str){
    // CARGA CONFIGS (Se carga la variable global 'worker_configs')
    worker_configs = get_configs_worker(nombre_config);
    if (worker_configs == NULL) {
        fprintf(stderr, "Error al cargar la configuracion del worker.\n");
        exit(EXIT_FAILURE);
    }

    // INICIO LOGGERS
    logger_worker = iniciarLoggerWorker(id_worker_str, worker_configs->log_level);
    log_debug(logger_worker, "Iniciado Worker ID: %d", id_worker);
}

int conexiones_worker(){
    //SOCKETS
    char puerto_storage[10];
    char puerto_master[10];
    sprintf(puerto_storage, "%d", worker_configs->puerto_storage);
    sprintf(puerto_master, "%d", worker_configs->puerto_master);

    // Conectar con Storage.
    conexion_storage = crear_conexion(worker_configs->ip_storage, puerto_storage);
    if(conexion_storage == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }

    // Arreglar HS con Storage -> recibe TAM_PAGINA (Agregar en configs)
    t_handshake_worker_storage* handshakeStorage = generarHandshakeStorage(id_worker);
    
    t_paquete* paquete = generarPaqueteStorage(HANDSHAKE_WORKER_STORAGE, handshakeStorage);
    
    enviar_paquete(conexion_storage, paquete);

    confirmarRecepcion(conexion_storage);
    
    limpiarMemoriaStorage(handshakeStorage);

    // Conectar con Master.
    conexion_master = crear_conexion(worker_configs->ip_master, puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }
    t_handshake_worker_master* handshakeMaster = generarHandshakeMaster(id_worker);

    paquete = generarPaqueteMaster(HANDSHAKE_WORKER_MASTER, handshakeMaster);
    
    enviar_paquete(conexion_master, paquete);

    confirmarRecepcion(conexion_master);
    
    limpiarMemoriaMaster(handshakeMaster);

    return 0;
}