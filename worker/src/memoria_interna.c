#include "memoria_interna.h"

// Definición de la variable global de memoria (aquí vive realmente)
memoria_interna* memoria_worker = NULL;

memoria_interna* inicializar_memoria() {
    memoria_interna* mem = malloc(sizeof(memoria_interna));
    if (!mem) {
        log_error(logger_worker, "Error: No se pudo asignar memoria para memoria_interna");
        return NULL;
    }
    
    mem->tam_memoria = worker_configs->tam_memoria;
    mem->tam_pagina = worker_configs->tam_pagina; // tamaño de página del handshake
    
    // Validación división por cero
    if (mem->tam_pagina <= 0) {
        log_error(logger_worker, "Error: Tamaño de página inválido (%d)", mem->tam_pagina);
        free(mem);
        return NULL;
    }

    mem->cantidad_marcos = worker_configs->tam_memoria / worker_configs->tam_pagina;
    
    // Reservar la memoria principal
    mem->memoria = malloc(mem->tam_memoria);
    if (!mem->memoria) {
        log_error(logger_worker, "Error: No se pudo asignar memoria principal");
        free(mem);
        return NULL;
    }

    // Inicializar array de marcos libres (1 = libre, 0 = ocupado)
    mem->marcos_libres = malloc(mem->cantidad_marcos * sizeof(int));
    
    if (!mem->marcos_libres) {
        log_error(logger_worker, "Error: No se pudo asignar memoria para marcos_libres");
        free(mem->memoria);
        free(mem);
        return NULL;
    }   

    // Inicializar todos los marcos como libres
    for (int i = 0; i < mem->cantidad_marcos; i++) {
        mem->marcos_libres[i] = 1; // 1 = libre, 0 = ocupado
    }

    // Inicializar tabla de páginas vacía
    mem->tabla = malloc(sizeof(tabla_paginas));
    if (!mem->tabla) {
        log_error(logger_worker, "Error: No se pudo asignar memoria para tabla_paginas");
        free(mem->marcos_libres);
        free(mem->memoria);
        free(mem);
        return NULL;
    }
    
    mem->tabla->entradas = NULL;
    mem->tabla->cantidad_entradas = 0;
    mem->tabla->puntero_clock = 0; // Inicializar puntero para CLOCK-M
    
    // --- SELECCIÓN DEL ALGORITMO DE REEMPLAZO ---
    if (strcmp(worker_configs->algoritmo_reemplazo, "LRU") == 0) {
        mem->tabla->algoritmo_reemplazo = ALGORITMO_LRU;
    } else if (strcmp(worker_configs->algoritmo_reemplazo, "CLOCK-M") == 0) {
        mem->tabla->algoritmo_reemplazo = ALGORITMO_CLOCK_M;
    } 

    fprintf(stderr, "Memoria interna inicializada:\n");
    fprintf(stderr, " - Tamaño memoria: %d bytes\n", mem->tam_memoria);
    fprintf(stderr, " - Tamaño página: %d bytes\n", mem->tam_pagina);
    fprintf(stderr, " - Cantidad de marcos: %d\n", mem->cantidad_marcos);
    fprintf(stderr, " - Tabla de páginas creada (vacía)\n");

    log_debug(logger_worker, "Memoria interna inicializada correctamente");

    return mem;
}

void destruir_memoria(void) {
    if (!memoria_worker) return;
    
    // Liberar todas las entradas de la tabla de páginas
    if (memoria_worker->tabla && memoria_worker->tabla->entradas) {
        for (int i = 0; i < memoria_worker->tabla->cantidad_entradas; i++) {
            if (memoria_worker->tabla->entradas[i]) {
                free(memoria_worker->tabla->entradas[i]->file_name);
                free(memoria_worker->tabla->entradas[i]->tag_name);
                free(memoria_worker->tabla->entradas[i]);
            }
        }
        free(memoria_worker->tabla->entradas);
    }
    
    // Liberar estructuras principales
    if (memoria_worker->tabla) free(memoria_worker->tabla);
    if (memoria_worker->marcos_libres) free(memoria_worker->marcos_libres);
    if (memoria_worker->memoria) free(memoria_worker->memoria);
    free(memoria_worker);
    memoria_worker = NULL;
    
    log_debug(logger_worker, "Memoria interna liberada correctamente");
}

