#include "conexion.h"
#include "m_funciones.h"

// Traemos variable externa para actualizar nivel de multiprogramación
extern pthread_mutex_t mutexnivelMultiprocesamiento;

// HILO DE ATENCION DE CONEXIONES
void* iniciar_receptor(void* socket_servidor) {
    int socket_servidor_ptr = *((int*) socket_servidor); // Casteo a int* y desreferencio
    log_debug(logger_master, "Socket casteado y desreferenciado: %d, procedo a escuchar conexiones", socket_servidor_ptr);
    
    // Recepcion de conexiones
    while (1) {
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int)); // Esto hay que liberarlo en el hilo que atiende la conexion
        *fd_conexion_ptr = esperar_cliente(socket_servidor_ptr);
        
        if(*fd_conexion_ptr == -1){
            fprintf(stderr, "Error al esperar cliente\n");
            free(fd_conexion_ptr); // Liberar si falla
            return NULL;
        }

        t_paquete* paquete = recibir_paquete(*fd_conexion_ptr);
        if(paquete == NULL){
            fprintf(stderr, "Error al recibir paquete inicial\n");
            close(*fd_conexion_ptr); // IMPORTANTE: Cerrar socket si falla la recepción inicial
            free(fd_conexion_ptr);
            return NULL; // O continuar esperando otros clientes
        }

        switch(paquete->codigo_operacion) {
            t_thread_args* thread_args;
            case HANDSHAKE_QC_MASTER: // Conexión de Query Control
                thread_args = malloc(sizeof(t_thread_args));
                thread_args->paquete = paquete;
                thread_args->fd_conexion = fd_conexion_ptr;

                // HILO DE ATENCION A QUERY CONTROL
                pthread_create(&thread, NULL, atender_query_control, (void*) thread_args);
                pthread_detach(thread);
                break;
            case HANDSHAKE_WORKER_MASTER: // Conexión de Worker
                thread_args = malloc(sizeof(t_thread_args));
                thread_args->paquete = paquete;
                thread_args->fd_conexion = fd_conexion_ptr;

                // HILO DE ATENCION A WORKER
                pthread_create(&thread, NULL, atender_worker, (void*) thread_args);
                pthread_detach(thread);
                break;
            default:
                fprintf(stderr, "Error: el paquete no es de tipo HANDSHAKE adecuado\n");
                destruir_paquete(paquete);
                free(fd_conexion_ptr);
                break; // No retornamos NULL para no matar al servidor, solo ignoramos la conexión errónea
        }
    }
    return NULL;
}

