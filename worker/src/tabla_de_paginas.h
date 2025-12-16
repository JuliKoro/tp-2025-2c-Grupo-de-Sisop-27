#ifndef TABLA_DE_PAGINAS_H_
#define TABLA_DE_PAGINAS_H_

#include "worker.h"
#include <time.h>

// Definir constantes para algoritmos de reemplazo
#define LRU 0
#define CLOCK_M 1

// Necesitamos saber: si está presente en memo, marco asignado, flags
/**
 * @brief Estructura que representa una entrada en la tabla de páginas
 * @param presente Indica si la página está en memoria (1) o no (0)
 * @param modificado Indica si la página fue modificada (1) o no (0)
 * @param bit_uso Bit de uso para el algoritmo CLOCK-M
 * @param marco Número de marco físico asignado a la página
 * @param file_name Nombre del File al que pertenece la página
 * @param tag_name Nombre del Tag al que pertenece la página
 * @param numero_pagina Número de página dentro del File:Tag
 * @param ultimo_acceso Timestamp del último acceso (para LRU)
 */
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

/**
 * @brief Enumeración de los algoritmos de reemplazo soportados
 */
typedef enum {
    ALGORITMO_NO_DEFINIDO, // Valor 0
    ALGORITMO_LRU,         // Valor 1
    ALGORITMO_CLOCK_M      // Valor 2
} t_algoritmo_reemplazo;


// Estructura para la tabla de páginas completa
/**
 * @brief Estructura que representa la tabla de páginas
 * @param entradas Array dinámico de punteros a entradas de tabla de páginas
 * @param cantidad_entradas Cantidad de entradas en la tabla
 * @param algoritmo_reemplazo Algoritmo de reemplazo usado (LRU o CLOCK-M)
 * @param puntero_clock Puntero para el algoritmo CLOCK-M
 */
typedef struct {
    entrada_tabla_paginas** entradas;
    int cantidad_entradas;
    t_algoritmo_reemplazo algoritmo_reemplazo; // LRU o CLOCK-M (usar las constantes de la config)
    int puntero_clock;      // Para CLOCK-M
} tabla_paginas;


// Estructura para la memoria interna
/**
 * @brief Estructura que representa la memoria interna del worker
 * @param memoria Puntero al bloque de memoria asignado
 * @param tam_memoria Tamaño total de la memoria
 * @param tam_pagina Tamaño de cada página
 * @param cantidad_marcos Cantidad total de marcos en la memoria
 * @param marcos_libres Array que indica si cada marco está libre (1) o ocupado (0)
 * @param tabla Puntero a la tabla de páginas asociada
 */
typedef struct {
    void* memoria;          // El malloc con la memoria
    int tam_memoria;        // TAM_MEMORIA del config
    int tam_pagina;         // Tamaño de página (hardcodeado por ahora)
    int cantidad_marcos;    // tam_memoria/tam_pagina
    int* marcos_libres;     // Array para tracking (rastreo) de marcos
    tabla_paginas* tabla;   // Tabla de páginas
} memoria_interna;

#endif