void mostrar_estado_memoria(void) {
    if (!memoria_worker) {
        fprintf(stderr, "Memoria no inicializada\n");
        return;
    }
    
    fprintf(stderr, "\n=== ESTADO DE LA MEMORIA INTERNA ===\n");
    fprintf(stderr, "Tamaño total: %d bytes\n", memoria_worker->tam_memoria);
    fprintf(stderr, "Tamaño página: %d bytes\n", memoria_worker->tam_pagina);
    fprintf(stderr, "Marcos totales: %d\n", memoria_worker->cantidad_marcos);
    
    // Mostrar marcos libres
    fprintf(stderr, "Marcos libres: ");
    for (int i = 0; i < memoria_worker->cantidad_marcos; i++) {
        fprintf(stderr, "%d ", memoria_worker->marcos_libres[i]);
    }
    fprintf(stderr, "\n");
    
    // Mostrar tabla de páginas
    fprintf(stderr, "Entradas en tabla de páginas: %d\n", memoria_worker->tabla->cantidad_entradas);
    for (int i = 0; i < memoria_worker->tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = memoria_worker->tabla->entradas[i];
        fprintf(stderr, "  [%d] File: %s, Tag: %s, Página: %d, Marco: %d, Presente: %d, Modificado: %d\n",
                i, entrada->file_name, entrada->tag_name, entrada->numero_pagina,
                entrada->marco, entrada->presente, entrada->modificado);
    }
    fprintf(stderr, "====================================\n\n");
}

//BUSCAR PAGINA

entrada_tabla_paginas* buscar_pagina(const char* file, const char* tag, int num_pagina) {
    for (int i = 0; i < memoria_worker->tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = memoria_worker->tabla->entradas[i];
        if (strcmp(entrada->file_name, file) == 0 &&
            strcmp(entrada->tag_name, tag) == 0 &&
            entrada->numero_pagina == num_pagina) {
            
            // Si la página está en memoria, actualizamos sus bits
            // Esto es importante para que LRU y CLOCK funquen
            if (entrada->presente) {
                entrada->bit_uso = 1;
                entrada->ultimo_acceso = time(NULL);
            }

            return entrada; // Devuelve la entrada (presente o no)
        }
    }
    return NULL; // No se encontró, es la primera vez que se accede
}

//AGREGAR ENTRADA A TABLA DE PAGINAS

void agregar_entrada_tabla(const char* file, const char* tag, int num_pagina, int marco) {
    tabla_paginas* tabla = memoria_worker->tabla;

    // Agrandamos el array de punteros para hacer espacio para el nuevo
    tabla->entradas = realloc(tabla->entradas, (tabla->cantidad_entradas + 1) * sizeof(entrada_tabla_paginas*));
    if (!tabla->entradas) {
        log_error(logger_worker, "Fallo crítico al hacer realloc para la tabla de páginas");
        // Aca se podría manejar el error de forma mas robusta, tal vez terminando la query
        return;
    }

    // Crear nueva entrada
    entrada_tabla_paginas* nueva = malloc(sizeof(entrada_tabla_paginas));
    nueva->file_name = strdup(file);
    nueva->tag_name = strdup(tag);
    nueva->numero_pagina = num_pagina;
    nueva->marco = marco;
    nueva->presente = 1;
    nueva->modificado = 0;
    nueva->bit_uso = 1; // Se considera usada al cargarla
    nueva->ultimo_acceso = time(NULL); // Se setea el tiempo

    tabla->entradas[tabla->cantidad_entradas] = nueva;
    tabla->cantidad_entradas++;

    // Log obligatorio - Asignar Marco
    log_info(logger_worker, "## Query %d: Se asigna el Marco: %d a la Página: %d perteneciente al - File: %s - Tag: %s", 
            id_query, marco, num_pagina, file, tag);
}

//------------ALGORITMOS DE REEMPLAZO------------

