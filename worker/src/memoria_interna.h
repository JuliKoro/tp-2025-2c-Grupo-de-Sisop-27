#ifndef MEMORIA_INTERNA_H_
#define MEMORIA_INTERNA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <utils/loggeo.h>
#include <utils/configs.h> //para worker_conf
#include <utils/mensajeria.h>

#include "registros.h" // para logger_worker
#include "tabla_de_paginas.h"

// Variable global de memoria (declarada como extern para que worker.c la vea)
/**
 * @brief Variable global que representa la memoria interna del worker
 */
extern memoria_interna* memoria_worker;

// --- FUNCIONES DE MEMORIA INTERNA ---

/**
 * @brief Inicializa las estructuras administrativas de la memoria y reserva el espacio contiguo.
 */
memoria_interna* inicializar_memoria();

/**
 * @brief Libera toda la memoria reservada por el módulo.
 * 
 * Usa la variable global memoria_worker directamente.
 */
void destruir_memoria(void);

/**
 * @brief Muestra por stderr el estado actual de marcos y tabla de páginas (Debug).
 * 
 * Usa la variable global memoria_worker directamente.
 */
void mostrar_estado_memoria(void);

/**
 * @brief Busca una página en la tabla. Si la encuentra (hit), actualiza bits de uso/acceso.
 * @note Usa la variable global memoria_worker->tabla directamente.
 */
entrada_tabla_paginas* buscar_pagina(const char* file, const char* tag, int num_pagina);

/**
 * @brief Agrega una nueva entrada a la tabla de páginas.
 * @note Usa la variable global memoria_worker->tabla directamente.
 */
void agregar_entrada_tabla(const char* file, const char* tag, int num_pagina, int marco);


// --- ALGORITMOS DE SUSTITUCIÓN ---

/**
 * @brief Encuentra la víctima según el algoritmo LRU.
 * @note Usa la variable global memoria_worker->tabla directamente.
 * @return Índice de la entrada víctima
 */
int encontrar_victima_lru(void);

/**
 * @brief Encuentra la víctima según el algoritmo CLOCK-M.
 * @note Usa la variable global memoria_worker->tabla directamente.
 * @return Índice de la entrada víctima
 */
int encontrar_victima_clock_m(void);

/**
 * @brief Ejecuta el algoritmo de reemplazo configurado y desaloja una página.
 * @param file_nuevo Nombre del archivo de la nueva página
 * @param tag_nuevo Nombre del tag de la nueva página
 * @param pag_nueva Número de página de la nueva página
 * @return Número de marco liberado
 * @note Función "despachadora" que llama al algoritmo de reemplazo
 * configurado, desaloja la página y devuelve el marco liberado.
 */
int ejecutar_algoritmo_reemplazo(const char* file_nuevo, const char* tag_nuevo, int pag_nueva);

// --- FUNCIONES DE TRADUCCIÓN Y ACCESO A MEMORIA ---

/**
 * @brief Busca un marco libre en memoria
 * 
 * Usa la variable global memoria_worker directamente.
 * 
 * @return Número de marco libre, o -1 si no hay marcos libres
 */
int buscar_marco_libre(void);

/**
 * @brief Calcula el número de página a partir de una dirección lógica
 * 
 * @param dir_logica Dirección lógica
 * @return Número de página
 */
uint32_t numero_pagina(uint32_t dir_logica);

/**
 * @brief Calcula el offset dentro de la página a partir de una dirección lógica
 * 
 * @param dir_logica Dirección lógica
 * @return Offset dentro de la página
 */
uint32_t offset_pagina(uint32_t dir_logica);

/**
 * @brief Traduce una dirección lógica a física
 * 
 * Usa la variable global memoria_worker directamente.
 * 
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @param dir_logica Dirección lógica a traducir
 * @param dir_fisica [OUT] Dirección física resultante
 * @return true si la traducción fue exitosa, false si hubo page fault
 */
bool traducir_direccion(const char* file, 
                        const char* tag,
                        uint32_t dir_logica, 
                        uint32_t* dir_fisica);

/**
 * @brief Maneja un page fault: carga una página desde Storage
 * 
 * Usa la variable global memoria_worker directamente.
 * 
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @param num_pagina Número de página a cargar
 * @param marco [OUT] Número de marco asignado
 * @return Resultado de la operación
 */
t_resultado_ejecucion manejar_page_fault(const char* file, const char* tag, uint32_t num_pagina, int* marco);

/**
 * @brief Solicita un bloque a Storage
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @param num_pagina Número de página a solicitar
 * @param tamanio Tamaño del bloque a solicitar
 * @return true si la solicitud fue enviada correctamente
 */
bool solicitar_bloque_storage(const char* file, const char* tag, uint32_t num_pagina, uint32_t tamanio);

/**
 * @brief Recibe un bloque de Storage
 * @param bloque [OUT] Puntero donde se almacenará el bloque recibido
 * @return Resultado de la operación
 */
t_resultado_ejecucion recibir_bloque_storage(void* bloque);

// --- OPERACIONES DE MEMORIA INTERNA/STORAGE ---

/**
 * @brief Lee contenido de la memoria interna
 * 
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @param dir_logica Dirección lógica inicial
 * @param tamanio Cantidad de bytes a leer
 * @param buffer [OUT] Buffer donde se copiará el contenido
 * @return Resultado de la operación
 */
t_resultado_ejecucion leer_memoria(const char* file, const char* tag, uint32_t dir_logica, uint32_t tamanio, void* buffer);

/**
 * @brief Escribe contenido en la memoria interna
 * 
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @param dir_logica Dirección lógica inicial
 * @param contenido Contenido a escribir
 * @param tamanio Cantidad de bytes a escribir
 * @return Resultado de la operación
 */
t_resultado_ejecucion escribir_memoria(const char* file, const char* tag, uint32_t dir_logica, const void* contenido, uint32_t tamanio);

/**
 * @brief Persiste todas las páginas modificadas de un File:Tag
 * 
 * Usa la variable global memoria_worker directamente.
 * 
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @return true si el flush fue exitoso
 */
bool flush_file_tag(const char* file,
                    const char* tag);

#endif