// HILO DE ATENCION A QUERY CONTROL
void* atender_query_control(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_query_control_ptr = thread_args_ptr->fd_conexion;

    t_handshake_qc_master* handshake = deserializar_handshake_qc_master(paquete_ptr->datos);
    log_debug(logger_master, "Handshake recibido de Query Control. Archivo: %s, Prioridad: %d", 
        handshake->archivo_query, handshake->prioridad);

    enviar_string(*conexion_query_control_ptr, "Master dice: Handshake recibido");

    // Creamos la query y la ponemos en READY
    t_query* nuevaQuery = crearNuevaQuery(handshake->archivo_query, 
        handshake->prioridad, *conexion_query_control_ptr);

    // Log Obligatorio - Conexión de Query Control
    log_info(logger_master, 
        "## Se conecta un Query Control para ejecutar la Query %s con prioridad %d - Id asignado: %d. Nivel multiprocesamiento %d", 
        nuevaQuery->archivoQuery, nuevaQuery->prioridad, nuevaQuery->id_query, nivelMultiprocesamiento );

    if(strcmp(master_config->algoritmo_planificacion, "PRIORIDADES") == 0){
        pthread_t thread;

        // HILO DE AGING PARA LA QUERY
        pthread_create(&thread, NULL, aging_de_query, (void*) nuevaQuery);
        pthread_detach(thread);
    }

    queryAReady(nuevaQuery);
    sem_post(&semPlanificador); // Avisamos al planificador que hay una nueva query
    
    // Limpieza de recursos del handshake
    destruir_paquete(paquete_ptr);
    free(handshake->archivo_query);
    free(handshake);

    // Loop de escucha para detectar desconexión
    while(1) {
        t_paquete* paquete = recibir_paquete(*conexion_query_control_ptr);
        if(paquete == NULL) {

            // Log Obligatorio - Desconexión de Query Control
            log_info(logger_master, "## Se desconecta un Query Control. Se finaliza la Query %d con prioridad %d. Nivel multiprocesamiento %d",
                nuevaQuery->id_query, nuevaQuery->prioridad, nivelMultiprocesamiento );
            
            if (nuevaQuery->estado == Q_READY) {
                actualizarEstadoQuery(nuevaQuery, Q_EXIT);
            } else if (nuevaQuery->estado == Q_EXEC) {
                // Notificar al Worker que debe cancelar la ejecución
                pthread_mutex_lock(&mutexListaWorkers);
                for (int i = 0; i < list_size(listaWorkers); i++) {
                    t_worker_interno* worker = list_get(listaWorkers, i);
                    if (worker->query != NULL && worker->query->id_query == nuevaQuery->id_query) {
                        log_debug(logger_master, "Enviando solicitud de cancelación al Worker %d para Query %d", 
                                 worker->id_worker, nuevaQuery->id_query);
                        
                        t_buffer* buffer_vacio = crear_buffer(0);
                        t_paquete* paquete_fin = empaquetar_buffer(OP_FIN_QUERY, buffer_vacio);
                        enviar_paquete(worker->socket_fd, paquete_fin);
                        break;
                    }
                }
                pthread_mutex_unlock(&mutexListaWorkers);
            }
            break;
        }
        // Si el QC manda otros mensajes, procesarlos aquí
        destruir_paquete(paquete); 
    }

    close(*conexion_query_control_ptr); // Cerrar el socket del sistema operativo
    free(conexion_query_control_ptr);
    free(thread_args_ptr);

    return NULL;
}


// -----------------------------------------------------------------------------
// --- GESTIÓN DE WORKERS: Registro, Lista y Bucle de Escucha ---
// -----------------------------------------------------------------------------