int encontrar_victima_lru(void) {
    tabla_paginas* tabla = memoria_worker->tabla;
    int indice_victima = -1;
    time_t min_tiempo = time(NULL) + 1; // Un tiempo futuro

    for (int i = 0; i < tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = tabla->entradas[i];

        // Solo consideramos páginas que estén en memoria
        if (entrada->presente) {
            if (entrada->ultimo_acceso < min_tiempo) {
                min_tiempo = entrada->ultimo_acceso;
                indice_victima = i;
            }
        }
    }
    return indice_victima;
}

int encontrar_victima_clock_m(void) {
    tabla_paginas* tabla = memoria_worker->tabla;
    int N = tabla->cantidad_entradas;
    if (N == 0) return -1; // No hay páginas

    // PRIMERA PASADA: Buscar (use=0, modificado=0) 
    // Damos hasta N vueltas para revisar todas las entradas
    for (int i = 0; i < N; i++) {
        int indice_actual = tabla->puntero_clock;
        entrada_tabla_paginas* entrada = tabla->entradas[indice_actual];

        // Avanzar el puntero para la próxima vez
        tabla->puntero_clock = (indice_actual + 1) % N;

        // Solo consideramos páginas que estén en memoria
        if (!entrada->presente) {
            continue;
        }

        if (entrada->bit_uso == 0 && entrada->modificado == 0) {
            // ¡Víctima encontrada en (0,0)!
            return indice_actual;
        }

        // Si no es (0,0), damos segunda oportunidad si use=1
        if (entrada->bit_uso == 1) {
            entrada->bit_uso = 0;
        }
    }

    // SEGUNDA PASADA: Buscar (use=0, modificado=1) 
    // Si llegamos aca, dimos una vuelta completa y todos los (1,X) son ahora (0,X)
    // Volvemos a recorrer
    for (int i = 0; i < N; i++) {
        int indice_actual = tabla->puntero_clock;
        entrada_tabla_paginas* entrada = tabla->entradas[indice_actual];

        // Avanzar el puntero
        tabla->puntero_clock = (indice_actual + 1) % N;

        if (!entrada->presente) {
            continue;
        }

        if (entrada->bit_uso == 0 && entrada->modificado == 0) {
            // Encontramos un (0,0) que antes era (1,0)
            return indice_actual;
        }

        if (entrada->bit_uso == 0 && entrada->modificado == 1) {
            // Víctima encontrada en (0,1)
            return indice_actual;
        }

        // Si era (1,1), en la pasada anterior se volvió (0,1)
        // así que la condición anterior lo va a atrapar
        // Si era (1,0), se volvió (0,0) y la primera condición lo atrapa
    }
    
    // Esto teóricamente no debería pasar si hay páginas presentes, pero por las dudas, devolvemos la posición actual
    return tabla->puntero_clock;
}

t_resultado_ejecucion ejecutar_algoritmo_reemplazo(const char* file_nuevo, const char* tag_nuevo, int pag_nueva, int* marco_asignado) {

    int indice_victima = -1;

    // Acá usamos el config para decidir qué algoritmo usar
    if (strcmp(worker_configs->algoritmo_reemplazo, "LRU") == 0) {
        indice_victima = encontrar_victima_lru();
    } else if (strcmp(worker_configs->algoritmo_reemplazo, "CLOCK-M") == 0) {
        indice_victima = encontrar_victima_clock_m();
    } else {
        // Fallback por si el config está mal (o FIFO si lo implementaran)
        log_error(logger_worker, "Algoritmo de reemplazo no reconocido. Usando LRU.");
        indice_victima = encontrar_victima_lru();
    }

    if (indice_victima == -1) {
        log_error(logger_worker, "Error: No se pudo encontrar una víctima para reemplazo.");
        return ERROR_MEMORIA_INTERNA; // Error grave
    }

    entrada_tabla_paginas* victima = memoria_worker->tabla->entradas[indice_victima];

    // Log Obligatorio - Reemplazo Algoritmo
    log_info(logger_worker, "## Query %d: Se reemplaza la página <%s:%s>/<%d> por la <%s:%s>/<%d>",
             id_query, victima->file_name, victima->tag_name, victima->numero_pagina,
             file_nuevo, tag_nuevo, pag_nueva);

    // Si la víctima está modificada, hay que guardarla en Storage
    if (victima->modificado) {
        log_info(logger_worker, "Página víctima (%s:%s Pag %d) modificada. Iniciando FLUSH.",
                 victima->file_name, victima->tag_name, victima->numero_pagina);

        t_resultado_ejecucion resultado_flush = flush_pagina(victima->file_name, victima->tag_name, victima->numero_pagina);

        if (resultado_flush != EXEC_OK) {
            log_error(logger_worker, "Error crítico: Falló el flush de la página víctima. Se perderán datos.");
            return resultado_flush; // Propagamos el error y abortamos el reemplazo
        }
    }

    // "Desalojar" la página (sin borrar la entrada, solo marcarla como ausente)
    int marco_liberado = victima->marco;
    victima->presente = 0;
    victima->modificado = 0; // Ya no está modificada (se flusheó o se descartó)
    victima->bit_uso = 0;
    victima->marco = -1; // No tiene marco asignado

    // Log Obligatorio - Liberación de Marco
    log_info(logger_worker, "## Query %d: Se libera el Marco: %d perteneciente al - File: %s - Tag: %s", 
             id_query, marco_liberado, victima->file_name, victima->tag_name);

    if (marco_asignado != NULL) {
        *marco_asignado = marco_liberado;
    }
    return EXEC_OK;
}

