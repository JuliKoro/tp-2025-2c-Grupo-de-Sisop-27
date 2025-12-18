#include "query_interpreter.h"

// Variables externas
extern t_log* logger_worker;
//extern worker_conf* worker_configs;
extern int conexion_storage;
extern int conexion_master;
extern uint32_t id_worker;

// ============================================================================
// FUNCIÓN PRINCIPAL DEL INTERPRETER
// ============================================================================

t_resultado_ejecucion query_interpreter() {
    // Construir path completo del archivo de query
    char path_completo[512];
    snprintf(path_completo, sizeof(path_completo), "%s/%s", worker_configs->path_queries, path_query);
    
    log_debug(logger_worker, "Path completo de operaciones es: %s", path_completo);
    
    // Abrir archivo de query
    FILE* archivo = abrir_archivo_query(path_completo);
    if (archivo == NULL) {
        log_error(logger_worker, "Error al abrir el archivo de query: %s", path_completo);
        return EXEC_ERROR;
    }
    
    t_resultado_ejecucion resultado = EXEC_OK; // Resultado de la ejecución de la Query
    
    // Ciclo de instruccion
    while (true) {
        // Verificar si se debe desalojar
        if (flag_desalojo_query) {
            // Log Obligatorio - Desalojo de Query
            log_info(logger_worker, "## Query %d: Desalojada por pedido del Master", id_query);
            fclose(archivo);
            return EXEC_DESALOJO;
        }
        
        // FETCH: Obtener la instrucción actual
        char* instruccion_str = fetch_instruction(archivo);
        if (instruccion_str == NULL) {
            log_error(logger_worker, "Error al hacer fetch de la instrucción en PC: %d", pc_actual);
            resultado = EXEC_ERROR;
            break;
        }
        
        // DECODE: Parsear la instrucción
        t_instruccion* instruccion = parse_instruction(instruccion_str);
        free(instruccion_str);

        // Log Obligatorio - Fetch Instrucción
        log_info(logger_worker, "## Query %d: FETCH - Program Counter: %d - %s", 
                 id_query, pc_actual, tipo_instruccion_to_string(instruccion->tipo));
        
        if (instruccion == NULL) {
            log_error(logger_worker, "Error al parsear la instrucción en PC: %d", pc_actual);
            resultado = EXEC_ERROR;
            break;
        }
        
        // EXECUTE: Ejecutar la instrucción
        t_resultado_ejecucion resultado = execute_instruction(instruccion); // Resulado de la ejecución de la instrucción
        
        // Log obligatorio - Instrucción realizada
        if (resultado == EXEC_OK || resultado == EXEC_FIN_QUERY) {
            log_info(logger_worker, "## Query %d: - Instrucción realizada: %s", 
                     id_query, tipo_instruccion_to_string(instruccion->tipo));
        }
        
        destruir_instruccion(instruccion);
        
        // Verificar resultado de la ejecución de la instrucción
        if (resultado < 0) {
            log_error(logger_worker, "Error al ejecutar la instrucción en PC: %d", pc_actual);
            break;
        }
        
        if (resultado == EXEC_FIN_QUERY) {
            log_debug(logger_worker, "Query %d finalizada correctamente", id_query);
            pc_actual++;
            break;
        }
        
        // Incrementar PC para la siguiente instrucción
        pc_actual++;
    }
    
    fclose(archivo);
    return resultado;
}

// ============================================================================
// FETCH - DECODE - EXECUTE
// ============================================================================

char* fetch_instruction(FILE* archivo) {
    // Volver al inicio del archivo
    rewind(archivo);

    char* linea = NULL;
    size_t len = 0;
    ssize_t read;
    uint32_t linea_actual = 0;

    // Leer hasta llegar a la línea del PC
    while ((read = getline(&linea, &len, archivo)) != -1) {
        if (linea_actual == pc_actual) {
            // Eliminar el salto de línea al final
            if (linea[read - 1] == '\n') {
                linea[read - 1] = '\0';
            }
            return linea;
        }
        linea_actual++;
    }

    // No se encontró la línea
    if (linea != NULL) {
        free(linea);
    }
    return NULL;
}

