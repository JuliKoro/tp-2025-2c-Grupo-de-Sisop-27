#ifndef REGISTROS_H_
#define REGISTROS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

// REGISTROS
/**
 * @brief Variable global para el PC (Program Counter)
 * @note Si se usan hilos, hay que proteger la variable con mutex
 */
extern uint32_t pc_actual = 0; // Variable global para el PC (Program Counter)

/**
 * @brief Variable global para el path de la Query actual
 * @note Si se usan hilos, hay que proteger la variable con mutex
 */
extern char* path_query = NULL;           // Path de la Query actual

// FLAGS
extern bool query_en_ejecucion = false;          // Flag: ¿Hay una Query activa?
extern bool desalojar_query = false;             // Flag para desalojo (chequeada en interpreter)
//bool hay_query_activa = false;            // Flag para saber si hay una query activa (chequeada en master)

#endif