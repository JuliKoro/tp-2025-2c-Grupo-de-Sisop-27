#include "m_funciones.h"


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
pthread_mutex_t mutexnivelMultiprocesamiento ;
// Mutex para workers
pthread_mutex_t mutexListaWorkers;
pthread_mutex_t mutexWorkerLibre;

//mechi ultimos 2 logs?

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
    pthread_mutex_init(&mutexnivelMultiprocesamiento , NULL);
    // Init mutex workers
    pthread_mutex_init(&mutexListaWorkers, NULL);
    //chequear que se aumente y disminuya al asignar y liberar worker
    pthread_mutex_init(&mutexWorkerLibre, NULL);
    log_debug(logger_master, "Listas y semaforos de queries inicializados");
}

// --- FUNCIONES DE DESTRUCCIÓN DE ELEMENTOS ---

void destruir_query(void* elemento) {
    t_query* query = (t_query*) elemento;
    if (query->archivoQuery != NULL) {
        free(query->archivoQuery);
    }
    // Si hay un hilo de aging, deberías asegurarte de que termine antes de liberar
    // pthread_cancel(query->thread_aging); // Opcional si se usan flags
    free(query);
}

void destruir_worker_interno(void* elemento) {
    t_worker_interno* worker = (t_worker_interno*) elemento;
    // No cerramos el socket_fd aaca porque eso lo maneja el hilo atender_worker
    // o se cierra al caerse la conexión.
    free(worker);
}

void finalizarMaster() {
    log_debug(logger_master, "Finalizando Master");

    //destroy listas
    //list_destroy_and_destroy_elements(listaQueriesReady, free);
    //list_destroy_and_destroy_elements(listaQueriesExec, free);
    //list_destroy_and_destroy_elements(listaQueriesExit, free);
    //list_destroy_and_destroy_elements(listaWorkers, free);
       
    // 1. Destruir listas y sus elementos
    // Usamos las funciones auxiliares para liberar la memoria interna de cada nodo
    if (listaQueriesReady) 
        list_destroy_and_destroy_elements(listaQueriesReady, destruir_query);
    if (listaQueriesExec) 
        list_destroy_and_destroy_elements(listaQueriesExec, destruir_query);
    if (listaQueriesExit) 
        list_destroy_and_destroy_elements(listaQueriesExit, destruir_query);
    if (listaWorkers) 
        list_destroy_and_destroy_elements(listaWorkers, destruir_worker_interno);

    // 2. Destruir semáforos y mutex
    //destroy semaforos y mutex
    pthread_mutex_destroy(&mutexListaQueriesReady);
    pthread_mutex_destroy(&mutexListaQueriesExec);
    pthread_mutex_destroy(&mutexListaQueriesExit);
    pthread_mutex_destroy(&mutexIdentificadorQueryGlobal);
    pthread_mutex_destroy(&mutexnivelMultiprocesamiento );
    pthread_mutex_destroy(&mutexListaWorkers);
    pthread_mutex_destroy(&mutexWorkerLibre);
    sem_destroy(&semPlanificador);

    // 3. Liberar configuraciones
    if (master_config != NULL) {
        destruir_configs_master(master_config);
    }

    // 4. Cierre de logger (Hacerlo al final para poder loguear lo anterior)
    // Cierre de logger
    log_warning(logger_master, "Logger cerrado.");
    if (logger_master != NULL) {
        log_destroy(logger_master);
    }

}

t_query* crearNuevaQuery(char* archivoQuery, uint8_t prioridad, int socketQuery) {
    t_query* nuevaQuery = malloc(sizeof(t_query));
    nuevaQuery->archivoQuery = strdup(archivoQuery);
    nuevaQuery->prioridad = prioridad;
    nuevaQuery->estado = Q_READY;
    nuevaQuery->socketQuery = socketQuery;
    nuevaQuery->pc = 0; // Inicializamos el Program Counter en 0
    nuevaQuery->desconexion_solicitada = false;

    pthread_mutex_lock(&mutexIdentificadorQueryGlobal);
    nuevaQuery->id_query = identificadorQueryGlobal++;
    pthread_mutex_unlock(&mutexIdentificadorQueryGlobal);

    return nuevaQuery;
}

int cantidadWorkersDisponibles(t_list* listaWorkers, t_list* listaQueriesReady){
    int cantidadWorkersLibres = list_size(listaWorkers);
    int cantidadQueriesReady = list_size(listaQueriesReady);
    return cantidadWorkersLibres - cantidadQueriesReady;
}



