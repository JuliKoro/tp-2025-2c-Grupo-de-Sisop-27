#include "memoria_interna.h"

// Definición de la variable global de memoria (aquí vive realmente)
memoria_interna* memoria_worker = NULL;

// Referencias a variables externas definidas en worker.c
extern t_log* logger_worker;

memoria_interna* inicializar_memoria(int tam_memoria_config, int tam_pagina_hardcodeado) {
    memoria_interna* mem = malloc(sizeof(memoria_interna));
    if (!mem) {
        fprintf(stderr, "Error: No se pudo asignar memoria para memoria_interna\n");
        return NULL;
    }
    
    // Hardcodeo el tamaño de página por ahora
    mem->tam_pagina = tam_pagina_hardcodeado; // Ej: 128 bytes
    mem->tam_memoria = tam_memoria_config;
    mem->cantidad_marcos = tam_memoria_config / tam_pagina_hardcodeado;
    
    
    // Reservar la memoria principal
    mem->memoria = malloc(tam_memoria_config);
    if (!mem->memoria) {
        fprintf(stderr, "Error: No se pudo asignar memoria principal\n");
        free(mem);
        return NULL;
    }
    

    // Inicializar array de marcos libres
    mem->marcos_libres = malloc(mem->cantidad_marcos * sizeof(int));
        if (!mem->marcos_libres) {
        fprintf(stderr, "Error: No se pudo asignar memoria para marcos_libres\n");
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
        fprintf(stderr, "Error: No se pudo asignar memoria para tabla_paginas\n");
        free(mem->marcos_libres);
        free(mem->memoria);
        free(mem);
        return NULL;
    }
    
    mem->tabla->entradas = NULL;
    mem->tabla->cantidad_entradas = 0;
    mem->tabla->puntero_clock = 0; // Inicializar puntero para CLOCK-M
    //Aca se puede setear el algoritmo de reemplazo según la config si se quiere, pero lo dejo en NULL por ahora
    // if (strcmp(worker_conf->algoritmo_reemplazo, "LRU") == 0)
    mem->tabla->algoritmo_reemplazo = ALGORITMO_NO_DEFINIDO; 
    
    fprintf(stderr, "Memoria interna inicializada:\n");
    fprintf(stderr, " - Tamaño memoria: %d bytes\n", mem->tam_memoria);
    fprintf(stderr, " - Tamaño página: %d bytes\n", mem->tam_pagina);
    fprintf(stderr, " - Cantidad de marcos: %d\n", mem->cantidad_marcos);
    fprintf(stderr, " - Tabla de páginas creada (vacía)\n");

    return mem;
}

void destruir_memoria(memoria_interna* mem) {
    if (!mem) return;
    
    // Liberar todas las entradas de la tabla de páginas
    if (mem->tabla && mem->tabla->entradas) {
        for (int i = 0; i < mem->tabla->cantidad_entradas; i++) {
            if (mem->tabla->entradas[i]) {
                free(mem->tabla->entradas[i]->file_name);
                free(mem->tabla->entradas[i]->tag_name);
                free(mem->tabla->entradas[i]);
            }
        }
        free(mem->tabla->entradas);
    }
    
    // Liberar estructuras principales
    if (mem->tabla) free(mem->tabla);
    if (mem->marcos_libres) free(mem->marcos_libres);
    if (mem->memoria) free(mem->memoria);
    free(mem);
    
    fprintf(stderr, "Memoria interna liberada correctamente\n");
}

void mostrar_estado_memoria(memoria_interna* mem) {
    if (!mem) {
        fprintf(stderr, "Memoria no inicializada\n");
        return;
    }
    
    fprintf(stderr, "\n=== ESTADO DE LA MEMORIA INTERNA ===\n");
    fprintf(stderr, "Tamaño total: %d bytes\n", mem->tam_memoria);
    fprintf(stderr, "Tamaño página: %d bytes\n", mem->tam_pagina);
    fprintf(stderr, "Marcos totales: %d\n", mem->cantidad_marcos);
    
    // Mostrar marcos libres
    fprintf(stderr, "Marcos libres: ");
    for (int i = 0; i < mem->cantidad_marcos; i++) {
        fprintf(stderr, "%d ", mem->marcos_libres[i]);
    }
    fprintf(stderr, "\n");
    
    // Mostrar tabla de páginas
    fprintf(stderr, "Entradas en tabla de páginas: %d\n", mem->tabla->cantidad_entradas);
    for (int i = 0; i < mem->tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = mem->tabla->entradas[i];
        fprintf(stderr, "  [%d] File: %s, Tag: %s, Página: %d, Marco: %d, Presente: %d, Modificado: %d\n",
                i, entrada->file_name, entrada->tag_name, entrada->numero_pagina,
                entrada->marco, entrada->presente, entrada->modificado);
    }
    fprintf(stderr, "====================================\n\n");
}

