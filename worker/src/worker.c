#include "worker.h"
#include "memoria_interna.h" 
#include "registros.h" 

int conexion_storage;
int conexion_master;

// SEMAFOROS MUTEX
pthread_mutex_t mutex_registros;  // Protege: pc_actual, path_query, flags
pthread_mutex_t mutex_memoria;    // Protege: acceso a memoria_interna

// SEMAFOROS
sem_t sem_query_asignada;    // Master señaliza que hay query nueva
sem_t sem_query_terminada;   // Interpreter señaliza que terminó

worker_conf* worker_configs;

t_log* logger_worker = NULL;

uint32_t id_worker; //La hice global para que se pueda usar en el hilo master

// REGISTROS Y FLAGS (declarados en registros.h)
// DEFINICIÓN DE VARIABLES GLOBALES 
// Se crea el espacio en memoria
// Nota porque no entendía: Solo tienen que definirse en UN archivo .c (el main es el mejor lugar)
uint32_t pc_actual = 0;
char* path_query = NULL;
uint32_t id_query = 0;
volatile bool query_en_ejecucion = false;
volatile bool desalojar_query = false;

// Nota: memoria_worker ya no se declara acá porque está en memoria_interna.c y se accede via el header memoria_interna.h

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

    // INICIO MEMORIA INTERNA
    // -- CREACIÓN DE TABLAS DE PÁGINAS --
    // Inicializar memoria con tabla de páginas
    memoria_worker = inicializar_memoria(4096, 128); // Valores hardcodeados por ahora
    
    // Seteamos el algoritmo en la tabla (ahora que tenemos la memoria inicializada y el config cargado)
    if (memoria_worker && memoria_worker->tabla) {
        if (strcmp(worker_configs->algoritmo_reemplazo, "LRU") == 0) {
            memoria_worker->tabla->algoritmo_reemplazo = ALGORITMO_LRU;
        } else if (strcmp(worker_configs->algoritmo_reemplazo, "CLOCK-M") == 0) {
            memoria_worker->tabla->algoritmo_reemplazo = ALGORITMO_CLOCK_M;
        }
    }

    if (!memoria_worker) {
        fprintf(stderr, "Error crítico: No se pudo inicializar la memoria\n");
        return EXIT_FAILURE;
    }
    
    // Mostrar estado inicial para verificar
    mostrar_estado_memoria(memoria_worker);

    // SEMAFOROS
    // Inicializar semáforos
    sem_init(&sem_query_asignada, 0, 0);   // Inicia en 0 (sin queries)
    sem_init(&sem_query_terminada, 0, 1);  // Inicia en 1 (puede recibir)
    pthread_mutex_init(&mutex_registros, NULL);
    pthread_mutex_init(&mutex_memoria, NULL);


    // HILOS
    pthread_t thread_master, thread_query_interpreter;

    // Crear hilo para recibir quries y desalojos del master
    pthread_create(&thread_master, NULL, hilo_master, NULL);
    
    // Crear hilo para ejecutar el Query Interpreter
    pthread_create(&thread_query_interpreter, NULL, hilo_query_interpreter, NULL);

    // Esperar a que los hilos terminen (en este caso, no se espera porque son hilos de ejecución continua)
    pthread_join(thread_master, NULL);
    pthread_join(thread_query_interpreter, NULL);

    // DESTRUIR SEMAFOROS
    sem_destroy(&sem_query_asignada);
    sem_destroy(&sem_query_terminada);
    pthread_mutex_destroy(&mutex_registros);
    pthread_mutex_destroy(&mutex_memoria);

    // CERRAR SOCKETS
    close(conexion_storage);
    close(conexion_master);

    // Limpieza de memoria al salir
    destruir_memoria(memoria_worker);

    return 0;
}