void queryAReady(t_query* query){
    pthread_mutex_lock(&mutexListaQueriesReady);
    list_add(listaQueriesReady, query);
    pthread_mutex_unlock(&mutexListaQueriesReady);
    log_info(logger_master, "Query %d agregada a lista READY", query->id_query);
    //mechi
    //pthread_mutex_lock(&mutexListaWorkers);

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
    //mechi necesito el worker id pero no lo puedo conseguir aca, y veo que no hay ninguna funcion que 
    //invoque a query exit
    log_info(logger_master, "Se terminó la Query %d agregada a lista EXIT", query->id_query);
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
    //cambiomechi
    // Iteramos la lista buscando el primer worker con libre == true
    for (int i = 0; i < list_size(listaWorkers); i++) {
        t_worker_interno* worker = list_get(listaWorkers, i);
        if (worker->query == NULL) {
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

void* comparar_prioridades(void* query1, void* query2){
    t_query* queryA = (t_query*) query1;
    t_query* queryB = (t_query*) query2;

    return queryA->prioridad <= queryB->prioridad ? queryA : queryB;
}

t_query* obtener_siguiente_query_prioridades() {
    t_query* query = NULL;
    pthread_mutex_lock(&mutexListaQueriesReady);
    // BUSCO EL MENOR NUMERO DE PRIORIDAD
    if (!list_is_empty(listaQueriesReady)) {
        query = list_get_minimum(listaQueriesReady, comparar_prioridades);
    }
    pthread_mutex_unlock(&mutexListaQueriesReady);
    return query;
}

t_query* obtener_query_menos_prioritaria_ejecutandose() {
    t_query* query = NULL;
    pthread_mutex_lock(&mutexListaQueriesExec);
    // BUSCO EL MENOR NUMERO DE PRIORIDAD
    if (!list_is_empty(listaQueriesExec)) {
        //mechi chequear aca el comparar prioridades por si hay que hacer otra func
        query = list_get_maximum(listaQueriesExec, comparar_prioridades);
    }
    pthread_mutex_unlock(&mutexListaQueriesExec);
    return query;
}


//mechi no tendriamos que ver el punto config?
// HILO PLANIFICADOR (Corto Plazo)
void* iniciar_planificador(void* arg) {
    log_info(logger_master, "Planificador de Corto Plazo iniciado.");
/*
planif se despierta cuadno:
    - hay una nueva query en ready 
    - hay un worker libre
    - un worker se libera (termina una query)
    - una query cambia de prioridad (aging) (NO SE SI ESTO ES NECESARIO)
    - un worker se desconecta
*/
    while (1) {
        sem_wait(&semPlanificador);
        // 1. Verificar si hay queries pendientes en READY
        // (Usamos mutex para lectura segura, aunque obtener_siguiente lo hace también)
        pthread_mutex_lock(&mutexListaQueriesReady);
        bool hay_queries = !list_is_empty(listaQueriesReady);
        pthread_mutex_unlock(&mutexListaQueriesReady);

        pthread_mutex_lock(&mutexListaWorkers);
        bool hay_workers = !list_is_empty(listaWorkers);
        pthread_mutex_unlock(&mutexListaWorkers);

        if (hay_queries && hay_workers) {
            // 2. Verificar si hay Workers LIBRES
            t_worker_interno* worker_elegido = obtener_worker_libre();

            if (worker_elegido != NULL) {
                t_query* query_a_ejecutar = NULL;
                // 3. Obtener la siguiente query según algoritmo de planificación
                if (strcmp( master_config->algoritmo_planificacion, "FIFO") == 0) {
                    query_a_ejecutar = obtener_siguiente_query_fifo();
                    log_debug(logger_master, "query obtenida fifo");
                } else if (strcmp( master_config->algoritmo_planificacion, "PRIORIDADES") == 0) {
                    query_a_ejecutar = obtener_siguiente_query_prioridades();
                    log_debug(logger_master, "query obtenida prioridades");
                } else {
                    log_error(logger_master, "Algoritmo de planificación desconocido: %s", master_config->algoritmo_planificacion);
                }
                
                if (query_a_ejecutar != NULL) {
                    log_info(logger_master, "Asignando Query %d al Worker %d", 
                             query_a_ejecutar->id_query, worker_elegido->id_worker);

                    // 4. Mover Query a estado EXEC (Esto la saca de READY y la pone en EXEC)
                    actualizarEstadoQuery(query_a_ejecutar, Q_EXEC);
                    
                    // 5. Marcar worker como OCUPADO
                    pthread_mutex_lock(&mutexListaWorkers);
                    worker_elegido->libre = false;
                    worker_elegido->query = query_a_ejecutar;
                    pthread_mutex_unlock(&mutexListaWorkers);
                    log_debug(logger_master, "worker ocupado");
                    
                    // 6. Enviar la query al Worker (Serialización y Envío)
                    // Creamos la estructura que espera el worker (ver utils/estructuras.h)
                    t_asignacion_query* asignacion = malloc(sizeof(t_asignacion_query));
                    asignacion->id_query = query_a_ejecutar->id_query;
                    asignacion->path_query = strdup(query_a_ejecutar->archivoQuery);
                    asignacion->pc = query_a_ejecutar->pc; // Usamos el PC guardado (0 o el último ejecutado)
                    log_debug(logger_master, "Query quitada de ready");

                    t_paquete* paquete = malloc(sizeof(t_paquete));
                    paquete->codigo_operacion = OP_ASIGNAR_QUERY; // Necesitas definir este OpCode en utils
                    paquete->datos = serializar_asignacion_query(asignacion);

                    enviar_paquete(worker_elegido->socket_fd, paquete);
                    log_info(logger_master, "Se envía la Query %d (%d) al Worker %d", 
                             query_a_ejecutar->id_query, query_a_ejecutar->prioridad, worker_elegido->id_worker);
                    
                    // Limpieza temporal
                    free(asignacion->path_query);
                    free(asignacion);
                }
            } else { // Caso: No hay workers libres
                // caso de posible desalojo
                if(strcmp( master_config->algoritmo_planificacion, "PRIORIDADES") == 0){
                    t_query* query_a_ejecutar = obtener_siguiente_query_prioridades();
                    t_query* query_menor_prioritaria = obtener_query_menos_prioritaria_ejecutandose();
                    if(query_a_ejecutar->prioridad < query_menor_prioritaria->prioridad){
                        
                        // enviar mensaje de desalojo al worker
                        t_paquete* paquete_desalojo = malloc(sizeof(t_paquete));
                        paquete_desalojo->codigo_operacion = OP_DESALOJO_QUERY; // Necesitas definir este OpCode en utils
                        paquete_desalojo->datos = NULL; // No necesitamos datos adicionales

                        // Encontrar el worker que ejecuta la query menos prioritaria
                        t_worker_interno* worker_a_desalojar = NULL;
                        pthread_mutex_lock(&mutexListaWorkers);
                        for (int i = 0; i < list_size(listaWorkers); i++) {
                            t_worker_interno* worker = list_get(listaWorkers, i);
                            if (worker->query != NULL && worker->query->id_query == query_menor_prioritaria->id_query) {
                                worker_a_desalojar = worker;
                                break; 
                            }
                        }
                        pthread_mutex_unlock(&mutexListaWorkers);

                        if(worker_a_desalojar != NULL){
                            enviar_paquete(worker_a_desalojar->socket_fd, paquete_desalojo);                            
                            }

                        log_info(logger_master, "## Se desaloja la Query %d (%d) del Worker %d - Motivo: PRIORIDAD", 
                                 query_menor_prioritaria->id_query, query_menor_prioritaria->prioridad, worker_a_desalojar->id_worker);

                        }
                }
            }
        
        // Evitar Busy Wait agresivo (Consumo 100% CPU)
        // Dormimos 100ms o 500ms
        usleep(500000); 
        }
    return NULL;
    }
}

// HILO DE AGING PARA LA QUERY
void* aging_de_query(void* query){

//mechi hacer sincro para que duerma este hilo cuando query esta en exec (consultar a Damian)
    t_query* laQuery = (t_query*) query;

    while(1) {
        usleep(master_config->tiempo_aging * 1000);
        log_info(logger_master, "La prioridad es de %d", laQuery->prioridad);
        if(laQuery->estado == Q_READY) {
            if(laQuery->prioridad > 0) {
                laQuery->prioridad--;
                sem_post(&semPlanificador); // Avisar al planificador que hubo un cambio de prioridad
                log_info(logger_master, "%d Cambio de prioridad: %d - %d", laQuery->id_query, laQuery->prioridad + 1, laQuery->prioridad);
            } else {
                break;
            }
        }
    }    
    return NULL;
}