//BUSCAR PAGINA:
// Actualiza los bits de uso y acceso en cada hit
entrada_tabla_paginas* buscar_pagina(tabla_paginas* tabla, const char* file, const char* tag, int num_pagina) {
    for (int i = 0; i < tabla->cantidad_entradas; i++) {
        entrada_tabla_paginas* entrada = tabla->entradas[i];
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

//AGREGAR ENTRADA A TABLA DE PAGINAS:
void agregar_entrada_tabla(tabla_paginas* tabla, const char* file, const char* tag, int num_pagina, int marco) {
    
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
    
    // Log obligatorio
    log_info(logger_worker, "## Se asigna el Marco: %d a la Página: %d perteneciente al - File: %s - Tag: %s", 
             marco, num_pagina, file, tag);
}

//------------ALGORITMOS DE REEMPLAZO------------
/**
 * @brief Encuentra la víctima según el algoritmo LRU.
 * Recorre TODAS las entradas presentes en memoria y devuelve el
 * índice de la que tiene el 'ultimo_acceso' más antiguo.
 */
int encontrar_victima_lru(tabla_paginas* tabla) {
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

/**
 * @brief Encuentra la víctima según el algoritmo CLOCK-M 
 * Implementa la búsqueda en dos pasadas.
 */
int encontrar_victima_clock_m(tabla_paginas* tabla) {
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

/**
 * @brief Función "despachadora" que llama al algoritmo de reemplazo
 * configurado, desaloja la página y devuelve el marco liberado.
 */
int ejecutar_algoritmo_reemplazo(memoria_interna* mem, const char* file_nuevo, const char* tag_nuevo, int pag_nueva) {
    
    int indice_victima = -1;
    
    // Acá usamos el config para decidir qué algoritmo usar
    // Usamos worker_configs que trajimos con extern
    if (strcmp(worker_configs->algoritmo_reemplazo, "LRU") == 0) {
        indice_victima = encontrar_victima_lru(mem->tabla);
    } else if (strcmp(worker_configs->algoritmo_reemplazo, "CLOCK-M") == 0) {
        indice_victima = encontrar_victima_clock_m(mem->tabla);
    } else {
        // Fallback por si el config está mal (o FIFO si lo implementaran)
        log_error(logger_worker, "Algoritmo de reemplazo no reconocido. Usando LRU.");
        indice_victima = encontrar_victima_lru(mem->tabla);
    }

    if (indice_victima == -1) {
        log_error(logger_worker, "Error: No se pudo encontrar una víctima para reemplazo.");
        return -1; // Error grave
    }

    entrada_tabla_paginas* victima = mem->tabla->entradas[indice_victima];

    // Log obligatorio de reemplazo
    log_info(logger_worker, "## Query <ID>: Se reemplaza la página <%s:%s>/<%d> por la <%s:%s>/<%d>",
             victima->file_name, victima->tag_name, victima->numero_pagina,
             file_nuevo, tag_nuevo, pag_nueva);

    // Si la víctima está modificada, hay que guardarla en Storage
    if (victima->modificado) {
        log_info(logger_worker, "Página víctima (%s:%s Pag %d) modificada. Iniciando FLUSH.",
                 victima->file_name, victima->tag_name, victima->numero_pagina);
        
        // TODO: IMPLEMENTAR LÓGICA DE FLUSH 
        // 1. Calcular dirección física: (mem->memoria + (victima->marco * mem->tam_pagina))
        // 2. Enviar ese bloque de memoria al Storage (junto con file, tag y num_pagina/bloque)
        // 3. Esperar OK de Storage

    }

    // "Desalojar" la página (sin borrar la entrada, solo marcarla como ausente)
    int marco_liberado = victima->marco;
    victima->presente = 0;
    victima->modificado = 0; // Ya no está modificada (se flusheó o se descartó)
    victima->bit_uso = 0;
    victima->marco = -1; // No tiene marco asignado

    // Log de liberación 
    log_info(logger_worker, "## Se libera el Marco: %d perteneciente al - File: %s - Tag: %s", 
             marco_liberado, victima->file_name, victima->tag_name);

    return marco_liberado;
}