void* hilo_master(void* arg){

    while (1)
    {
        // RECIBO PAQUETE DEL MASTER
        t_paquete* paquete_recibido = recibir_paquete(conexion_master);
        if (paquete_recibido == NULL) {
            log_error(logger_worker, "Error al recibir paquete del Master.");
            break;
        }

        switch (paquete_recibido->codigo_operacion)
        {
            case OP_ASIGNAR_QUERY: // Cuando Master asigna una Query al Worker
                log_debug(logger_worker, "Solicitud de ASIGNAR_QUERY recibida.");
                // Chequeo explícito de query_en_ejecucion antes de asignar
                if (query_en_ejecucion) {
                    log_warning(logger_worker, "Intento de asignar nueva Query mientras hay una activa. Esperando fin...");
                    // Esperar a que termine la actual (Master no debería hacer esto, pero por seguridad)
                }
                sem_wait(&sem_query_terminada); // Esperar a que se libere la ejecución

                // Procesar la asignación de la nueva Query (deserializacion)
                t_asignacion_query* query_asignada = deserializar_asignacion_query(paquete_recibido->datos);

                // Proteger acceso a registros
                pthread_mutex_lock(&mutex_registros);
                // Actualizar registros globales
                id_query = query_asignada->id_query;
                path_query = strdup(query_asignada->path_query);
                pc_actual = query_asignada->pc;
                query_en_ejecucion = true;
                desalojar_query = false;
                pthread_mutex_unlock(&mutex_registros);

                log_info(logger_worker, "## Query %d: Se recibe la Query. El path de operaciones es: %s", id_query, path_query);

                // Señalizar que hay query lista
                sem_post(&sem_query_asignada);
                
                free(query_asignada->path_query);
                free(query_asignada);
                break;

            case OP_DESALOJAR_QUERY: // Cuando Master ordean desalojar la Query actual al Worker
                log_debug(logger_worker, "Solicitud de DESALOJAR_QUERY recibida.");

                pthread_mutex_lock(&mutex_registros);
                if (query_en_ejecucion) {
                    desalojar_query = true; // Interpreter lo chequeará
                } else {
                    log_warning(logger_worker, "Se recibió desalojo pero no hay query en ejecución.");
                }
                pthread_mutex_unlock(&mutex_registros);

                free(query_asignada);

                break;

            case OP_FIN_QUERY: // Si el Master cancela una query ("Desconexión de Query Control")
                log_debug(logger_worker, "Solicitud de FIN_QUERY (cancelación) recibida.");
                pthread_mutex_lock(&mutex_registros);
                if (query_en_ejecucion) {
                    desalojar_query = true; // Interpreter lo chequeará
                    // mismo flag para forzar la detención
                    // setear otro flag "fue_cancelada" para
                    // diferenciar entre desalojo (pausa) y cancelación (fin).
                } else {
                    log_warning(logger_worker, "Se recibió cancelacion pero no hay query en ejecución.");
                }
                pthread_mutex_unlock(&mutex_registros);
                break; 
        
            default:
                log_warning(logger_worker, "Opcode desconocido recibido del Master: %d", paquete_recibido->codigo_operacion);
                break;
        }

        destruir_paquete(paquete_recibido);
    }
    return NULL;
}

void* hilo_query_interpreter(void* arg){

    while (1)
    {
        // Esperar que haya una query asignada
        sem_wait(&sem_query_asignada);
        
        log_debug(logger_worker, "Query Interpreter: Iniciando ejecución de query");
        
        
        // Ejecutar el query interpreter (ciclo de instrucciones)
        t_resultado_ejecucion resultado = query_interpreter();
        
        // Procesar el resultado de la ejecución
        
        switch (resultado) {
            case EXEC_OK:
                log_info(logger_worker, "Query %d finalizada exitosamente", id_query);
                // TODO: Notificar al Master que la query terminó correctamente
                break;
                
            case EXEC_DESALOJO:
                log_info(logger_worker, "Query %d desalojada. PC actual: %d", id_query, pc_actual);
                // TODO: Enviar contexto (PC) al Master para reanudación posterior
                // TODO: Hacer FLUSH de todas las páginas modificadas
                break;
                
            case EXEC_ERROR:
                log_error(logger_worker, "Query %d terminó con error", id_query);
                // TODO: Notificar al Master del error
                break;
                
            case EXEC_FIN_QUERY:
                log_info(logger_worker, "Query %d ejecutó instrucción END", id_query);
                // TODO: Notificar al Master que la query terminó
                break;
        }
        
        pthread_mutex_lock(&mutex_registros);
        // Limpiar estado
        query_en_ejecucion = false;
        desalojar_query = false;
        if (path_query != NULL) {
            free(path_query);
            path_query = NULL;
        }
        pthread_mutex_unlock(&mutex_registros);
        
        // Señalizar que terminó y puede recibir otra query
        sem_post(&sem_query_terminada);
        
        log_debug(logger_worker, "Query Interpreter: Listo para recibir nueva query");
    }
    
    return NULL;
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
        log_error(logger_worker, "Error al conectar con el modulo storage.");
        return EXIT_FAILURE;
    }

    // Handshake con Storage para recibir el tamaño de página
    t_tam_pagina* tam_pagina = handshake_worker_storage(conexion_storage, id_worker);
    if(tam_pagina == NULL){
        log_error(logger_worker, "Error en el handshake con Storage.");
        return EXIT_FAILURE;
    }
    worker_configs->tam_pagina = tam_pagina->tam_pagina;
    log_debug(logger_worker, "Handshake con Storage exitoso. Tamaño de página recibido: %d bytes", tam_pagina->tam_pagina);
    free(tam_pagina);

    // Conectar con Master.
    conexion_master = crear_conexion(worker_configs->ip_master, puerto_master);
    if(conexion_master == -1){
        fprintf(stderr, "Error al conectar con el modulo storage.\n");
        return EXIT_FAILURE;
    }
    // Handshake con Master
    t_handshake_worker_master* handshakeMaster = generarHandshakeMaster(id_worker);

    t_paquete* paquete = generarPaqueteMaster(HANDSHAKE_WORKER_MASTER, handshakeMaster);
    
    enviar_paquete(conexion_master, paquete);

    confirmarRecepcion(conexion_master);
    
    limpiarMemoriaMaster(handshakeMaster);

    return 0;
}