#include "query_interpreter.h"

// Variables externas
extern t_log* logger_worker;
extern worker_conf* worker_configs;
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
    
    log_info(logger_worker, "## Query %d: Se recibe la Query. El path de operaciones es: %s", id_query, path_completo);
    
    // Abrir archivo de query
    FILE* archivo = abrir_archivo_query(path_completo);
    if (archivo == NULL) {
        log_error(logger_worker, "Error al abrir el archivo de query: %s", path_completo);
        return EXEC_ERROR;
    }
    
    t_resultado_ejecucion resultado = EXEC_OK;
    
    // Ciclo de instruccion
    while (true) {
        // Verificar si se debe desalojar
        if (desalojar_query) {
            log_info(logger_worker, "## Query %d: Desalojada por pedido del Master", id_query);
            return EXEC_DESALOJO;
        }
        
        // FETCH: Obtener la instrucción actual
        char* instruccion_str = fetch_instruction(archivo, pc_actual);
        if (instruccion_str == NULL) {
            log_error(logger_worker, "Error al hacer fetch de la instrucción en PC: %d", pc_actual);
            resultado = EXEC_ERROR;
            break;
        }
        
        // Log obligatorio: FETCH
        log_info(logger_worker, "## Query %d: FETCH - Program Counter: %d - %s", 
                 id_query, pc_actual, instruccion_str);
        
        // DECODE: Parsear la instrucción
        t_instruccion* instruccion = parse_instruction(instruccion_str);
        free(instruccion_str);
        
        if (instruccion == NULL) {
            log_error(logger_worker, "Error al parsear la instrucción en PC: %d", pc_actual);
            resultado = EXEC_ERROR;
            break;
        }
        
        // EXECUTE: Ejecutar la instrucción
        t_resultado_ejecucion resultado_exec = execute_instruction(instruccion);
        
        // Log obligatorio: Instrucción realizada
        if (resultado_exec == EXEC_OK || resultado_exec == EXEC_FIN_QUERY) {
            log_info(logger_worker, "## Query %d: - Instrucción realizada: %s", 
                     id_query, tipo_instruccion_to_string(instruccion->tipo));
        }
        
        destruir_instruccion(instruccion);
        
        // Verificar resultado de la ejecución
        if (resultado_exec == EXEC_ERROR) {
            log_error(logger_worker, "Error al ejecutar la instrucción en PC: %d", pc_actual);
            resultado = EXEC_ERROR;
            break;
        }
        
        if (resultado_exec == EXEC_FIN_QUERY) {
            log_info(logger_worker, "Query %d finalizada correctamente", id_query);
            resultado = EXEC_OK;
            break;
        }
        
        if (resultado_exec == EXEC_DESALOJO) {
            resultado = EXEC_DESALOJO;
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

char* fetch_instruction(FILE* archivo, uint32_t pc) {
    // Volver al inicio del archivo
    rewind(archivo);
    
    char* linea = NULL;
    size_t len = 0;
    ssize_t read;
    uint32_t linea_actual = 0;
    
    // Leer hasta llegar a la línea del PC
    while ((read = getline(&linea, &len, archivo)) != -1) {
        if (linea_actual == pc) {
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
    
    // Dividir la instrucción en tokens usando commons
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
    
    return inst;
}

t_resultado_ejecucion execute_instruction(t_instruccion* instruccion) {
    if (instruccion == NULL) {
        return EXEC_ERROR;
    }
    
    bool resultado = false;
    
    switch (instruccion->tipo) {
        case INST_CREATE:
            resultado = execute_create(instruccion->file_name, instruccion->tag_name);
            break;
            
        case INST_TRUNCATE:
            resultado = execute_truncate(instruccion->file_name, instruccion->tag_name, 
                                        instruccion->tamanio);
            break;
            
        case INST_WRITE:
            resultado = execute_write(instruccion->file_name, instruccion->tag_name,
                                     instruccion->direccion_base, instruccion->contenido);
            break;
            
        case INST_READ:
            resultado = execute_read(instruccion->file_name, instruccion->tag_name,
                                    instruccion->direccion_base, instruccion->tamanio);
            break;
            
        case INST_TAG:
            resultado = execute_tag(instruccion->file_name, instruccion->tag_name,
                                   instruccion->file_name_dest, instruccion->tag_name_dest);
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
            return EXEC_FIN_QUERY;
            
        default:
            log_error(logger_worker, "Tipo de instrucción desconocida");
            return EXEC_ERROR;
    }
    
    return resultado ? EXEC_OK : EXEC_ERROR;
}

// ============================================================================
// EJECUCIÓN DE INSTRUCCIONES ESPECÍFICAS
// ============================================================================

bool execute_create(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "CREATE: Parámetros inválidos");
        return false;
    }
    
    // TODO: Enviar solicitud CREATE al Storage
    // Por ahora, solo log
    log_debug(logger_worker, "Ejecutando CREATE %s:%s", file_name, tag_name);
    
    // Simular retardo de operación si está configurado
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_truncate(char* file_name, char* tag_name, uint32_t tamanio) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "TRUNCATE: Parámetros inválidos");
        return false;
    }
    
    // TODO: Enviar solicitud TRUNCATE al Storage
    log_debug(logger_worker, "Ejecutando TRUNCATE %s:%s %d", file_name, tag_name, tamanio);
    
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_write(char* file_name, char* tag_name, uint32_t direccion_base, char* contenido) {
    if (file_name == NULL || tag_name == NULL || contenido == NULL) {
        log_error(logger_worker, "WRITE: Parámetros inválidos");
        return false;
    }
    
    // TODO: Escribir en memoria interna
    // 1. Verificar si las páginas necesarias están en memoria
    // 2. Si no están, cargarlas desde Storage (page fault)
    // 3. Escribir el contenido en memoria
    // 4. Marcar páginas como modificadas
    
    log_debug(logger_worker, "Ejecutando WRITE %s:%s %d %s", 
              file_name, tag_name, direccion_base, contenido);
    
    // Log obligatorio de escritura en memoria
    log_info(logger_worker, "Query %d: Acción: ESCRIBIR - Dirección Física: %d - Valor: %s",
             id_query, direccion_base, contenido);
    
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_read(char* file_name, char* tag_name, uint32_t direccion_base, uint32_t tamanio) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "READ: Parámetros inválidos");
        return false;
    }
    
    // TODO: Leer de memoria interna
    // 1. Verificar si las páginas necesarias están en memoria
    // 2. Si no están, cargarlas desde Storage (page fault)
    // 3. Leer el contenido de memoria
    // 4. Enviar el contenido al Master para que lo reenvíe al Query Control
    
    log_debug(logger_worker, "Ejecutando READ %s:%s %d %d", 
              file_name, tag_name, direccion_base, tamanio);
    
    // Simular lectura
    char contenido_leido[256] = "CONTENIDO_EJEMPLO";
    
    // Log obligatorio de lectura en memoria
    log_info(logger_worker, "Query %d: Acción: LEER - Dirección Física: %d - Valor: %s",
             id_query, direccion_base, contenido_leido);
    
    // TODO: Enviar contenido leído al Master
    
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_tag(char* file_origen, char* tag_origen, char* file_destino, char* tag_destino) {
    if (file_origen == NULL || tag_origen == NULL || 
        file_destino == NULL || tag_destino == NULL) {
        log_error(logger_worker, "TAG: Parámetros inválidos");
        return false;
    }
    
    // TODO: Enviar solicitud TAG al Storage
    log_debug(logger_worker, "Ejecutando TAG %s:%s %s:%s", 
              file_origen, tag_origen, file_destino, tag_destino);
    
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_commit(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "COMMIT: Parámetros inválidos");
        return false;
    }
    
    // TODO: Enviar solicitud COMMIT al Storage
    log_debug(logger_worker, "Ejecutando COMMIT %s:%s", file_name, tag_name);
    
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_flush(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "FLUSH: Parámetros inválidos");
        return false;
    }
    
    // TODO: Persistir todas las páginas modificadas del File:Tag en Storage
    // 1. Buscar todas las páginas del File:Tag en memoria
    // 2. Para cada página modificada, escribirla en Storage
    // 3. Marcar las páginas como no modificadas
    
    log_debug(logger_worker, "Ejecutando FLUSH %s:%s", file_name, tag_name);
    
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_delete(char* file_name, char* tag_name) {
    if (file_name == NULL || tag_name == NULL) {
        log_error(logger_worker, "DELETE: Parámetros inválidos");
        return false;
    }
    
    // TODO: Enviar solicitud DELETE al Storage
    log_debug(logger_worker, "Ejecutando DELETE %s:%s", file_name, tag_name);
    
    if (worker_configs->retardo_memoria > 0) {
        usleep(worker_configs->retardo_memoria * 1000);
    }
    
    return true;
}

bool execute_end() {
    log_info(logger_worker, "Ejecutando END - Finalizando query");
    return true;
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

bool split_file_tag(char* file_tag, char** file_name, char** tag_name) {
    if (file_tag == NULL) {
        return false;
    }
    
    // Buscar el separador ':'
    char* separador = strchr(file_tag, ':');
    if (separador == NULL) {
        return false;
    }
    
    // Calcular longitudes
    size_t len_file = separador - file_tag;
    size_t len_tag = strlen(separador + 1);
    
    // Asignar memoria y copiar
    *file_name = malloc(len_file + 1);
    *tag_name = malloc(len_tag + 1);
    
    if (*file_name == NULL || *tag_name == NULL) {
        if (*file_name) free(*file_name);
        if (*tag_name) free(*tag_name);
        return false;
    }
    
    strncpy(*file_name, file_tag, len_file);
    (*file_name)[len_file] = '\0';
    
    strcpy(*tag_name, separador + 1);
    
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
