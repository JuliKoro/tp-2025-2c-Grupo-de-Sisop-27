#include "m_funciones.h"

char const* estadosQuery[] = {"Q_READY", "Q_EXEC", "Q_EXIT"};

t_list* listaQueriesReady = NULL;
t_list* listaQueriesExec = NULL;
t_list* listaQueriesExit = NULL;

pthread_mutex_t mutexListaQueriesReady;
pthread_mutex_t mutexListaQueriesExec;
pthread_mutex_t mutexListaQueriesExit;
pthread_mutex_t mutexIdentificadorQueryGlobal;
pthread_mutex_t mutexNivelMultiprogramacion;


void inicializarListasYSemaforos() {
    listaQueriesReady = list_create();
    listaQueriesExec = list_create();
    listaQueriesExit = list_create();

    pthread_mutex_init(&mutexListaQueriesReady, NULL);
    pthread_mutex_init(&mutexListaQueriesExec, NULL);
    pthread_mutex_init(&mutexListaQueriesExit, NULL);
    pthread_mutex_init(&mutexIdentificadorQueryGlobal, NULL);
    pthread_mutex_init(&mutexNivelMultiprogramacion, NULL);

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