t_instruccion* parse_instruction(char* instruccion_str) {
    if (instruccion_str == NULL || strlen(instruccion_str) == 0) {
        return NULL;
    }
    
    // Crear estructura de instrucción
    t_instruccion* inst = malloc(sizeof(t_instruccion));
    if (inst == NULL) {
        return NULL;
    }
    
    // Inicializar campos
    memset(inst, 0, sizeof(t_instruccion));
    inst->instruccion_raw = strdup(instruccion_str);
    
    // Dividir la instrucción en tokens
    char** tokens = string_split(instruccion_str, " ");
    if (tokens == NULL || tokens[0] == NULL) {
        destruir_instruccion(inst);
        return NULL;
    }
    
    // Determinar el tipo de instrucción
    if (strcmp(tokens[0], "CREATE") == 0) {
        inst->tipo = INST_CREATE;
        if (tokens[1] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
        }
    }
    else if (strcmp(tokens[0], "TRUNCATE") == 0) {
        inst->tipo = INST_TRUNCATE;
        if (tokens[1] != NULL && tokens[2] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
            inst->tamanio = atoi(tokens[2]);
        }
    }
    else if (strcmp(tokens[0], "WRITE") == 0) {
        inst->tipo = INST_WRITE;
        if (tokens[1] != NULL && tokens[2] != NULL && tokens[3] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
            inst->direccion_base = atoi(tokens[2]);
            inst->contenido = strdup(tokens[3]);
        }
    }
    else if (strcmp(tokens[0], "READ") == 0) {
        inst->tipo = INST_READ;
        if (tokens[1] != NULL && tokens[2] != NULL && tokens[3] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
            inst->direccion_base = atoi(tokens[2]);
            inst->tamanio = atoi(tokens[3]);
        }
    }
    else if (strcmp(tokens[0], "TAG") == 0) {
        inst->tipo = INST_TAG;
        if (tokens[1] != NULL && tokens[2] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
            split_file_tag(tokens[2], &inst->file_name_dest, &inst->tag_name_dest);
        }
    }
    else if (strcmp(tokens[0], "COMMIT") == 0) {
        inst->tipo = INST_COMMIT;
        if (tokens[1] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
        }
    }
    else if (strcmp(tokens[0], "FLUSH") == 0) {
        inst->tipo = INST_FLUSH;
        if (tokens[1] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
        }
    }
    else if (strcmp(tokens[0], "DELETE") == 0) {
        inst->tipo = INST_DELETE;
        if (tokens[1] != NULL) {
            split_file_tag(tokens[1], &inst->file_name, &inst->tag_name);
        }
    }
    else if (strcmp(tokens[0], "END") == 0) {
        inst->tipo = INST_END;
    }
    else {
        inst->tipo = INST_UNKNOWN;
    }
    
    // Liberar tokens
    string_iterate_lines(tokens, (void*)free);
    free(tokens);
    
    log_debug(logger_worker, "Instrucción parseada: Tipo=%s, Raw=%s", 
              tipo_instruccion_to_string(inst->tipo), inst->instruccion_raw);
    return inst;
}

