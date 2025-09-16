#ifndef M_FUNCIONES_H
#define M_FUNCIONES_H   

#include <utils/loggeo.h>
#include <utils/configs.h>
#include <pthread.h>
#include <commons/collections/list.h>


extern t_log* logger_master;

extern int identificadorQueryGlobal; //La inicilizamos en master.c, en 0
extern int nivelMultiprogramacion; //La inicilizamos en master.c, en 0
extern char const* estadosQuery[]; //La inicilizamos en m_funciones.c. Es para traducir los enum a string, para loggear.
extern master_conf* master_config; //La inicilizamos en master.c, con la config del master

extern t_list* listaQueriesReady;
extern t_list* listaQueriesExec;
extern t_list* listaQueriesExit;

extern pthread_mutex_t mutexListaQueriesReady;
extern pthread_mutex_t mutexListaQueriesExec;
extern pthread_mutex_t mutexListaQueriesExit;
extern pthread_mutex_t mutexIdentificadorQueryGlobal; 
extern pthread_mutex_t mutexNivelMultiprogramacion;

typedef enum {
    Q_READY=0,
    Q_EXEC,
    Q_EXIT
} e_estado_query;

typedef struct{
    uint32_t id_query;
    uint8_t prioridad;
    char* archivoQuery;
    e_estado_query estado;
    int socketQuery; //socket para comunicarse con el query control que envio la query
    //Quiza haga falta un semaforo para manejar el estado de la query
} t_query;

/**
 * @brief: Inicializa las listas de queries (ready, exec, exit).
 */
void inicializarListasYSemaforos();

/**
 * @brief: Crea una estructura t_query* con los datos pasados por parametro.
 * @param archivo_query: El path al archivo de la query.
 * @param prioridad: La prioridad de la query.
 * @return: El t_log* creado.
 */
t_query* crearNuevaQuery(char* archivoQuery, uint8_t prioridad, int socketQuery);

/**
 * @brief: Agrega una query a la lista de queries en estado READY.
 * @param  t_query* query: La query a agregar.
 */
void queryAReady(t_query* query);

/**
 * @brief: Agrega una query a la lista de queries en estado EXEC.
 * @param  t_query* query: La query a agregar.
 */
void queryAExec(t_query* query);

/**
 * @brief: Agrega una query a la lista de queries en estado EXIT.
 * @param  t_query* query: La query a agregar.
 */ 
void queryAExit(t_query* query);

/**
 * @brief: Actualiza el estado de una query al indicado.
 * @param query: Una t_query*
 * @param nuevoEstado: El nuevo estado a asignar a la query. es un e_estado_query
 */
void actualizarEstadoQuery(t_query* query, e_estado_query nuevoEstado);



#endif