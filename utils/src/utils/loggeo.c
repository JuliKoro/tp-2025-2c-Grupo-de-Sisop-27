#include "loggeo.h"

t_log* iniciarLoggerQC(char* archivo_query, char* log_level){
    char nombre[128];
    char nombre_archivo[512];
    snprintf(nombre, sizeof(nombre), "QUERY_CONTROL - %s", archivo_query);
    snprintf(nombre_archivo, sizeof(nombre_archivo), "%s.log", nombre);
    t_log* logger = log_create(nombre_archivo, nombre, 1, log_level_from_string(log_level));
    log_info(logger, "Logger de Query Control inicializado");
    return logger;
}
t_log* iniciarLoggerMaster(char* log_level){
    t_log* logger = log_create("master.log", "MASTER", 1, log_level_from_string(log_level));
    log_info(logger, "Logger de Master inicializado");
    return logger;
}


t_log* iniciarLoggerWorker(char* id_worker, char* log_level){
    char nombre[128];
    char nombre_archivo[256];
    snprintf(nombre, sizeof(nombre), "WORKER - %s", id_worker);
    snprintf(nombre_archivo, sizeof(nombre_archivo), "%s.log", nombre);
    t_log* logger = log_create(nombre_archivo, nombre, 1, log_level_from_string(log_level));
    log_info(logger, "Logger de Worker %s inicializado", id_worker);
    return logger;
}


t_log* iniciarLoggerStorage(char* log_level){
    t_log* logger = log_create("storage.log", "STORAGE", 1, log_level_from_string(log_level));
    log_info(logger, "Logger de Storage inicializado");
    return logger;
}
