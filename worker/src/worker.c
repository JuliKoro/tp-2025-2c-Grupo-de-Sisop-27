#include "worker.h"
#include <sys/types.h>
#include <sys/socket.h>

// DEFINICIÓN DE VARIABLES GLOBALES
// Nota porque no entendía: Solo tienen que definirse en UN archivo .c (el main es el mejor lugar)
// SOCKETS
int conexion_storage;
int conexion_master;

// SEMAFOROS MUTEX
pthread_mutex_t mutex_registros;  // Protege: pc_actual, path_query, flags
pthread_mutex_t mutex_memoria;    // Protege: acceso a memoria_interna

// SEMAFOROS
sem_t sem_query_asignada;    // Master señaliza que hay query nueva
sem_t sem_query_terminada;   // Interpreter señaliza que terminó

t_log* logger_worker = NULL; // LOGGER GLOBAL

uint32_t id_worker; // ID del Worker

// REGISTROS Y FLAGS (declarados en registros.h)
uint32_t pc_actual = 0;
char* path_query = NULL;
uint32_t id_query = 0;
volatile bool flag_query_activa = false;
volatile bool flag_desalojo_query = false;

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

    inicializar_worker(nombre_config, argv[2]);

    // CONEXIONES
    
    if (conexiones_worker() == EXIT_FAILURE) {
        fprintf(stderr, "Error en las conexiones del worker.\n");
        return EXIT_FAILURE;
    }

    // INICIO MEMORIA INTERNA
    // -- CREACIÓN DE TABLAS DE PÁGINAS --
    // Inicializar memoria con tabla de páginas
    memoria_worker = inicializar_memoria();

    if (!memoria_worker) {
        log_error(logger_worker, "Error crítico: No se pudo inicializar la memoria");
        return EXIT_FAILURE;
    }

    // Mostrar estado inicial para verificar
    mostrar_estado_memoria();

    // SEMAFOROS
    // Inicializar semáforos
    sem_init(&sem_query_asignada, 0, 0);   // Inicia en 0 (sin queries)
    sem_init(&sem_query_terminada, 0, 1);  // Inicia en 1 (puede recibir)
    pthread_mutex_init(&mutex_registros, NULL);
    pthread_mutex_init(&mutex_memoria, NULL);

    // --- TEST MANUAL DE INTEGRACIÓN CON STORAGE ---
    // Descomentar la siguiente línea para ejecutar la prueba al iniciar:
    // test_query_interpreter_con_storage("AGING_1");

    // NUEVO TEST: INTEGRACIÓN STORAGE CONSOLA (Sin Master)
    //test_integracion_storage_consola("STORAGE_1"); // Cambia "AGING_1" por el script que quieras probar
    
    //finalizar_worker();
    //return 0;

    // HILOS
    pthread_t thread_master, thread_query_interpreter;

    // Crear hilo para recibir quries y desalojos del master
    pthread_create(&thread_master, NULL, hilo_master, NULL);
    
    // Crear hilo para ejecutar el Query Interpreter
    pthread_create(&thread_query_interpreter, NULL, hilo_query_interpreter, NULL);

    // Esperar a que los hilos terminen (en este caso, no se espera porque son hilos de ejecución continua)
    pthread_join(thread_master, NULL);
    pthread_join(thread_query_interpreter, NULL);

    finalizar_worker();

    return 0;
}