t_resultado_ejecucion execute_instruction(t_instruccion* instruccion) {
    if (instruccion == NULL) {
        return EXEC_ERROR;
    }
    
    t_resultado_ejecucion resultado = EXEC_ERROR;
    
    switch (instruccion->tipo) {
        case INST_CREATE:
            resultado = execute_create(instruccion->file_name, instruccion->tag_name);
            break;
            
        case INST_TRUNCATE:
            resultado = execute_truncate(instruccion->file_name, instruccion->tag_name, instruccion->tamanio);
            break;
            
        case INST_WRITE:
            resultado = execute_write(instruccion->file_name, instruccion->tag_name, instruccion->direccion_base, instruccion->contenido);
            break;
            
        case INST_READ:
            resultado = execute_read(instruccion->file_name, instruccion->tag_name, instruccion->direccion_base, instruccion->tamanio);
            break;
            
        case INST_TAG:
            resultado = execute_tag(instruccion->file_name, instruccion->tag_name, instruccion->file_name_dest, instruccion->tag_name_dest);
            break;
            
        case INST_COMMIT:
            // FLUSH implícito antes de COMMIT
            execute_flush(instruccion->file_name, instruccion->tag_name);
            resultado = execute_commit(instruccion->file_name, instruccion->tag_name);
            break;
            
        case INST_FLUSH:
            resultado = execute_flush(instruccion->file_name, instruccion->tag_name);
            break;
            
        case INST_DELETE:
            resultado = execute_delete(instruccion->file_name, instruccion->tag_name);
            break;
            
        case INST_END:
            resultado = execute_end();
            break;
            
        default:
            log_error(logger_worker, "Tipo de instrucción desconocida");
            return EXEC_ERROR;
    }
    
    return resultado;
}

// ============================================================================
// EJECUCIÓN DE INSTRUCCIONES ESPECÍFICAS
// ============================================================================

t_resultado_ejecucion execute_create(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "CREATE: Parámetros inválidos");
        return EXEC_ERROR;
    }

    log_debug(logger_worker, "Ejecutando CREATE %s:%s", file_name, tag_name);

    t_create* sol_create = malloc(sizeof(t_create));
    sol_create->file_name = file_name;
    sol_create->tag_name = tag_name;

    if (!enviar_instruccion(OP_CREATE, sol_create)) {
        log_error(logger_worker, "Error al enviar solicitud CREATE al Storage");
        return ERROR_CONEXION;
    }

    t_resultado_ejecucion respuesta_storage = recibir_respuesta_storage();

    return respuesta_storage;
}

t_resultado_ejecucion execute_truncate(char* file_name, char* tag_name, uint32_t tamanio) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "TRUNCATE: Parámetros inválidos");
        return EXEC_ERROR;
    }
    
    log_debug(logger_worker, "Ejecutando TRUNCATE %s:%s %d", file_name, tag_name, tamanio);
    
    t_truncate* sol_truncate = malloc(sizeof(t_truncate));
    sol_truncate->file_name = file_name;
    sol_truncate->tag_name = tag_name;
    sol_truncate->size = tamanio;

    if (!enviar_instruccion(OP_TRUNCATE, sol_truncate)) {
        log_error(logger_worker, "Error al enviar solicitud TRUNCATE al Storage");
        return ERROR_CONEXION;
    }

    t_resultado_ejecucion respuesta_storage = recibir_respuesta_storage();

    return respuesta_storage;
}

t_resultado_ejecucion execute_write(char* file_name, char* tag_name, uint32_t direccion_base, char* contenido) {
    if (file_name == NULL || tag_name == NULL || contenido == NULL) {
        log_error(logger_worker, "WRITE: Parámetros inválidos");
        return EXEC_ERROR;
    }
    
    log_debug(logger_worker, "Ejecutando WRITE %s:%s %d %s", 
              file_name, tag_name, direccion_base, contenido);
    
    // Calcular tamaño del contenido
    uint32_t tamanio = strlen(contenido);

    // Escribir en memoria interna
    t_resultado_ejecucion resultado = escribir_memoria(file_name, tag_name, direccion_base, contenido, tamanio);
    if (resultado != EXEC_OK) {
        log_error(logger_worker, "Error al escribir en memoria interna");
        return resultado;
    }
    
    return EXEC_OK;
}

