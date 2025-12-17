#include "conexion.h"
#include "operaciones.h"

void* atender_worker(void* thread_args) {
    t_thread_args* thread_args_ptr = (t_thread_args*) thread_args;
    t_paquete* paquete_ptr = thread_args_ptr->paquete;
    int* conexion_worker_ptr = thread_args_ptr->fd_conexion;

    int socket_cliente = *conexion_worker_ptr;

    free(conexion_worker_ptr);
    free(thread_args_ptr);

    t_handshake_worker_storage* handshake = deserializar_handshake_worker_storage(paquete_ptr->datos);
    int id_worker = handshake->id_worker;
    //printf("ID WORKER: %d\n", handshake->id_worker);
    pthread_mutex_lock(&g_mutexCantidadWorkers);
    g_cantidadWorkers++;
    pthread_mutex_unlock(&g_mutexCantidadWorkers);
    log_info(g_logger_storage, "##Se conecta el Worker <%d> - Cantidad de Workers: <%d>",id_worker, g_cantidadWorkers);
    destruir_paquete(paquete_ptr);
    free(handshake);

    //enviar_string(*conexion_query_control_ptr, "Storage dice: Handshake recibido");

    t_tam_pagina* tam_pagina_struct = malloc(sizeof(t_tam_pagina));
    tam_pagina_struct->tam_pagina = g_superblock_config->block_size;

    t_buffer* buffer_tam_pagina = serializar_tam_pagina(tam_pagina_struct);
    t_paquete* paquete_tam_pagina = empaquetar_buffer(HANDSHAKE_WORKER_STORAGE, buffer_tam_pagina);
    enviar_paquete(socket_cliente, paquete_tam_pagina);

    free(tam_pagina_struct);

    
    while(1){
        t_paquete* paquete = recibir_paquete(socket_cliente);

        if(paquete == NULL){
            pthread_mutex_lock(&g_mutexCantidadWorkers);
            g_cantidadWorkers--;
            pthread_mutex_unlock(&g_mutexCantidadWorkers);
            log_info(g_logger_storage, "##Se desconecta el Worker <%d> - Cantidad de Workers: <%d>",id_worker,g_cantidadWorkers);
            break;     
        }
        int resultado = 0;
        switch(paquete->codigo_operacion){
            
            case OP_CREATE:
                t_create* create_instr = deserializar_create(paquete->datos);
                log_info(g_logger_storage, "Operacion CREATE recibida del Worker <%d> para el archivo <%s> con tag <%s>", id_worker, create_instr->file_name, create_instr->tag_name);
                resultado = create(create_instr->id_query, create_instr->file_name, create_instr->tag_name);
                log_debug(g_logger_storage, "Resultado operacion CREATE: %d, enviando a worker", resultado);

                enviar_entero(socket_cliente, resultado);

                free(create_instr->file_name);
                free(create_instr->tag_name);
                free(create_instr);
                break;

            case OP_TRUNCATE:
                t_truncate* truncate_instr = deserializar_truncate(paquete->datos);
                log_info(g_logger_storage, "Operacion TRUNCATE recibida del Worker <%d> para el archivo <%s> con tag <%s> a tamaño <%d>", id_worker, truncate_instr->file_name, truncate_instr->tag_name, truncate_instr->size);
                resultado = truncate_file(truncate_instr->id_query, truncate_instr->file_name, truncate_instr->tag_name, truncate_instr->size);

                log_debug(g_logger_storage, "Resultado operacion TRUNCATE: %d, enviando a worker", resultado);

                enviar_entero(socket_cliente, resultado);
                free(truncate_instr->file_name);
                free(truncate_instr->tag_name);
                free(truncate_instr);

                break;
            
            case OP_TAG:
                t_tag* tag_instr = deserializar_tag(paquete->datos);
                log_info(g_logger_storage, "Operacion TAG recibida del Worker <%d> para el archivo <%s> desde tag <%s> a tag <%s>", id_worker, tag_instr->file_name_origen, tag_instr->tag_name_origen, tag_instr->tag_name_destino);
                resultado = tag(tag_instr->id_query, tag_instr->file_name_origen, tag_instr->tag_name_origen, tag_instr->tag_name_destino);
                log_debug(g_logger_storage, "Resultado operacion TAG: %d, enviando a worker", resultado);

                enviar_entero(socket_cliente, resultado);

                free(tag_instr->file_name_origen);
                free(tag_instr->tag_name_origen);
                free(tag_instr->file_name_destino);
                free(tag_instr->tag_name_destino);
                free(tag_instr);
                break;

            case OP_COMMIT:
                t_tag* commit_instr = deserializar_tag(paquete->datos);
                log_info(g_logger_storage, "Operacion COMMIT recibida del Worker <%d> para el archivo <%s> con tag <%s>", id_worker, commit_instr->file_name_origen, commit_instr->tag_name_origen);
                resultado = commitFile(commit_instr->id_query, commit_instr->file_name_origen, commit_instr->tag_name_origen);
                log_debug(g_logger_storage, "Resultado operacion COMMIT: %d, enviando a worker", resultado);    

                enviar_entero(socket_cliente, resultado);

                free(commit_instr->file_name_origen);
                free(commit_instr->tag_name_origen);
                free(commit_instr);
                break;
            case OP_DELETE:
                t_delete* delete_instr = deserializar_delete(paquete->datos);
                log_info(g_logger_storage, "Operacion DELETE recibida del Worker <%d> para el archivo <%s> con tag <%s>", id_worker, delete_instr->file_name, delete_instr->tag_name);
                resultado = eliminarTag(delete_instr->id_query, delete_instr->file_name, delete_instr->tag_name);
                log_debug(g_logger_storage, "Resultado operacion DELETE: %d, enviando a worker", resultado);

                enviar_entero(socket_cliente, resultado);

                free(delete_instr->file_name);
                free(delete_instr->tag_name);
                free(delete_instr);
                break;

            case OP_READ:
                //Deserializo la solicitud de read
                void* buffer_leido = NULL;
                t_sol_read* read_instr = deserializar_solicitud_read(paquete->datos);
                log_info(g_logger_storage, "Operacion READ recibida del Worker <%d> para el archivo <%s> con tag <%s> en bloque lógico <%d>", id_worker, read_instr->file_name, read_instr->tag_name, read_instr->numero_bloque);
                resultado = leer(read_instr->id_query, read_instr->file_name, read_instr->tag_name, read_instr->numero_bloque, &buffer_leido);

                if (resultado != 0) {
                    t_buffer* buffer_cod_error = serializar_cod_error(&resultado);
                    t_paquete* paquete_cod_error = empaquetar_buffer(OP_ERROR, buffer_cod_error);
                    log_debug(g_logger_storage, "Error en operacion READ: %d, enviando a worker", resultado);
                    enviar_paquete(socket_cliente, paquete_cod_error);

                    free(buffer_cod_error);
                } else {
                    t_bloque_leido* bloque_leido = malloc(sizeof(t_bloque_leido));
                    bloque_leido->contenido = buffer_leido;
                    bloque_leido->tamanio = g_superblock_config->block_size;

                    t_buffer* buffer_bloque_leido = serializar_bloque_leido(bloque_leido);

                    t_paquete* paquete_bloque_leido = empaquetar_buffer(OP_READ, buffer_bloque_leido);
                    enviar_paquete(socket_cliente, paquete_bloque_leido);

                    //free(bloque_leido);
                    //free(buffer_bloque_leido);
                }
                
                free(read_instr->file_name);
                free(read_instr->tag_name);
                free(read_instr);
                break;
            
            case OP_WRITE:
                //TODO
                log_debug(g_logger_storage, "Operacion WRITE recibida del Worker <%d> - no hay respuesta hacia worker pero la ejecutamos", id_worker);
                t_sol_write* write_instr = deserializar_solicitud_write(paquete->datos);
                resultado = escribirBloque(write_instr->id_query, write_instr->file_name, write_instr->tag_name, write_instr->numero_bloque, write_instr->contenido);
                log_debug(g_logger_storage, "Resultado operacion WRITE: %d", resultado);
                enviar_entero(socket_cliente, resultado);
                free(write_instr->file_name);
                free(write_instr->tag_name);
                free(write_instr->contenido);
                free(write_instr);
            break;

            default:
                log_warning(g_logger_storage, "Operacion desconocida recibida del Worker <%d>", id_worker);
                break;
                destruir_paquete(paquete);
        
        
    }
    destruir_paquete(paquete);
    }
    log_debug(g_logger_storage, "Cerrando socket %d (Worker %d) para liberar recursos.", socket_cliente, id_worker);
    close(socket_cliente);
    return NULL;
}