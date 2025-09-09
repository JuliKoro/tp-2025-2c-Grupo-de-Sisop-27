#include "configs.h"

char* modulo_desde_enum_conf(e_modulos_conf modulo){
    switch(modulo){
        case MODULO_QUERY_CONTROL:
            return "query_control";
    }
}

t_config* get_configs(e_modulos_conf modulo, char* nombre_config){
    t_config* config = NULL;

    switch(modulo){
}