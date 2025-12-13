#include "m_funciones.h"


char const* estadosQuery[] = {"Q_READY", "Q_EXEC", "Q_EXIT"};

t_list* listaQueriesReady = NULL;
t_list* listaQueriesExec = NULL;
t_list* listaQueriesExit = NULL;
// Inicialización de la lista de workers
t_list* listaWorkers = NULL;

pthread_mutex_t mutexListaQueriesReady;
pthread_mutex_t mutexListaQueriesExec;
pthread_mutex_t mutexListaQueriesExit;
pthread_mutex_t mutexIdentificadorQueryGlobal;
pthread_mutex_t mutexNivelMultiprogramacion;
// Mutex para workers
pthread_mutex_t mutexListaWorkers;


void inicializarListasYSemaforos() {
    listaQueriesReady = list_create();
    listaQueriesExec = list_create();
    listaQueriesExit = list_create();
    // Crear lista de workers
    listaWorkers = list_create();

    pthread_mutex_init(&mutexListaQueriesReady, NULL);
    pthread_mutex_init(&mutexListaQueriesExec, NULL);
    pthread_mutex_init(&mutexListaQueriesExit, NULL);
    pthread_mutex_init(&mutexIdentificadorQueryGlobal, NULL);
    pthread_mutex_init(&mutexNivelMultiprogramacion, NULL);
    // Init mutex workers
    pthread_mutex_init(&mutexListaWorkers, NULL);

    log_debug(logger_master, "Listas y semaforos de queries inicializados");
}

t_query* crearNuevaQuery(char* archivoQuery, uint8_t prioridad, int socketQuery) {
    t_query* nuevaQuery = malloc(sizeof(t_query));
    nuevaQuery->archivoQuery = strdup(archivoQuery);
    nuevaQuery->prioridad = prioridad;
    nuevaQuery->estado = Q_READY;
    nuevaQuery->socketQuery = socketQuery;

    pthread_mutex_lock(&mutexIdentificadorQueryGlobal);
    nuevaQuery->id_query = identificadorQueryGlobal++;
    pthread_mutex_unlock(&mutexIdentificadorQueryGlobal);

    log_info(logger_master, 
        "## Se conecta un Query Control para ejecutar la Query %s con prioridad %d - Id asignado: %d. Nivel multiprogramación %d", 
        nuevaQuery->archivoQuery, nuevaQuery->prioridad, nuevaQuery->id_query, nivelMultiprogramacion);

    return nuevaQuery;
}

void queryAReady(t_query* query){
    pthread_mutex_lock(&mutexListaQueriesReady);
    list_add(listaQueriesReady, query);
    pthread_mutex_unlock(&mutexListaQueriesReady);
    log_info(logger_master, "Query %d agregada a lista READY", query->id_query);
}

void queryAExec(t_query* query){
    pthread_mutex_lock(&mutexListaQueriesExec);
    list_add(listaQueriesExec, query);
    pthread_mutex_unlock(&mutexListaQueriesExec);
    log_info(logger_master, "Query %d agregada a lista EXEC", query->id_query);
}

void queryAExit(t_query* query){
    pthread_mutex_lock(&mutexListaQueriesExit);
    list_add(listaQueriesExit, query);
    pthread_mutex_unlock(&mutexListaQueriesExit);
    log_info(logger_master, "Query %d agregada a lista EXIT", query->id_query);
}

void actualizarEstadoQuery(t_query* query, e_estado_query nuevoEstado){
    //Primero nos fijamos donde esta la query, y la sacamos
    t_query* queryEncontrada = NULL;
    log_debug(logger_master, "Actualizando estado de Query %d de %s a %s", query->id_query, estadosQuery[query->estado], estadosQuery[nuevoEstado]);
    switch(query->estado){
        case Q_READY:
            pthread_mutex_lock(&mutexListaQueriesReady);
            for(int i = 0; i < list_size(listaQueriesReady); i++) {
                t_query* temp_query= list_get(listaQueriesReady, i);
                if(temp_query->id_query == query->id_query) {
                    log_debug(logger_master, "Query encontrada en lista READY, removiendo");
                    queryEncontrada = list_remove(listaQueriesReady, i);
                    break;
                }
            }
            pthread_mutex_unlock(&mutexListaQueriesReady);
            break;
        case Q_EXEC:
            pthread_mutex_lock(&mutexListaQueriesExec);
            for(int i = 0; i < list_size(listaQueriesExec); i++) {
                t_query* temp_query= list_get(listaQueriesExec, i);
                if(temp_query->id_query == query->id_query) {
                    log_debug(logger_master, "Query encontrada en lista EXEC, removiendo");
                    queryEncontrada = list_remove(listaQueriesExec, i);
                    break;
                }
            }
            pthread_mutex_unlock(&mutexListaQueriesExec);
            break;
        case Q_EXIT:
            pthread_mutex_lock(&mutexListaQueriesExit);
            for(int i = 0; i < list_size(listaQueriesExit); i++) {
                t_query* temp_query= list_get(listaQueriesExit, i);
                if(temp_query->id_query == query->id_query) {
                    log_debug(logger_master, "Query encontrada en lista EXIT, removiendo");
                    queryEncontrada = list_remove(listaQueriesExit, i);
                    break;
                }
            }
            pthread_mutex_unlock(&mutexListaQueriesExit);
            break;
        default:
            log_error(logger_master, "Error: estado de query desconocido");
            return;
    }

    if(queryEncontrada == NULL) {
        log_error(logger_master, "Error: no se encontro la query en la lista correspondiente a su estado actual");
        return;
    } else {
        //Una vez encontrada la query en la lista que correspondia, la pasamos a la lista del nuevo estado
        switch(nuevoEstado){
            case Q_READY:
                queryEncontrada->estado = Q_READY;
                queryAReady(queryEncontrada);
                break;
            case Q_EXEC:
                queryEncontrada->estado = Q_EXEC;
                queryAExec(queryEncontrada);
                break;
            case Q_EXIT:
                queryEncontrada->estado = Q_EXIT;
                queryAExit(queryEncontrada);
                break;
            default:
                log_error(logger_master, "Error: nuevo estado de query invalido");
                return;
        }

    }
}