void* atender_worker(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_worker_ptr = thread_args_ptr->fd_conexion;

    // 1. Deserializar handshake para obtener ID del Worker
    t_handshake_worker_master* handshake = deserializar_handshake_worker_master(paquete_ptr->datos);
    log_debug(logger_master, "Handshake recibido de Worker ID: %d", handshake->id_worker);

    // 2. Responder al handshake confirmando conexión
    enviar_string(*conexion_worker_ptr, "Master dice: Handshake OK");

    // 3. Crear estructura interna y agregarlo a la lista de gestión
    t_worker_interno* nuevoWorker = malloc(sizeof(t_worker_interno));
    nuevoWorker->socket_fd = *conexion_worker_ptr;
    nuevoWorker->id_worker = handshake->id_worker;
    nuevoWorker->libre = true; // Al conectarse está libre para recibir trabajo

    pthread_mutex_lock(&mutexListaWorkers);
    list_add(listaWorkers, nuevoWorker);
    pthread_mutex_unlock(&mutexListaWorkers);
    
    log_debug(logger_master, "Worker ID %d agregado a la lista de gestión interna", nuevoWorker->id_worker);

    // 4. Actualizar métricas (nivel de multiprogramación)
    pthread_mutex_lock(&mutexnivelMultiprocesamiento );
    nivelMultiprocesamiento ++;
    pthread_mutex_unlock(&mutexnivelMultiprocesamiento );
    
    // Log Obligatorio - Conexión de Worker
    log_info(logger_master, "## Se conecta el Worker %d. Cantidad total de Workers: %d", 
        nuevoWorker->id_worker, nivelMultiprocesamiento );

    // Liberar memoria temporal del handshake
    destruir_paquete(paquete_ptr);
    free(handshake);

    // 5. BUCLE PRINCIPAL DE ESCUCHA (Keep-Alive)
    // Este hilo se queda bloqueado esperando mensajes de este Worker específico
    while(1) {
        t_paquete* paquete = recibir_paquete(nuevoWorker->socket_fd);  
        
        if(paquete == NULL) {
            // TODO: Caso de desconexión (recv devuelve 0 o error) Revisar si realmente va aca o donde dice Mechi
            /*
            Al momento de que un Worker se desconecte, la Query que se encontraba en ejecución en 
            dicho Worker se finalizará con error y se notificará al Query Control correspondiente.
             */
            log_warning(logger_master, "Worker %d se ha desconectado.", nuevoWorker->id_worker);

            // Log Obligatorio - Desconexión de Worker
            log_info(logger_master, "## Se desconecta el Worker %d - Se finaliza la Query %d - Cantidad total de Workers: %d",
                nuevoWorker->id_worker, nuevoWorker->query->id_query, nivelMultiprocesamiento);
            break;
        }

        switch(paquete->codigo_operacion) {
            
            // CASO CLAVE: El worker terminó una tarea
            case OP_RESULTADO_QUERY:
                t_fin_query* resultado = deserializar_resultado_query(paquete->datos); // Asegúrate de tener este enum en estructuras.h
                switch (resultado->estado)
                {
                case EXEC_FIN_QUERY:

                    break;
                case EXEC_DESALOJO:

                    /* code */
                    break;
                    //ERRORRRRRRR
                case resultado->estado < 0:

                    /* code */
                    break;
                default:
                    break;
                }
                // TODO: Revisar finalizacion de la query (exito, error, desalojo, etc.)
                //log_info(logger_master, "Worker %d finalizó una tarea con éxito.", nuevoWorker->id_worker);
                
                // Aquí podrías deserializar el resultado si el worker manda datos
                // t_resultado* res = deserializar_resultado(paquete->datos);
                
                // CRÍTICO: Marcar al Worker como LIBRE nuevamente
                pthread_mutex_lock(&mutexListaWorkers);
                if (nuevoWorker->query != NULL) {
                    actualizarEstadoQuery(nuevoWorker->query, Q_EXIT);
                    nuevoWorker->query = NULL;
                }
                nuevoWorker->libre = true;
                pthread_mutex_unlock(&mutexListaWorkers);
                sem_post(&semPlanificador); // Avisamos al planificador que hay un worker libre
                log_debug(logger_master, "Worker %d marcado como LIBRE. Listo para próxima tarea.", nuevoWorker->id_worker);
                break;

            // Aquí puedes agregar más casos (ej: OP_ERROR, logs intermedios, etc.)
            case OP_READ:
                // Procesar mensaje de lectura si es necesario
                log_debug(logger_master, "Worker %d envió un mensaje READ.", nuevoWorker->id_worker);
                enviar_paquete(nuevoWorker->query->socketQuery, paquete)

                break;
            default:
                log_warning(logger_master, "Mensaje desconocido del Worker %d (OpCode: %d)", 
                            nuevoWorker->id_worker, paquete->codigo_operacion);
                break;
        }
        
        log_info(logger_master, "Mensaje recibido del Worker %d (código operación: %d)", 
            nuevoWorker->id_worker, paquete->codigo_operacion);

        destruir_paquete(paquete);
    }

    // 6. LIMPIEZA POST DESCONEXIÓN
    // Eliminar de la lista de workers para que el planificador no intente asignarle nada
    pthread_mutex_lock(&mutexListaWorkers);
    
    // Eliminamos directamente por puntero, que es más eficiente y estándar.
    // Esto funciona porque 'nuevoWorker' sigue apuntando a la misma dirección de memoria que guardamos.
    list_remove_element(listaWorkers, nuevoWorker);

    // ¡Aquí borré la línea que daba error!

    pthread_mutex_unlock(&mutexListaWorkers);

    // Decrementar nivel de multiprogramación
    pthread_mutex_lock(&mutexnivelMultiprocesamiento );
    nivelMultiprocesamiento --;
    pthread_mutex_unlock(&mutexnivelMultiprocesamiento );

    log_info(logger_master, "## Se desconecta el Worker %d. Cantidad total de Workers: %d", 
        nuevoWorker->id_worker, nivelMultiprocesamiento );

    // mechi TODO: Si el worker estaba ejecutando algo (nuevoWorker->libre == false) al momento de desconectarse,
    // deberías manejar el fallo de esa query aquí para no dejarla "colgada" en EXEC.

    close(nuevoWorker->socket_fd); // Cerrar el socket del sistema operativo
    // Liberación de memoria
    free(nuevoWorker);
    free(conexion_worker_ptr); // Liberamos el puntero al FD creado en iniciar_receptor
    free(thread_args_ptr);

    return NULL;
}