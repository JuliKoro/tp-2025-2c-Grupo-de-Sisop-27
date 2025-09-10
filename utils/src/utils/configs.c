#include "configs.h"


query_control_conf* get_configs_query_control(char* nombre_config){
    t_config* config = NULL;
    config = config_create(nombre_config);
    query_control_conf* query_control_conf = malloc(sizeof(*query_control_conf));

    if(config_has_property(config, "IP_MASTER")){
                char* temp = config_get_string_value(config, "IP_MASTER");
                query_control_conf->ip_master = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad IP_MASTER en el archivo de configuracion\n");
    }

    if(config_has_property(config, "PUERTO_MASTER")){
        query_control_conf->puerto_master = config_get_int_value(config, "PUERTO_MASTER");
    } else {
        printf("ERROR: No se encontro la propiedad PUERTO_MASTER en el archivo de configuracion\n");
    }

    if(config_has_property(config, "LOG_LEVEL")){
        char* temp = config_get_string_value(config, "LOG_LEVEL");
        query_control_conf->log_level = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad LOG_LEVEL en el archivo de configuracion\n");
    }
    
    return query_control_conf;
}

master_conf* get_configs_master(char* nombre_config){
    t_config* config = NULL;
    config = config_create(nombre_config);

    if (config == NULL){
        printf("ERROR: No se pudo crear el t_config\n");
        return NULL;
    }

    
    master_conf* master_conf = malloc(sizeof(*master_conf));

    if(config_has_property(config, "PUERTO_ESCUCHA")){
        master_conf->puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    } else {
        printf("ERROR: No se encontro la propiedad PUERTO_ESCUCHA en el archivo de configuracion\n");
    }
    if(config_has_property(config, "ALGORITMO_PLANIFICACION")){
        char* temp = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
        master_conf->algoritmo_planificacion = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad ALGORITMO_PLANIFICACION en el archivo de configuracion\n");
    }
    if(config_has_property(config, "TIEMPO_AGING")){
        master_conf->tiempo_aging = config_get_int_value(config, "TIEMPO_AGING");
    } else {
        printf("ERROR: No se encontro la propiedad TIEMPO_AGING en el archivo de configuracion\n");
    }
    if(config_has_property(config, "LOG_LEVEL")){
        char* temp = config_get_string_value(config, "LOG_LEVEL");
        master_conf->log_level = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad LOG_LEVEL en el archivo de configuracion\n");
    }   
    config_destroy(config);
    return master_conf;
}

worker_conf* get_configs_worker(char* nombre_config){
    t_config* config = NULL;
    config = config_create(nombre_config);
    worker_conf* worker_conf = malloc(sizeof(*worker_conf));

    if(config_has_property(config, "IP_MASTER")){
        char* temp = config_get_string_value(config, "IP_MASTER");
        worker_conf->ip_master = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad IP_MASTER en el archivo de configuracion\n");
    }
    if(config_has_property(config, "PUERTO_MASTER")){
        worker_conf->puerto_master = config_get_int_value(config, "PUERTO_MASTER");
    } else {
        printf("ERROR: No se encontro la propiedad PUERTO_MASTER en el archivo de configuracion\n");
    }
    if(config_has_property(config, "IP_STORAGE")){
        char* temp = config_get_string_value(config, "IP_STORAGE");
        worker_conf->ip_storage = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad IP_STORAGE en el archivo de configuracion\n");
    }
    if(config_has_property(config, "PUERTO_STORAGE")){
        worker_conf->puerto_storage = config_get_int_value(config, "PUERTO_STORAGE");
    } else {
        printf("ERROR: No se encontro la propiedad PUERTO_STORAGE en el archivo de configuracion\n");
    }
    if(config_has_property(config, "TAM_MEMORIA")){
        worker_conf->tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    } else {
        printf("ERROR: No se encontro la propiedad TAM_MEMORIA en el archivo de configuracion\n");
    }
    if(config_has_property(config, "RETARDO_MEMORIA")){
        worker_conf->retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
    } else {
        printf("ERROR: No se encontro la propiedad RETARDO_MEMORIA en el archivo de configuracion\n");
    }
    if(config_has_property(config, "ALGORITMO_REEMPLAZO")){
        char* temp = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
        worker_conf->algoritmo_reemplazo = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad ALGORITMO_REEMPLAZO en el archivo de configuracion\n");
    }
    if(config_has_property(config, "PATH_QUERIES")){
        char* temp = config_get_string_value(config, "PATH_QUERIES");
        worker_conf->path_queries = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad PATH_QUERIES en el archivo de configuracion\n");
    }
    if(config_has_property(config, "LOG_LEVEL")){
        char* temp = config_get_string_value(config, "LOG_LEVEL");
        worker_conf->log_level = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad LOG_LEVEL en el archivo de configuracion\n");
    }
    config_destroy(config);
    return worker_conf;
}

storage_conf* get_configs_storage(char* nombre_config){
    t_config* config = NULL;
    config = config_create(nombre_config);
    storage_conf* storage_conf = malloc(sizeof(*storage_conf));

    if(config_has_property(config, "PUERTO_ESCUCHA")){
        storage_conf->puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    } else {
        printf("ERROR: No se encontro la propiedad PUERTO_ESCUCHA en el archivo de configuracion\n");
    }
    if(config_has_property(config, "FRESH_START")){
        storage_conf->fresh_start = config_get_int_value(config, "FRESH_START");
    } else {
        printf("ERROR: No se encontro la propiedad FRESH_START en el archivo de configuracion\n");
    }
    if(config_has_property(config, "PUNTO_MONTAJE")){
        char* temp = config_get_string_value(config, "PUNTO_MONTAJE");
        storage_conf->punto_montaje = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad PUNTO_MONTAJE en el archivo de configuracion\n");
    }
    if(config_has_property(config, "RETARDO_OPERACION")){
        storage_conf->retardo_operacion = config_get_int_value(config, "RETARDO_OPERACION");
    } else {
        printf("ERROR: No se encontro la propiedad RETARDO_OPERACION en el archivo de configuracion\n");
    }   
    if(config_has_property(config, "RETARDO_ACCESO_BLOQUE")){
        storage_conf->retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
    } else {
        printf("ERROR: No se encontro la propiedad RETARDO_ACCESO_BLOQUE en el archivo de configuracion\n");
    }
    if(config_has_property(config, "LOG_LEVEL")){   
        char* temp = config_get_string_value(config, "LOG_LEVEL");
        storage_conf->log_level = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad LOG_LEVEL en el archivo de configuracion\n");
    }
    config_destroy(config);
    return storage_conf;
}

void destruir_configs_query_control(query_control_conf* query_control_conf){
    printf("Destruyendo configuracion de query control\n");
    free(query_control_conf->ip_master);
    free(query_control_conf->log_level);
    free(query_control_conf);
}
void destruir_configs_master(master_conf* master_conf){
    printf("Destruyendo configuracion de master\n");
    free(master_conf->log_level);
    free(master_conf->algoritmo_planificacion);
    free(master_conf);
}
void destruir_configs_worker(worker_conf* worker_conf){
    printf("Destruyendo configuracion de worker\n");
    free(worker_conf->ip_master);
    free(worker_conf->ip_storage);
    free(worker_conf->algoritmo_reemplazo);
    free(worker_conf->path_queries);
    free(worker_conf->log_level);
    free(worker_conf);
}
void destruir_configs_storage(storage_conf* storage_conf){
    printf("Destruyendo configuracion de storage\n");
    free(storage_conf->punto_montaje);
    free(storage_conf->log_level);
    free(storage_conf);
}