//-------------------FIFO-------------------

t_worker_interno* obtener_worker_libre() {
    t_worker_interno* worker_encontrado = NULL;
    
    pthread_mutex_lock(&mutexListaWorkers);
    
    // Iteramos la lista buscando el primer worker con libre == true
    for (int i = 0; i < list_size(listaWorkers); i++) {
        t_worker_interno* worker = list_get(listaWorkers, i);
        if (worker->libre) {
            worker_encontrado = worker;
            break; 
        }
    }
    
    pthread_mutex_unlock(&mutexListaWorkers);
    return worker_encontrado;
}

t_query* obtener_siguiente_query_fifo() {
    t_query* query = NULL;
    
    pthread_mutex_lock(&mutexListaQueriesReady);
    if (!list_is_empty(listaQueriesReady)) {
        // En FIFO, siempre sacamos el primero de la lista (índice 0)
        // NOTA: No hacemos list_remove aquí, solo obtenemos el puntero.
        // La función actualizarEstadoQuery se encargará de moverla.
        query = list_get(listaQueriesReady, 0);
    }
    pthread_mutex_unlock(&mutexListaQueriesReady);
    
    return query;
}

void* iniciar_planificador(void* arg) {
    log_info(logger_master, "Planificador de Corto Plazo iniciado.");

    while (1) {
        // 1. Verificar si hay queries pendientes en READY
        // (Usamos mutex para lectura segura, aunque obtener_siguiente lo hace también)
        pthread_mutex_lock(&mutexListaQueriesReady);
        bool hay_queries = !list_is_empty(listaQueriesReady);
        pthread_mutex_unlock(&mutexListaQueriesReady);

        if (hay_queries) {
            // 2. Verificar si hay Workers LIBRES
            t_worker_interno* worker_elegido = obtener_worker_libre();

            if (worker_elegido != NULL) {
                // 3. Obtener la siguiente query según algoritmo (Por ahora FIFO)
                t_query* query_a_ejecutar = obtener_siguiente_query_fifo();

                if (query_a_ejecutar != NULL) {
                    log_info(logger_master, "Asignando Query %d al Worker %d", 
                             query_a_ejecutar->id_query, worker_elegido->id_worker);

                    // 4. Mover Query a estado EXEC (Esto la saca de READY y la pone en EXEC)
                    actualizarEstadoQuery(query_a_ejecutar, Q_EXEC);

                    // 5. Marcar worker como OCUPADO
                    pthread_mutex_lock(&mutexListaWorkers);
                    worker_elegido->libre = false;
                    pthread_mutex_unlock(&mutexListaWorkers);

                    // 6. Enviar la query al Worker (Serialización y Envío)
                    // Creamos la estructura que espera el worker (ver utils/estructuras.h)
                    t_asignacion_query* asignacion = malloc(sizeof(t_asignacion_query));
                    asignacion->id_query = query_a_ejecutar->id_query;
                    asignacion->path_query = strdup(query_a_ejecutar->archivoQuery);
                    asignacion->pc = 0; // Inicialmente PC es 0 (luego manejaremos desalojo)

                    t_paquete* paquete = malloc(sizeof(t_paquete));
                    paquete->codigo_operacion = OP_ASIGNAR_QUERY; // Necesitas definir este OpCode en utils
                    paquete->datos = serializar_asignacion_query(asignacion);

                    enviar_paquete(worker_elegido->socket_fd, paquete);
                    
                    // Limpieza temporal
                    free(asignacion->path_query);
                    free(asignacion);
                    // (paquete se libera dentro de enviar_paquete)
                }
            } else {
                // No hay workers libres, esperamos un poco
                // log_trace(logger_master, "No hay workers libres. Esperando...");
            }
        }

        // Evitar Busy Wait agresivo (Consumo 100% CPU)
        // Dormimos 100ms o 500ms
        usleep(500000); 
    }
    return NULL;
}
