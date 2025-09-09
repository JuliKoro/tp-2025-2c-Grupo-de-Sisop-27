#ifndef CONFIGS_H
#define CONFIGS_H

#include <commons/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    MODULO_QUERY_CONTROL,
    MODULO_MASTER,
    MODULO_WORKER,
    MODULO_STORAGE,
} e_modulos_conf;

 t_config* get_configs(e_modulos_conf modulo, char* nombre_config);





#endif