// ============================================================================
// FUNCIONES DE TRADUCCIÓN Y ACCESO A MEMORIA
// ============================================================================

int buscar_marco_libre(void) {
    if (!memoria_worker) return -1;
    
    for (int i = 0; i < memoria_worker->cantidad_marcos; i++) {
        if (memoria_worker->marcos_libres[i] == 1) {
            return i;
        }
    }
    return -1; // No hay marcos libres
}

uint32_t numero_pagina(uint32_t dir_logica) {
    return dir_logica / memoria_worker->tam_pagina;
}

uint32_t offset_pagina(uint32_t dir_logica) {
    return dir_logica % memoria_worker->tam_pagina;
}


bool traducir_direccion(const char* file, const char* tag, uint32_t dir_logica, uint32_t* dir_fisica) {
    if (!memoria_worker || !file || !tag || !dir_fisica) {
        return false;
    }
    
    // Calcular número de página y desplazamiento
    uint32_t num_pagina = numero_pagina(dir_logica);
    uint32_t offset = offset_pagina(dir_logica);
    
    // Buscar la página en la tabla
    entrada_tabla_paginas* entrada = buscar_pagina(file, tag, num_pagina);
    
    // Si no existe la entrada, es la primera vez que se accede
    if (entrada == NULL) {
        return false; // PAGE FAULT
    }
    
    // Si existe pero no está presente en memoria
    if (!entrada->presente) {
        return false; // PAGE FAULT
    }
    
    // HIT: La página está en memoria
    // Calcular dirección física
    *dir_fisica = (entrada->marco * memoria_worker->tam_pagina) + offset;
    
    return true;
}

