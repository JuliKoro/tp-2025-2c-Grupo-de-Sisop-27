#ifndef MI_Y_ALGOSUST_H_
#define MI_Y_ALGOSUST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include "estructuras.h"
#include <utils/configs.h> //para worker_conf

// Variable global de memoria (declarada como extern para que worker.c la vea)
extern memoria_interna* memoria_worker;

// --- FUNCIONES DE MEMORIA ---

/**
 * @brief Inicializa las estructuras administrativas de la memoria y reserva el espacio contiguo.
 */
memoria_interna* inicializar_memoria(int tam_memoria_config, int tam_pagina_hardcodeado);

/**
 * @brief Libera toda la memoria reservada por el módulo.
 */
void destruir_memoria(memoria_interna* mem);

/**
 * @brief Muestra por stderr el estado actual de marcos y tabla de páginas (Debug).
 */
void mostrar_estado_memoria(memoria_interna* mem);

/**
 * @brief Busca una página en la tabla. Si la encuentra (hit), actualiza bits de uso/acceso.
 */
entrada_tabla_paginas* buscar_pagina(tabla_paginas* tabla, const char* file, const char* tag, int num_pagina);

/**
 * @brief Agrega una nueva entrada a la tabla de páginas.
 */
void agregar_entrada_tabla(tabla_paginas* tabla, const char* file, const char* tag, int num_pagina, int marco);


// --- ALGORITMOS DE SUSTITUCIÓN ---

int encontrar_victima_lru(tabla_paginas* tabla);

int encontrar_victima_clock_m(tabla_paginas* tabla);

/**
 * @brief Ejecuta el algoritmo configurado para desalojar una página y obtener un marco libre.
 */
int ejecutar_algoritmo_reemplazo(memoria_interna* mem, const char* file_nuevo, const char* tag_nuevo, int pag_nueva);

#endif