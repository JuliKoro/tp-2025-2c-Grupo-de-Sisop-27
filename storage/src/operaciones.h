#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <s_funciones.h>

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

#endif