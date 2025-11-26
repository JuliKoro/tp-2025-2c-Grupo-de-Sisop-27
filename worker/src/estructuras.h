#ifndef W_ESTRUCTURAS_H_
#define W_ESTRUCTURAS_H_

#include "worker.h"
#include <time.h>

// Definir constantes para algoritmos de reemplazo
#define LRU 0
#define CLOCK_M 1

// Estructura para una entrada de tabla de páginas.
// Necesitamos saber: si está presente en memo, marco asignado, flags
typedef struct {
    int presente;           // 1 si está en memoria, 0 si no
    int modificado;         // 1 si fue modificado
    int bit_uso;          // Bit de uso para CLOCK-M
    int marco;              // Número de marco físico asignado
    char* file_name;        // Nombre del file
    char* tag_name;         // Nombre del tag
    int numero_pagina;      // Número de página dentro del file:tag
    time_t ultimo_acceso;   // Para algoritmos de reemplazo
    //El tipo de dato time_t es un tipo de dato de la biblioteca ISO C definido para 
    //almacenar valores de tiempo del sistema. Estos valores se devuelven desde la función estándar de la biblioteca time()
} entrada_tabla_paginas;


typedef enum {
    ALGORITMO_NO_DEFINIDO, // Valor 0
    ALGORITMO_LRU,         // Valor 1
    ALGORITMO_CLOCK_M      // Valor 2
} t_algoritmo_reemplazo;


// Estructura para la tabla de páginas completa
typedef struct {
    entrada_tabla_paginas** entradas;
    int cantidad_entradas;
    t_algoritmo_reemplazo algoritmo_reemplazo; // LRU o CLOCK-M (usar las constantes de la config)
    int puntero_clock;      // Para CLOCK-M
} tabla_paginas;


// Estructura para la memoria interna
typedef struct {
    void* memoria;          // El malloc con la memoria
    int tam_memoria;        // TAM_MEMORIA del config
    int tam_pagina;         // Tamaño de página (hardcodeado por ahora)
    int cantidad_marcos;    // tam_memoria/tam_pagina
    int* marcos_libres;     // Array para tracking (rastreo) de marcos
    tabla_paginas* tabla;   // Tabla de páginas
} memoria_interna;

#endif