t_resultado_ejecucion execute_read(char* file_name, char* tag_name, uint32_t direccion_base, uint32_t tamanio) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "READ: Parámetros inválidos");
        return EXEC_ERROR;
    }
    
    log_debug(logger_worker, "Ejecutando READ %s:%s %d %d", 
              file_name, tag_name, direccion_base, tamanio);
    
    // Reservar buffer para el contenido
    char* buffer = malloc(tamanio + 1);
    if (!buffer) {
        log_error(logger_worker, "Error al asignar memoria para buffer de lectura");
        return EXEC_ERROR;
    }
    
    // Leer de memoria interna
    t_resultado_ejecucion resultado = leer_memoria(file_name, tag_name, direccion_base, tamanio, buffer);
    if (resultado != EXEC_OK) {
        log_error(logger_worker, "Error al leer de memoria interna");
        free(buffer);
        return resultado;
    }
    
    buffer[tamanio] = '\0'; // Null-terminator para strings

    if (!enviar_info_a_master(buffer, tamanio, file_name, tag_name)) {
        log_error(logger_worker, "Error al enviar contenido leído al Master");
        free(buffer);
        return ERROR_CONEXION;
    }
    
    free(buffer);
    return EXEC_OK;
}

t_resultado_ejecucion execute_tag(char* file_origen, char* tag_origen, char* file_destino, char* tag_destino) {
    if (file_origen == NULL || tag_origen == NULL || 
        file_destino == NULL || tag_destino == NULL) {
        log_error(logger_worker, "TAG: Parámetros inválidos");
        return EXEC_ERROR;
    }
    
    log_debug(logger_worker, "Ejecutando TAG %s:%s %s:%s", 
              file_origen, tag_origen, file_destino, tag_destino);
    
    t_tag* sol_tag = malloc(sizeof(t_tag));
    sol_tag->file_name_origen = file_origen;
    sol_tag->tag_name_origen = tag_origen;
    sol_tag->file_name_destino = file_destino;
    sol_tag->tag_name_destino = tag_destino;

    if (!enviar_instruccion(OP_TAG, sol_tag)) {
        log_error(logger_worker, "Error al enviar solicitud TAG al Storage");
        return ERROR_CONEXION;
    }

    t_resultado_ejecucion respuesta_storage = recibir_respuesta_storage();

    return respuesta_storage;
}

t_resultado_ejecucion execute_commit(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "COMMIT: Parámetros inválidos");
        return EXEC_ERROR;
    }
    
    log_debug(logger_worker, "Ejecutando COMMIT %s:%s", file_name, tag_name);

    t_commit* sol_commit = malloc(sizeof(t_commit));
    sol_commit->file_name = file_name;
    sol_commit->tag_name = tag_name;

    if (!enviar_instruccion(OP_COMMIT, sol_commit)) {
        log_error(logger_worker, "Error al enviar solicitud COMMIT al Storage");
        return ERROR_CONEXION;
    }

    t_resultado_ejecucion respuesta_storage = recibir_respuesta_storage();
    
    return respuesta_storage;
}

t_resultado_ejecucion execute_flush(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "FLUSH: Parámetros inválidos");
        return EXEC_ERROR;
    }
    
    log_debug(logger_worker, "Ejecutando FLUSH %s:%s", file_name, tag_name);

    // Flush del File:Tag
    t_resultado_ejecucion resultado = flush_file_tag(file_name, tag_name);
    if (resultado != EXEC_OK) {
        log_error(logger_worker, "Error al hacer flush de %s:%s", file_name, tag_name);
        return resultado;
    }
    
    log_debug(logger_worker, "FLUSH completado para %s:%s", file_name, tag_name);
    return EXEC_OK;
}

t_resultado_ejecucion execute_delete(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "DELETE: Parámetros inválidos");
        return EXEC_ERROR;
    }
    
    log_debug(logger_worker, "Ejecutando DELETE %s:%s", file_name, tag_name);
    
    t_delete* sol_delete = malloc(sizeof(t_delete));
    sol_delete->file_name = file_name;
    sol_delete->tag_name = tag_name;

    if (!enviar_instruccion(OP_DELETE, sol_delete)) {
        log_error(logger_worker, "Error al enviar solicitud DELETE al Storage");
        return ERROR_CONEXION;
    }

    t_resultado_ejecucion respuesta_storage = recibir_respuesta_storage();

    return respuesta_storage;
}

