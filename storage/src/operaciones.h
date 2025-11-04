#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <s_funciones.h>

/** 
 * @brief Crea un archivo con un tag especificado en el sistema de almacenamiento. 
 * @param nombreFile Nombre del archivo a crear.
 * @param nombreTag Nombre del tag asociado al archivo.
 * @return 0 si la creación fue exitosa, -1 en caso de error.
 */
int create(char* nombreFile, char* nombreTag);

#endif