t_resultado_ejecucion manejar_page_fault(const char* file, const char* tag, uint32_t num_pagina, int* marco) {
    if (!memoria_worker || !file || !tag) {
        return ERROR_MEMORIA_INTERNA;
    }
    
    // Log de PAGE FAULT
    log_warning(logger_worker, "## Query %d: PAGE FAULT - File: %s - Tag: %s - Página: %d", 
             id_query, file, tag, num_pagina);

    // Log Obligatorio - Bloque faltante en Memoria
    log_info(logger_worker, "## Query %d: - Memoria Miss - File: %s - Tag: %s - Pagina: %d", id_query, file, tag, num_pagina);
    
    // Variable local para manejar el número de marco
    int marco_asignado = buscar_marco_libre();
    
    // Buscar marco libre
    // Si no hay marco libre, ejecutar algoritmo de reemplazo
    if (marco_asignado == -1) {
        log_warning(logger_worker, "No hay marcos libres. Ejecutando algoritmo de reemplazo.");
        t_resultado_ejecucion res_reemplazo = ejecutar_algoritmo_reemplazo(file, tag, num_pagina, &marco_asignado);
        
        if (res_reemplazo != EXEC_OK) {
            log_error(logger_worker, "Error al ejecutar algoritmo de reemplazo: %d", res_reemplazo);
            return res_reemplazo;
        }
    }

    // Log obligatorio - Asignar Marco
    log_info(logger_worker, "## Query %d: Se asigna el Marco: %d a la Página: %d perteneciente al - File: %s - Tag: %s", 
             id_query, marco_asignado, num_pagina, file, tag);
    
    // Solicitar bloque a Storage
    if (!solicitar_bloque_storage(file, tag, num_pagina, memoria_worker->tam_pagina)) {
        log_error(logger_worker, "Error al solicitar bloque %d de %s:%s a Storage", num_pagina, file, tag);
        return ERROR_CONEXION;
    }

    void* bloque = malloc(memoria_worker->tam_pagina);

    t_resultado_ejecucion resultado = recibir_bloque_storage(bloque);

    if (resultado != EXEC_OK) {
        log_error(logger_worker, "Error al recibir bloque %d de %s:%s desde Storage", num_pagina, file, tag);
        free(bloque);
        return resultado; // Propagar el error
    }
    
    // Copiamos el contenido recibido al marco de memoria correspondiente
    void* dir_marco = memoria_worker->memoria + (marco_asignado * memoria_worker->tam_pagina);
    memcpy(dir_marco, bloque, memoria_worker->tam_pagina);
    free(bloque); // Liberamos el buffer temporal
    
    // Verificar si ya existe una entrada para esta página (desalojada previamente)
    entrada_tabla_paginas* entrada_existente = buscar_pagina(file, tag, num_pagina);
    
    if (entrada_existente != NULL) {
        // La entrada ya existe (fue desalojada antes), solo actualizarla
        entrada_existente->marco = marco_asignado;
        entrada_existente->presente = 1;
        entrada_existente->modificado = 0;
        entrada_existente->bit_uso = 1;
        entrada_existente->ultimo_acceso = time(NULL);
        
    } else {
        // Es una página nueva, agregar entrada a la tabla
        agregar_entrada_tabla(file, tag, num_pagina, marco_asignado);
    }
    
    // Marcar marco como ocupado
    memoria_worker->marcos_libres[marco_asignado] = 0;
    
    // Asignamos el valor al puntero de salida
    if (marco != NULL) *marco = marco_asignado;

    // Log Obligatorio - Bloque ingresado en Memoria
    log_info(logger_worker, "## Query %d: - Memoria Add - File: %s - Tag: %s - Pagina: %d - Marco: %d", 
             id_query, file, tag, num_pagina, marco_asignado);
    
    return EXEC_OK;
}

bool solicitar_bloque_storage(const char* file, const char* tag, uint32_t num_pagina, uint32_t tamanio) {
    t_sol_read* solicitud = malloc(sizeof(t_sol_read));
    solicitud->file_name = strdup(file);
    solicitud->tag_name = strdup(tag);
    solicitud->numero_bloque = num_pagina;
    solicitud->tamanio = tamanio;

    t_buffer* buffer_solicitud = serializar_solicitud_read(solicitud);

    t_paquete* paquete = empaquetar_buffer(OP_READ, buffer_solicitud);

    if (enviar_paquete(conexion_storage, paquete) == -1) {
        log_error(logger_worker, "Error al enviar solicitud de lectura a Storage");
        free(solicitud->file_name);
        free(solicitud->tag_name);
        free(solicitud);
        return false;
    }

    free(solicitud->file_name);
    free(solicitud->tag_name);
    free(solicitud);

    log_debug(logger_worker, "Solicitando bloque %d de %s:%s a Storage", num_pagina, file, tag);
    return true;
}