t_resultado_ejecucion execute_end() {
    log_debug(logger_worker, "Ejecutando END - Finalizando query");
    return EXEC_FIN_QUERY;
}

bool enviar_instruccion(e_codigo_operacion cod_op, void* estructura_inst) {
    t_buffer* buffer;
    switch (cod_op) {
        case OP_CREATE: buffer = serializar_create(estructura_inst); break;
        case OP_TRUNCATE: buffer = serializar_truncate(estructura_inst); break;
        case OP_WRITE: buffer = serializar_write(estructura_inst); break;
        case OP_READ: buffer = serializar_read(estructura_inst); break;
        case OP_TAG: buffer = serializar_tag(estructura_inst); break;
        case OP_COMMIT: buffer = serializar_commit(estructura_inst); break;
        case OP_FLUSH: buffer = serializar_flush(estructura_inst); break;
        case OP_DELETE: buffer = serializar_delete(estructura_inst); break;
        default:
            log_error(logger_worker, "Operación inválida para enviar instrucción al Storage");
            return false;
    }
    
    t_paquete* paquete = empaquetar_buffer(cod_op, buffer);

    if (enviar_paquete(conexion_storage, paquete) == -1) {
        log_error(logger_worker, "Error al enviar solicitud de instruccion al Storage");
        free(estructura_inst);
        return false;
    }

    free(estructura_inst);
    return true;
}

t_resultado_ejecucion recibir_respuesta_storage() {
    int respuesta;
    if (recibir_entero(conexion_storage, &respuesta) == -1) {
        log_error(logger_worker, "Error al recibir respuesta del Storage");
        return ERROR_CONEXION;
    }

    switch (respuesta) {
        case 0:
            log_debug(logger_worker, "Operación realizada con éxito en Storage");
            return EXEC_OK;
        case -2:
            log_error(logger_worker, "Error: El archivo no existe en Storage");
            return ERROR_FILE_NO_EXISTE;
        case -3:
            log_error(logger_worker, "Error: El archivo ya existe en Storage");
            return ERROR_YA_EXISTE;
        case -4:
            log_error(logger_worker, "Error: Espacio insuficiente en Storage");
            return ERROR_ESPACIO_INSUFICIENTE;
        case -5:
            log_error(logger_worker, "Error: Escritura no permitida en Storage");
            return ERROR_ESCRITURA_NO_PERMITIDA;
        case -6:
            log_error(logger_worker, "Error: Dirección fuera de límite en Storage");
            return ERROR_FUERA_DE_LIMITE;
        default:
            log_error(logger_worker, "Error desconocido recibido del Storage");
            return EXEC_ERROR;
    }

}

bool enviar_info_a_master(char* info, uint32_t size_info, char* file_name, char* tag_name) {

    t_bloque_leido* bloque = malloc(sizeof(t_bloque_leido));
    bloque->id_query = id_query;
    bloque->file_name = file_name;
    bloque->tag_name = tag_name;
    bloque->tamanio = size_info;
    bloque->contenido = info;

    t_buffer* buffer = serializar_bloque_leido(bloque);
    t_paquete* paquete = empaquetar_buffer(OP_READ, buffer);

    if (enviar_paquete(conexion_master, paquete) == -1) {
        log_error(logger_worker, "Error al enviar información al Master");
        free(bloque);
        return false;
    }

    free(bloque);
    return true;
}

