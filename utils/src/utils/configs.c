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
        char* temp = config_get_string_value(config, "FRESH_START");
        if (strcmp(temp, "TRUE") == 0) {
            storage_conf->fresh_start = true;
        } else if (strcmp(temp, "FALSE") == 0) {
        storage_conf->fresh_start = false;
        } else {
            printf("ERROR: La propiedad FRESH_START debe ser TRUE o FALSE\n");
            storage_conf->fresh_start = false; // Valor por defecto en caso de error
        }
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

superblock_conf* get_configs_superblock(char* nombre_config){
    t_config* config = NULL;
    config = config_create(nombre_config);
    superblock_conf* superblock_conf = malloc(sizeof(*superblock_conf));

    if(config_has_property(config, "FS_SIZE")){
        superblock_conf->fs_size = config_get_int_value(config, "FS_SIZE");
    } else {
        printf("ERROR: No se encontro la propiedad FS_SIZE en el archivo de configuracion\n");
    }
    if(config_has_property(config, "BLOCK_SIZE")){
        superblock_conf->block_size = config_get_int_value(config, "BLOCK_SIZE");
    } else {
        printf("ERROR: No se encontro la propiedad BLOCK_SIZE en el archivo de configuracion\n");
    }
    superblock_conf->cantidad_bloques = superblock_conf->fs_size / superblock_conf->block_size;
    config_destroy(config);
    return superblock_conf;
}

metadata_conf* get_configs_metadata(char* nombre_metadata){
    t_config* config = NULL;
    config = config_create(nombre_metadata);
    metadata_conf* metadata_conf = malloc(sizeof(*metadata_conf));

    if(config_has_property(config, "TAMANIO")){
        metadata_conf->tamanio = config_get_int_value(config, "TAMANIO");
    } else {
        printf("ERROR: No se encontro la propiedad TAMANIO en el archivo de metadata\n");
    }
    if(config_has_property(config, "BLOCKS")){
        int cantidad = 0;
        char** string_array = config_get_array_value(config, "BLOCKS");
        while(string_array[cantidad]!= NULL){
            cantidad++;
        }
        int* int_array = malloc(cantidad * sizeof(int));

        if(int_array == NULL){
            printf("No se pudo asignar memoria para el int array de bloques");
        }

        for(int i = 0; i< cantidad; i++){
            int_array[i] = atoi(string_array[i]);
        }

        string_array_destroy(string_array);

        metadata_conf->blocks = int_array;
        metadata_conf->cantidad_blocks= cantidad;




    } else {
        printf("ERROR: No se encontro la propiedad BLOCKS en el archivo de metadata\n");
    }
    if(config_has_property(config, "ESTADO")){
        char* temp = config_get_string_value(config, "ESTADO");
        metadata_conf->estado = strdup(temp);
    } else {
        printf("ERROR: No se encontro la propiedad ESTADO en el archivo de metadata\n");
    }
    config_destroy(config);
    return metadata_conf;
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
void destruir_configs_superblock(superblock_conf* superblock_conf){
    printf("Destruyendo configuracion de storage superblock\n");
    free(superblock_conf);
}
void destruir_configs_metadata(metadata_conf* metadata_conf){
    printf("Destruyendo configuracion de storage metadafa\n");
    free(metadata_conf->estado);
    free(metadata_conf);
}