t_resultado_ejecucion recibir_bloque_storage(void* bloque) {
    t_paquete* paquete_recibido = recibir_paquete(conexion_storage);
    if (!paquete_recibido) {
        log_error(logger_worker, "Error al recibir paquete de Storage");
        return ERROR_CONEXION;
    }

    if (paquete_recibido->codigo_operacion == OP_ERROR) {
        log_error(logger_worker, "Storage respondió con ERROR al solicitar bloque");
        t_resultado_ejecucion* cod_error = deserializar_cod_error(paquete_recibido->datos);
        destruir_paquete(paquete_recibido);
        return *cod_error;
    }

    if (paquete_recibido->codigo_operacion != OP_READ) {
        log_error(logger_worker, "Código de operación inesperado al recibir bloque de Storage");
        // Liberar paquete recibido
        destruir_paquete(paquete_recibido);
        return ERROR_CONEXION;
    }

    t_bloque_leido* bloque_leido = deserializar_bloque_leido(paquete_recibido->datos);

    // Chequear si el tamaño recibido es correcto y no excede el tamaño esperado
    if (bloque_leido->tamanio > worker_configs->tam_pagina) {
        log_error(logger_worker, "Tamaño de bloque recibido excede el tamaño de página esperado");
        // Liberar recursos
        free(bloque_leido->file_name);
        free(bloque_leido->tag_name);
        free(bloque_leido->contenido);
        free(bloque_leido);
        destruir_paquete(paquete_recibido);
        return ERROR_TAMANIO_INVALIDO;
    }
    
    memcpy(bloque, bloque_leido->contenido, bloque_leido->tamanio); // Copiar contenido al bloque proporcionado

    free(bloque_leido->file_name);
    free(bloque_leido->tag_name);
    free(bloque_leido->contenido);
    free(bloque_leido);
    destruir_paquete(paquete_recibido);
    return EXEC_OK;
}

t_resultado_ejecucion flush_pagina(const char* file, const char* tag, uint32_t num_pagina) {
    entrada_tabla_paginas* entrada = buscar_pagina(file, tag, num_pagina);
    if (!entrada || !entrada->presente) {
        log_error(logger_worker, "No se puede hacer flush: página no está en memoria");
        return ERROR_MEMORIA_INTERNA;
    }

    // Calcular dirección física
    uint32_t dir_fisica = (entrada->marco * memoria_worker->tam_pagina);

    // Preparar bloque para enviar a Storage
    t_sol_write* bloque = malloc(sizeof(t_sol_write));
    bloque->id_query = id_query;
    bloque->file_name = strdup(file);
    bloque->tag_name = strdup(tag);
    bloque->numero_bloque = num_pagina;
    bloque->tamanio = memoria_worker->tam_pagina;
    bloque->contenido = malloc(bloque->tamanio);
    
    // Copiar contenido de memoria real al buffer
    memcpy(bloque->contenido, memoria_worker->memoria + dir_fisica, bloque->tamanio);

    // Serializar y enviar a Storage
    t_buffer* buffer_flush = serializar_solicitud_write(bloque);
    t_paquete* paquete_flush = empaquetar_buffer(OP_WRITE, buffer_flush);

    if (enviar_paquete(conexion_storage, paquete_flush) == -1) {
        log_error(logger_worker, "Error al enviar flush a Storage");
        liberar_bloque(bloque);
        return ERROR_CONEXION;
    }

    // Liberar recursos
    liberar_bloque(bloque);

    t_resultado_ejecucion respuesta_storage = recibir_respuesta_storage();
    if (respuesta_storage != EXEC_OK) {
        log_error(logger_worker, "Error al recibir respuesta de flush desde Storage");
        return respuesta_storage;
    }
    
    log_info(logger_worker, "Flush completado para %s:%s Página %d", file, tag, num_pagina);
    return EXEC_OK;
}

void liberar_bloque(t_sol_write* bloque) {
    if (!bloque) return;
    if (bloque->file_name) free(bloque->file_name);
    if (bloque->tag_name) free(bloque->tag_name);
    if (bloque->contenido) free(bloque->contenido);
    free(bloque);
}

// OPERACIONES DE MEMORIA INTERNA/STORAGE