void* hilo_master(void* arg){

    t_asignacion_query* query_asignada = NULL;

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
                // Chequeo explícito de flag_query_activa antes de asignar
                if (flag_query_activa) {
                    log_warning(logger_worker, "Intento de asignar nueva Query mientras hay una activa. Esperando fin...");
                    // Esperar a que termine la actual (Master no debería hacer esto, pero por seguridad)
                }
                sem_wait(&sem_query_terminada); // Esperar a que se libere la ejecución

                // Procesar la asignación de la nueva Query (deserializacion)
                query_asignada = deserializar_asignacion_query(paquete_recibido->datos);

                // Proteger acceso a registros
                pthread_mutex_lock(&mutex_registros);
                // Actualizar registros globales
                id_query = query_asignada->id_query;
                path_query = strdup(query_asignada->path_query);
                pc_actual = query_asignada->pc;
                flag_query_activa = true;
                flag_desalojo_query = false;
                pthread_mutex_unlock(&mutex_registros);

                // Log Obligatorio - Recepción de Query
                log_info(logger_worker, "## Query %d: Se recibe la Query. El path de operaciones es: %s", id_query, path_query);

                // Señalizar que hay query lista
                sem_post(&sem_query_asignada);
                
                free(query_asignada->path_query);
                free(query_asignada);
                break;

            case OP_DESALOJO_QUERY: // Cuando Master ordean desalojar la Query actual al Worker
                log_debug(logger_worker, "Solicitud de flag_desalojo_query recibida.");

                pthread_mutex_lock(&mutex_registros);
                if (flag_query_activa) {
                    flag_desalojo_query = true; // Interpreter lo chequeará
                } else {
                    log_warning(logger_worker, "Se recibió desalojo pero no hay query en ejecución.");
                }
                pthread_mutex_unlock(&mutex_registros);

                break;

            case OP_FIN_QUERY: // Si el Master cancela una query ("Desconexión de Query Control")
                log_debug(logger_worker, "Solicitud de FIN_QUERY (cancelación) recibida.");
                pthread_mutex_lock(&mutex_registros);
                if (flag_query_activa) {
                    flag_desalojo_query = true; // Interpreter lo chequeará
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
        if (resultado == EXEC_FIN_QUERY) {
            log_debug(logger_worker, "Query %d finalizó correctamente.", id_query);
        } else if (resultado == EXEC_DESALOJO) {
            log_warning(logger_worker, "Query %d fue desalojada en PC: %d.", id_query, pc_actual);
        } 
        
        t_resultado_ejecucion resultado_flush = flush_all(); // Asegurar que todo esté persistido al finalizar o desalojar

        if (resultado >= 0 && resultado_flush < 0) {
            resultado = resultado_flush;
        }

        if (!notificar_resultado_a_master(resultado)) {
            log_error(logger_worker, "Error al notificar resultado de la Query %d al Master", id_query);
        }

        pthread_mutex_lock(&mutex_registros);
        desalojar_query(); // Limpiar estado de la query
        pthread_mutex_unlock(&mutex_registros);
        
        // Señalizar que terminó y puede recibir otra query
        sem_post(&sem_query_terminada);
        
        log_debug(logger_worker, "Query Interpreter: Listo para recibir nueva query");
    }
    
    return NULL;
}

