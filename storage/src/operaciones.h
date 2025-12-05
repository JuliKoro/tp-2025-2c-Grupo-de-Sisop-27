#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <s_funciones.h>
/** 
 * @brief Simula un retardo en la operación de almacenamiento según la configuración.
 */
void simularRetardoOperacion();
/** 
 * @brief Simula un retardo en el acceso a bloques físicos según la configuración.
 */
void simularRetardoAccesoBloque();

/** 
 * @brief Lee un bloque físico específico del sistema de almacenamiento.
 * @param bloqueFisico Número del bloque físico a leer.
 * @return Puntero al buffer que contiene los datos leídos, o NULL en caso de error.
 */
void* leerBloqueFisico(int bloqueFisico);

/** 
 * @brief Crea un archivo con un tag especificado en el sistema de almacenamiento. 
 * @param nombreFile Nombre del archivo a crear.
 * @param nombreTag Nombre del tag asociado al archivo.
 * @param query_id Identificador de la consulta para seguimiento.
 * @return 0 si la creación fue exitosa, -1 en caso de error.
 */
int create(u_int32_t query_id, char* nombreFile, char* nombreTag);

/**
* @brief Crea un nuevo tag para un archivo existente copiando desde un tag origen.
 * @param nombreFile Nombre del archivo al que pertenece el tag.
 * @param tagOrigen Nombre del tag desde el cual se copiarán los datos.
 * @param tagDestino Nombre del nuevo tag que se creará.
 * @param query_id Identificador de la consulta para seguimiento.
 * @return 0 si la operación fue exitosa, -1 en caso de error.
 */
int tag(uint32_t query_id,char* nombreFile, char* tagOrigen, char* tagDestino);

/**
 * @brief Lee un bloque lógico específico de un archivo y tag dados.
 * @param query_id query id
 * @param nombreFile Nombre del archivo del cual se leerá el bloque.
 * @param nombreTag Nombre del tag asociado al archivo.
 * @param bloqueLogico Número del bloque lógico a leer.
 * @return Puntero al buffer que contiene los datos leídos, o NULL en caso de error.
 */
void* leer(uint32_t query_id, char* nombreFile, char* nombreTag, uint32_t bloqueLogico);

/**
 * @brief Libera un bloque físico si ya no está siendo referenciado por ningún archivo lógico.
 * @param query_id Identificador de la consulta para seguimiento.
 * @param numeroBloque Número del bloque físico a liberar.
 */
void liberarBloqueSiEsNecesario(u_int32_t query_id, int numeroBloque);

/**
 * @brief Elimina un tag específico de un archivo en el sistema de almacenamiento.
 * @param query_id query id
 * @param nombreFile Nombre del archivo del cual se eliminará el tag.
 * @param nombreTag Nombre del tag a eliminar.
 * @return 0 si la eliminación fue exitosa, -1 en caso de error.
 */
int eliminarTag(u_int32_t query_id, char* nombreFile, char* nombreTag);

#endif