t_resultado_ejecucion leer_memoria(const char* file, const char* tag, uint32_t dir_logica, uint32_t tamanio, void* buffer) {
    if (!memoria_worker || !file || !tag || !buffer || tamanio == 0) {
        return ERROR_MEMORIA_INTERNA;
    }
    
    log_debug(logger_worker, "Leyendo %d bytes desde dirección lógica %d de %s:%s", 
              tamanio, dir_logica, file, tag);
    
    uint32_t bytes_leidos = 0;
    uint32_t dir_actual = dir_logica;
    
    while (bytes_leidos < tamanio) {
        // Calcular página actual
        uint32_t num_pagina = numero_pagina(dir_actual);
        uint32_t offset = offset_pagina(dir_actual);
        
        // Calcular cuántos bytes leer de esta página
        uint32_t bytes_en_pagina = memoria_worker->tam_pagina - offset;
        uint32_t bytes_a_leer = (tamanio - bytes_leidos < bytes_en_pagina) ? 
                                 (tamanio - bytes_leidos) : bytes_en_pagina;
        
        // Traducir dirección
        uint32_t dir_fisica;
        if (!traducir_direccion(file, tag, dir_actual, &dir_fisica)) {
            // PAGE FAULT
            log_debug(logger_worker, "Page fault al leer página %d", num_pagina);
            int marco;
            t_resultado_ejecucion resultado = manejar_page_fault(file, tag, num_pagina, &marco);
            if (resultado != EXEC_OK) {
                log_error(logger_worker, "Error al manejar page fault");
                return resultado;
            }

            if (marco < 0) {
                log_error(logger_worker, "Error al manejar page fault");
                return ERROR_MEMORIA_INTERNA;
            }
            
            // Reintentar traducción
            if (!traducir_direccion(file, tag, dir_actual, &dir_fisica)) {
                log_error(logger_worker, "Error al traducir dirección después de page fault");
                return ERROR_MEMORIA_INTERNA;
            }
        }
        
        // Copiar bytes de memoria al buffer
        void* origen = memoria_worker->memoria + dir_fisica;
        memcpy((char*)buffer + bytes_leidos, origen, bytes_a_leer);
        
        // Log obligatorio de lectura (por cada acceso a marco físico)
        char contenido_str[256];
        snprintf(contenido_str, sizeof(contenido_str), "%.*s", (int)bytes_a_leer, (char*)origen);

        // Log Obligatorio - Lectura Memoria
        log_info(logger_worker, "## Query %d: Acción: LEER - Dirección Física: %d - Valor: %s",
                    id_query, dir_fisica, contenido_str);
        
        // Aplicar retardo de memoria
        if (worker_configs->retardo_memoria > 0) {
            usleep(worker_configs->retardo_memoria * 1000);
        }
        
        bytes_leidos += bytes_a_leer;
        dir_actual += bytes_a_leer;
    }
    
    log_debug(logger_worker, "Lectura completada: %d bytes", bytes_leidos);
    return EXEC_OK;
}


