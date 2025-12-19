#ifndef M_FUNCIONES_H
#define M_FUNCIONES_H   

#include <stdio.h>
#include <stdlib.h>
#include <utils/loggeo.h>
#include <utils/configs.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <utils/estructuras.h>
#include <utils/mensajeria.h>
#include <semaphore.h>

// Variables globales
extern t_log* logger_master;

extern int identificadorQueryGlobal; //La inicilizamos en master.c, en 0

/**
 * @brief: Nivel de multiprocesamiento actual (cantidad de workers conectados)
 */
extern int nivelMultiprocesamiento ; //La inicilizamos en master.c, en 0

extern char const* estadosQuery[]; //La inicilizamos en m_funciones.c. Es para traducir los enum a string, para loggear.
extern master_conf* master_config; //La inicilizamos en master.c, con la config del master

extern t_list* listaQueriesReady;
extern t_list* listaQueriesExec;
extern t_list* listaQueriesExit;

// Lista global de Workers 
extern t_list* listaWorkers;

extern pthread_mutex_t mutexListaQueriesReady;
extern pthread_mutex_t mutexListaQueriesExec;
extern pthread_mutex_t mutexListaQueriesExit;
extern pthread_mutex_t mutexIdentificadorQueryGlobal; 
extern pthread_mutex_t mutexnivelMultiprocesamiento ;
// Mutex para la lista de workers 
extern pthread_mutex_t mutexListaWorkers;

extern sem_t semPlanificador;

// Definición de estados para las queries
typedef enum {
    Q_READY=0,
    Q_EXEC,
    Q_EXIT
} e_estado_query;


// Estructura interna para representar una Query en el Master
/**
 * @brief: Estructura que representa una Query en el Master.
 * @param id_query: Identificador único de la Query.
 * @param prioridad: Prioridad de la Query (0 = más alta).
 * @param archivoQuery: Path al archivo de la Query.
 * @param estado: Estado actual de la Query (READY, EXEC, EXIT).
 * @param socketQuery: Socket para comunicarse con el Query Control que envió la Query
 */
typedef struct{
    uint32_t id_query;
    uint8_t prioridad;
    char* archivoQuery;
    e_estado_query estado;
    int socketQuery; //socket para comunicarse con el query control que envio la query
    u_int32_t pc; //program counter para reanudar query desalojada
    bool desconexion_solicitada; // Flag para indicar si se solicitó desconexión (QC desconectado)
} t_query;


// Estructura interna para representar un Worker 
typedef struct {
    int socket_fd;
    uint32_t id_worker;
    bool libre; // true = LIBRE, false = OCUPADO
    t_query* query;
} t_worker_interno;


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

//funciones para planificador
void* iniciar_planificador(void* arg);

t_worker_interno* obtener_worker_libre();

t_query* obtener_siguiente_query_fifo();

void* aging_de_query(void* query);

// t_query* obtener_siguiente_query_prioridades(); // Para más adelante

void finalizarMaster();

#endif