bool notificar_resultado_a_master(t_resultado_ejecucion estado) {
    t_fin_query* resultado = malloc(sizeof(t_fin_query));
    resultado->id_query = id_query;
    resultado->estado = estado;
    resultado->pc_final = pc_actual;

    if (estado < 0) {
        log_warning(logger_worker, "Query %d abortada debido a un error. Cod. Error: %d - %s", 
                    id_query, estado, obtener_mensaje_resultado(estado));
    
    } else {
        log_debug(logger_worker, "Query %d desalojada de Worker - Motivo: %s", 
                    id_query, obtener_mensaje_resultado(estado));
    }
    
    t_buffer* buffer = serializar_resultado_query(resultado);
    t_paquete* paquete = empaquetar_buffer(OP_RESULTADO_QUERY, buffer);
    
    if (enviar_paquete(conexion_master, paquete) == -1) {
        log_error(logger_worker, "Error al notificar resultado de la Query %d al Master", id_query);
        free(resultado);
        return false;
    }
    
    free(resultado);
    return true;
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

bool split_file_tag(char* file_tag, char** file_name, char** tag_name) {
    if (file_tag == NULL) {
        return false;
    }

    // Usar string_split de commons para dividir por ':'
    char** tokens = string_split(file_tag, ":");
    if (tokens == NULL || tokens[0] == NULL || tokens[1] == NULL) {
        // Liberar tokens si fueron creados
        if (tokens != NULL) {
            string_iterate_lines(tokens, (void*)free);
            free(tokens);
        }
        return false;
    }

    // Verificar que no haya más de 2 tokens (solo file:tag)
    if (tokens[2] != NULL) {
        string_iterate_lines(tokens, (void*)free);
        free(tokens);
        return false;
    }

    // Asignar memoria y copiar los componentes
    *file_name = strdup(tokens[0]);
    *tag_name = strdup(tokens[1]);

    if (*file_name == NULL || *tag_name == NULL) {
        if (*file_name) free(*file_name);
        if (*tag_name) free(*tag_name);
        string_iterate_lines(tokens, (void*)free);
        free(tokens);
        return false;
    }

    // Liberar tokens
    string_iterate_lines(tokens, (void*)free);
    free(tokens);

    return true;
}

void destruir_instruccion(t_instruccion* instruccion) {
    if (instruccion == NULL) {
        return;
    }
    
    if (instruccion->file_name) free(instruccion->file_name);
    if (instruccion->tag_name) free(instruccion->tag_name);
    if (instruccion->file_name_dest) free(instruccion->file_name_dest);
    if (instruccion->tag_name_dest) free(instruccion->tag_name_dest);
    if (instruccion->contenido) free(instruccion->contenido);
    if (instruccion->instruccion_raw) free(instruccion->instruccion_raw);
    
    free(instruccion);
}

const char* tipo_instruccion_to_string(t_tipo_instruccion tipo) {
    switch (tipo) {
        case INST_CREATE:   return "CREATE";
        case INST_TRUNCATE: return "TRUNCATE";
        case INST_WRITE:    return "WRITE";
        case INST_READ:     return "READ";
        case INST_TAG:      return "TAG";
        case INST_COMMIT:   return "COMMIT";
        case INST_FLUSH:    return "FLUSH";
        case INST_DELETE:   return "DELETE";
        case INST_END:      return "END";
        default:            return "UNKNOWN";
    }
}

FILE* abrir_archivo_query(char* path_query) {
    if (path_query == NULL) {
        return NULL;
    }
    
    FILE* archivo = fopen(path_query, "r");
    if (archivo == NULL) {
        log_error(logger_worker, "No se pudo abrir el archivo: %s", path_query);
        return NULL;
    }
    
    return archivo;
}

uint32_t contar_lineas_archivo(FILE* archivo) {
    if (archivo == NULL) {
        return 0;
    }
    
    uint32_t lineas = 0;
    char ch;
    
    rewind(archivo);
    
    while ((ch = fgetc(archivo)) != EOF) {
        if (ch == '\n') {
            lineas++;
        }
    }
    
    rewind(archivo);
    return lineas;
}