t_resultado_ejecucion escribir_memoria(const char* file, const char* tag, uint32_t dir_logica, const void* contenido, uint32_t tamanio) {
    if (!memoria_worker || !file || !tag || !contenido || tamanio == 0) {
        return ERROR_MEMORIA_INTERNA;
    }
    
    log_debug(logger_worker, "Escribiendo %d bytes en dirección lógica %d de %s:%s", 
              tamanio, dir_logica, file, tag);
    
    uint32_t bytes_escritos = 0;
    uint32_t dir_actual = dir_logica;
    
    while (bytes_escritos < tamanio) {
        // Calcular página actual
        uint32_t num_pagina = numero_pagina(dir_actual);
        uint32_t offset = offset_pagina(dir_actual);
        
        // Calcular cuántos bytes escribir en esta página
        uint32_t bytes_en_pagina = memoria_worker->tam_pagina - offset;
        uint32_t bytes_a_escribir = (tamanio - bytes_escritos < bytes_en_pagina) ? 
                                     (tamanio - bytes_escritos) : bytes_en_pagina;
        
        // Traducir dirección
        uint32_t dir_fisica;
        if (!traducir_direccion(file, tag, dir_actual, &dir_fisica)) {
            // PAGE FAULT
            log_warning(logger_worker, "Page fault al escribir en página %d", num_pagina);
            int marco;
            t_resultado_ejecucion resultado = manejar_page_fault(file, tag, num_pagina, &marco);
            if (resultado != EXEC_OK) {
                log_error(logger_worker, "Error al manejar page fault");
                return resultado;
            }
            
            // Reintentar traducción
            if (!traducir_direccion(file, tag, dir_actual, &dir_fisica)) {
                log_error(logger_worker, "Error al traducir dirección después de page fault");
                return ERROR_MEMORIA_INTERNA;
            }
        }
        
        // Copiar bytes del contenido a memoria
        void* destino = memoria_worker->memoria + dir_fisica;
        memcpy(destino, (char*)contenido + bytes_escritos, bytes_a_escribir);
        
        // Marcar página como modificada
        entrada_tabla_paginas* entrada = buscar_pagina(file, tag, num_pagina);
        if (entrada && entrada->presente) {
            entrada->modificado = 1;
        }
        
        // Log obligatorio de escritura (por cada acceso a marco físico)
        char contenido_str[256];
        snprintf(contenido_str, sizeof(contenido_str), "%.*s", (int)bytes_a_escribir, (char*)contenido);
        
        // Log Obligatorio - Escritura Memoria
        log_info(logger_worker, "## Query %d: Acción: ESCRIBIR - Dirección Física: %d - Valor: %s",
                    id_query, dir_fisica, contenido_str);
        
        // Aplicar retardo de memoria
        if (worker_configs->retardo_memoria > 0) {
            usleep(worker_configs->retardo_memoria * 1000);
        }
        
        bytes_escritos += bytes_a_escribir;
        dir_actual += bytes_a_escribir;
    }
    
    log_debug(logger_worker, "Escritura completada: %d bytes", bytes_escritos);
    return EXEC_OK;
}

t_resultado_ejecucion flush_file_tag(const char* file, const char* tag) {
    if (!memoria_worker || !file || !tag) {
        return ERROR_MEMORIA_INTERNA;
    }
    
    log_debug(logger_worker, "Iniciando FLUSH de %s:%s", file, tag);
    
    int paginas_flusheadas = 0;
    
    // Recorrer todas las entradas de la tabla
    for (int i = 0; i < memoria_worker->tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = memoria_worker->tabla->entradas[i];
        
        // Filtrar por file:tag
        if (strcmp(entrada->file_name, file) != 0 ||
            strcmp(entrada->tag_name, tag) != 0) {
            continue;
        }
        
        // Solo persistir si está presente Y modificada
        if (entrada->presente && entrada->modificado) {
            t_resultado_ejecucion res = flush_pagina(file, tag, entrada->numero_pagina);
            if (res != EXEC_OK) {
                log_error(logger_worker, "Error al hacer flush de página %d de %s:%s", entrada->numero_pagina, file, tag);
                return res;
            }
            
            // Marcar como no modificada
            entrada->modificado = 0;
            paginas_flusheadas++;
            
            // Aplicar retardo de memoria
            if (worker_configs->retardo_memoria > 0) {
                usleep(worker_configs->retardo_memoria * 1000);
            }
        }
    }
    
    log_debug(logger_worker, "FLUSH completado: %d páginas persistidas", paginas_flusheadas);
    return EXEC_OK;
}

t_resultado_ejecucion flush_all(void) {
    if (!memoria_worker) {
        return ERROR_MEMORIA_INTERNA;
    }
    
    log_debug(logger_worker, "Iniciando FLUSH de toda la memoria");
    
    int paginas_flusheadas = 0;
    
    // Recorrer todas las entradas de la tabla
    for (int i = 0; i < memoria_worker->tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = memoria_worker->tabla->entradas[i];
        
        // Solo persistir si está presente Y modificada
        if (entrada->presente && entrada->modificado) {
            t_resultado_ejecucion res = flush_pagina(entrada->file_name, entrada->tag_name, entrada->numero_pagina);
            if (res != EXEC_OK) {
                log_error(logger_worker, "Error al hacer flush de página %d de %s:%s", entrada->numero_pagina, entrada->file_name, entrada->tag_name);
                return res;
            }
            
            // Marcar como no modificada
            entrada->modificado = 0;
            paginas_flusheadas++;
        }
    }
    
    log_debug(logger_worker, "FLUSH completado: %d páginas persistidas", paginas_flusheadas);
    return EXEC_OK;
}