void inicializar_worker(char* nombre_config, char* id_worker_str){
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

void desalojar_query(){
    // Limpiar estado
    //resultado = EXEC_OK;
    pc_actual = 0;
    id_query = 0;
    flag_query_activa = false;
    flag_desalojo_query = false;
    if (path_query != NULL) {
        free(path_query);
        path_query = NULL;
    }
}

void finalizar_worker(){
    log_debug(logger_worker, "Finalizando Worker ID: %d", id_worker);

    // DESTRUIR SEMAFOROS
    sem_destroy(&sem_query_asignada);
    sem_destroy(&sem_query_terminada);
    pthread_mutex_destroy(&mutex_registros);
    pthread_mutex_destroy(&mutex_memoria);

    // CERRAR SOCKETS
    close(conexion_storage);
    close(conexion_master);
    log_warning(logger_worker, "Conexiones cerradas.");

    // Limpieza de memoria al salir
    destruir_memoria();

    // Cierre de logger
    log_warning(logger_worker, "Logger cerrado.");
    if (logger_worker != NULL) {
        log_destroy(logger_worker);
    }

    // Liberar configuraciones
    if (worker_configs != NULL) {
        destruir_configs_worker(worker_configs);
    }
}

void test_query_interpreter_con_storage(char* test_path) {
    log_info(logger_worker, "==== INICIO TEST MANUAL: QUERY INTERPRETER ====");

    pthread_mutex_lock(&mutex_registros);
    
    // 1. Configuración manual de registros para simular una query asignada
    id_query = 999; // ID de prueba
    pc_actual = 0;
    flag_query_activa = true;
    flag_desalojo_query = false;

    // 2. Hardcodeo del path de la query
    // Este path se concatenará con worker_configs->path_queries en el interpreter.
    // Asegúrate de que el archivo exista.
    if (path_query) free(path_query);
    path_query = strdup(test_path); 

    pthread_mutex_unlock(&mutex_registros);

    log_info(logger_worker, "Ejecutando Query de prueba: %s (ID: %d)", path_query, id_query);

    // 3. Ejecutar lógica del interpreter directamente
    t_resultado_ejecucion resultado = query_interpreter();

    // 4. Verificar resultado
    if (resultado == EXEC_FIN_QUERY) {
        log_info(logger_worker, "==== TEST FINALIZADO: ÉXITO (EXEC_FIN_QUERY) ====");
    } else {
        log_error(logger_worker, "==== TEST FINALIZADO: FALLO O DESALOJO (Estado: %d) ====", resultado);
    }

    // 5. Limpieza post-test
    pthread_mutex_lock(&mutex_registros);
    desalojar_query();
    pthread_mutex_unlock(&mutex_registros);
}

void test_integracion_storage_consola(char* nombre_archivo) {
    log_info(logger_worker, "=== INICIO TEST INTEGRACION STORAGE (CONSOLA) ===");

    // 1. Conexión manual a Storage
    char puerto_storage[10];
    sprintf(puerto_storage, "%d", worker_configs->puerto_storage);
    
    log_info(logger_worker, "Conectando a Storage en %s:%s...", worker_configs->ip_storage, puerto_storage);
    conexion_storage = crear_conexion(worker_configs->ip_storage, puerto_storage);
    
    if (conexion_storage == -1) {
        log_error(logger_worker, "Fallo conexión a Storage. Abortando test.");
        return;
    }

    // Handshake Storage
    t_tam_pagina* tam = handshake_worker_storage(conexion_storage, id_worker);
    if (!tam) {
        log_error(logger_worker, "Fallo handshake Storage. Abortando test.");
        close(conexion_storage);
        return;
    }
    worker_configs->tam_pagina = tam->tam_pagina;
    free(tam);
    log_info(logger_worker, "Storage conectado. Tamaño página: %d", worker_configs->tam_pagina);

    // 2. Simular conexión a Master (para que no fallen los envíos en READ/FIN)
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0) {
        conexion_master = sv[0];
        // El otro extremo (sv[1]) actuará como el Master recibiendo datos silenciosamente
        log_info(logger_worker, "Conexión Master simulada (socketpair) para evitar errores de envío.");
    } else {
        log_warning(logger_worker, "No se pudo simular conexión Master. READ/FIN podrían fallar.");
        sv[1] = -1;
    }

    // 3. Inicializar Memoria
    memoria_worker = inicializar_memoria();
    if (!memoria_worker) {
        close(conexion_storage);
        if (conexion_master != -1) close(conexion_master);
        if (sv[1] != -1) close(sv[1]);
        return;
    }
    mostrar_estado_memoria();

    // 4. Configurar contexto de ejecución
    pthread_mutex_lock(&mutex_registros);
    id_query = 100; // ID Test
    pc_actual = 0;
    flag_query_activa = true;
    flag_desalojo_query = false;
    if (path_query) free(path_query);
    path_query = strdup(nombre_archivo);
    pthread_mutex_unlock(&mutex_registros);

    // 5. Bucle de ejecución con prints por pantalla
    char path_completo[512];
    snprintf(path_completo, sizeof(path_completo), "%s/%s", worker_configs->path_queries, path_query);
    
    printf("\n>>> EJECUTANDO SCRIPT: %s <<<\n", path_completo);
    
    FILE* archivo = abrir_archivo_query(path_completo);
    if (!archivo) {
        log_error(logger_worker, "No se pudo abrir el archivo: %s", path_completo);
    } else {
        while (true) {
            char* linea = fetch_instruction(archivo);
            if (!linea) break;

            t_instruccion* inst = parse_instruction(linea);
            free(linea);
            
            if (!inst) break;

            // Imprimir instrucción
            printf("[PC %d] %-30s -> ", pc_actual, inst->instruccion_raw);
            fflush(stdout);

            // Ejecutar
            t_resultado_ejecucion res = execute_instruction(inst);
            
            // Imprimir resultado
            char* msg_res = obtener_mensaje_resultado(res);
            printf("RETORNO: %d (%s)\n", res, msg_res);
            free(msg_res);
            
            destruir_instruccion(inst);

            if (res != EXEC_OK && res != EXEC_FIN_QUERY) {
                printf(">>> ERROR DETECTADO. FIN DEL TEST.\n");
                break;
            }
            if (res == EXEC_FIN_QUERY) {
                printf(">>> FIN QUERY.\n");
                break;
            }
            pc_actual++;
        }
        fclose(archivo);
    }

    // 6. Limpieza
    printf("\n=== FIN TEST ===\n");
    destruir_memoria();
    close(conexion_storage);
    if (conexion_master != -1) close(conexion_master);
    if (sv[1] != -1) close(sv[1]);
}