#include "worker.h"

int conexion_storage;
int conexion_master;

// SEMAFOROS MUTEX
pthread_mutex_t mutex_registros;  // Protege: pc_actual, path_query, flags
pthread_mutex_t mutex_memoria;    // Protege: acceso a memoria_interna

// SEMAFOROS
sem_t sem_query_asignada;    // Master señaliza que hay query nueva
sem_t sem_query_terminada;   // Interpreter señaliza que terminó


worker_conf* worker_configs;
t_asignacion_query* query_asignada;

t_log* logger_worker = NULL;

uint32_t id_worker; //La hice global para que se pueda usar en el hilo master

// Definición de registros y flags (declarados en registros.h)
uint32_t pc_actual = 0;
char* path_query = NULL;
bool query_en_ejecucion = false;
bool desalojar_query = false;

//--------- Ce estuvo aqui, hello ---------
// Variable global para la memoria
memoria_interna* memoria_worker = NULL;

memoria_interna* inicializar_memoria(int tam_memoria_config, int tam_pagina_hardcodeado) {
    memoria_interna* mem = malloc(sizeof(memoria_interna));
    if (!mem) {
        fprintf(stderr, "Error: No se pudo asignar memoria para memoria_interna\n");
        return NULL;
    }
    
    // Hardcodeo el tamaño de página por ahora
    mem->tam_pagina = tam_pagina_hardcodeado; // Ej: 128 bytes
    mem->tam_memoria = tam_memoria_config;
    mem->cantidad_marcos = tam_memoria_config / tam_pagina_hardcodeado;
    
    
    // Reservar la memoria principal
    mem->memoria = malloc(tam_memoria_config);
    if (!mem->memoria) {
        fprintf(stderr, "Error: No se pudo asignar memoria principal\n");
        free(mem);
        return NULL;
    }
    

    // Inicializar array de marcos libres
    mem->marcos_libres = malloc(mem->cantidad_marcos * sizeof(int));
        if (!mem->marcos_libres) {
        fprintf(stderr, "Error: No se pudo asignar memoria para marcos_libres\n");
        free(mem->memoria);
        free(mem);
    return NULL;
    }   
    // Inicializar todos los marcos como libres
    for (int i = 0; i < mem->cantidad_marcos; i++) {
        mem->marcos_libres[i] = 1; // 1 = libre, 0 = ocupado
    }

    
    // Inicializar tabla de páginas vacía
    mem->tabla = malloc(sizeof(tabla_paginas));
    if (!mem->tabla) {
        fprintf(stderr, "Error: No se pudo asignar memoria para tabla_paginas\n");
        free(mem->marcos_libres);
        free(mem->memoria);
        free(mem);
        return NULL;
    }
    
    mem->tabla->entradas = NULL;
    mem->tabla->cantidad_entradas = 0;
    mem->tabla->algoritmo_reemplazo = 0; // 0 para LRU, 1 para CLOCK-M
    
    fprintf(stderr, "Memoria interna inicializada:\n");
    fprintf(stderr, " - Tamaño memoria: %d bytes\n", mem->tam_memoria);
    fprintf(stderr, " - Tamaño página: %d bytes\n", mem->tam_pagina);
    fprintf(stderr, " - Cantidad de marcos: %d\n", mem->cantidad_marcos);
    fprintf(stderr, " - Tabla de páginas creada (vacía)\n");

    return mem;
}

void destruir_memoria(memoria_interna* mem) {
    if (!mem) return;
    
    // Liberar todas las entradas de la tabla de páginas
    if (mem->tabla && mem->tabla->entradas) {
        for (int i = 0; i < mem->tabla->cantidad_entradas; i++) {
            if (mem->tabla->entradas[i]) {
                free(mem->tabla->entradas[i]->file_name);
                free(mem->tabla->entradas[i]->tag_name);
                free(mem->tabla->entradas[i]);
            }
        }
        free(mem->tabla->entradas);
    }
    
    // Liberar estructuras principales
    if (mem->tabla) free(mem->tabla);
    if (mem->marcos_libres) free(mem->marcos_libres);
    if (mem->memoria) free(mem->memoria);
    free(mem);
    
    fprintf(stderr, "Memoria interna liberada correctamente\n");
}

void mostrar_estado_memoria(memoria_interna* mem) {
    if (!mem) {
        fprintf(stderr, "Memoria no inicializada\n");
        return;
    }
    
    fprintf(stderr, "\n=== ESTADO DE LA MEMORIA INTERNA ===\n");
    fprintf(stderr, "Tamaño total: %d bytes\n", mem->tam_memoria);
    fprintf(stderr, "Tamaño página: %d bytes\n", mem->tam_pagina);
    fprintf(stderr, "Marcos totales: %d\n", mem->cantidad_marcos);
    
    // Mostrar marcos libres
    fprintf(stderr, "Marcos libres: ");
    for (int i = 0; i < mem->cantidad_marcos; i++) {
        fprintf(stderr, "%d ", mem->marcos_libres[i]);
    }
    fprintf(stderr, "\n");
    
    // Mostrar tabla de páginas
    fprintf(stderr, "Entradas en tabla de páginas: %d\n", mem->tabla->cantidad_entradas);
    for (int i = 0; i < mem->tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = mem->tabla->entradas[i];
        fprintf(stderr, "  [%d] File: %s, Tag: %s, Página: %d, Marco: %d, Presente: %d, Modificado: %d\n",
                i, entrada->file_name, entrada->tag_name, entrada->numero_pagina,
                entrada->marco, entrada->presente, entrada->modificado);
    }
    fprintf(stderr, "====================================\n\n");
}

//-------------------------------------------------------


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
         // ========== CREACIÓN DE TABLAS DE PÁGINAS ==========
    // Inicializar memoria con tabla de páginas
    memoria_worker = inicializar_memoria(4096, 128); // Valores hardcodeados por ahora
    
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
                pc_actual = query_asignada->pc;
                path_query = strdup(query_asignada->path_query); // Guardar el path de la query
                query_en_ejecucion = true;
                desalojar_query = false;
                pthread_mutex_unlock(&mutex_registros);

                log_info(logger_worker, "## Query %d: Se recibe la Query. El path de operaciones es: %s", query_asignada->id_query, path_query);

                // Señalizar que hay query lista
                sem_post(&sem_query_asignada);

                //free(query_asignada); // Revisar si combiene utilizar este struct o los registos por separado

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
        // Falta adaptar al ciclo de instruciones:
        // Quiza hacer el loop de ejecucion aqui y preguntar por el desalojo, etc.
        // Ejecutar query línea por línea
        pthread_mutex_lock(&mutex_registros);
        //query_interpreter(); // PARSEAR Y EJECUTAR QUERY
        pthread_mutex_unlock(&mutex_registros);

        sem_post(&sem_query_terminada);  // Ahora puede recibir otra
        
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