#include <utils/hello.h>
#include <utils/configs.h>

int main(int argc, char* argv[]) {
    saludar("master");
    master_conf* master_conf = get_configs_master("master.config");
    printf("PUERTO_ESCUCHA: %d\n", master_conf->puerto_escucha);
    printf("ALGORITMO_PLANIFICACION: %s\n", master_conf->algoritmo_planificacion);
    printf("TIEMPO_AGING: %d\n", master_conf->tiempo_aging);
    printf("LOG_LEVEL: %s\n", master_conf->log_level);
    destruir_configs_master(master_conf);
    return 0;
}
