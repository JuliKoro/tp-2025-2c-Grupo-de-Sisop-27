#!/bin/bash

# Directorios (AJUSTAR SI ES NECESARIO)
BASE_DIR=$(pwd)
CONF_MASTER="$BASE_DIR/master/master.config"
CONF_STORAGE="$BASE_DIR/storage/storage.config"
CONF_WORKER="$BASE_DIR/worker/worker.config"
# Asumimos que bajaste el repo de pruebas en la carpeta anterior o ajusta el path
PATH_PRUEBAS="../master-of-files-pruebas" 

# Colores para prints
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Función para actualizar configs
update_config() {
    key=$1
    value=$2
    file=$3
    # Usa sed para reemplazar la linea que empieza con la key
    sed -i "s|^$key=.*|$key=$value|" "$file"
    echo -e "Config actualizada en $file: $key=$value"
}

# Función para configurar IPs en todos los módulos
configurar_ips() {
    echo -e "${GREEN}Configurando IPs de la red...${NC}"
    read -p "Ingrese IP del MASTER (Ej: 192.168.x.x): " IP_MASTER
    read -p "Ingrese IP del STORAGE (Ej: 192.168.x.x): " IP_STORAGE

    # Ajustar claves según tus archivos config reales (IP_MASTER, IP_STORAGE, IP_FILESYSTEM, etc)
    update_config "IP_MASTER" "$IP_MASTER" "$CONF_WORKER"
    update_config "IP_FILESYSTEM" "$IP_STORAGE" "$CONF_WORKER" # A veces se llama IP_FILESYSTEM
    update_config "IP_STORAGE" "$IP_STORAGE" "$CONF_WORKER"    # O IP_STORAGE
    
    update_config "IP_STORAGE" "$IP_STORAGE" "$CONF_MASTER"
    
    echo -e "${GREEN}IPs actualizadas.${NC}"
}

start_module() {
    module=$1
    echo -e "${GREEN}Iniciando $module...${NC}"
    cd "$BASE_DIR/$module"
    ./bin/$module $module.config
}

kill_all() {
    echo -e "${RED}Matando procesos...${NC}"
    pkill -9 master
    pkill -9 worker
    pkill -9 storage
    sleep 2
}

# MENU PRINCIPAL
echo "=========================================="
echo "   MASTER OF FILES - AUTOMATIZACIÓN 2C2025"
echo "   (Entorno Distribuido / Laboratorio)"
echo "=========================================="
echo "--- CONFIGURACIÓN ---"
echo "1) Configurar IPs (Master/Storage)"
echo "2) Configurar Escenario: PLANIFICACIÓN (FIFO/PRIORIDADES)"
echo "3) Configurar Escenario: MEMORIA (CLOCK-M/LRU)"
echo "4) Configurar Escenario: ERRORES"
echo "5) Configurar Escenario: STORAGE"
echo "6) Configurar Escenario: ESTABILIDAD"
echo ""
echo "--- EJECUCIÓN (Seleccionar según la PC) ---"
echo "7) Iniciar MASTER"
echo "8) Iniciar STORAGE"
echo "9) Iniciar WORKER"
echo "10) Matar procesos locales (pkill)"
echo "=========================================="
read -p "Seleccione una opción: " OPCION

case $OPCION in
    1)
        configurar_ips
        ;;
    2)
        echo ">>> CONFIGURANDO PRUEBA PLANIFICACIÓN <<<"
        echo "a) Parte 1: FIFO"
        echo "b) Parte 2: PRIORIDADES + AGING"
        read -p "Seleccione sub-test: " SUB

        if [ "$SUB" == "a" ]; then
        # --- PARTE 1: FIFO ---
        update_config "ALGORITMO_PLANIFICACION" "FIFO" "$CONF_MASTER"
        update_config "TIEMPO_AGING" "0" "$CONF_MASTER" # Desactivar aging temporalmente si es necesario o poner un valor alto
        
        # Storage Configs 
        update_config "RETARDO_OPERACION" "25" "$CONF_STORAGE"
        update_config "RETARDO_ACCESO_BLOQUE" "25" "$CONF_STORAGE"
        update_config "FRESH_START" "TRUE" "$CONF_STORAGE"

        # Worker Configs (Asumimos 1 config base, si usas multiples workers en misma PC, duplica el config)
        update_config "TAM_MEMORIA" "1024" "$CONF_WORKER"
        update_config "RETARDO_MEMORIA" "50" "$CONF_WORKER"
        update_config "ALGORITMO_REEMPLAZO" "LRU" "$CONF_WORKER"
        
        elif [ "$SUB" == "b" ]; then
        # --- PARTE 2: PRIORIDADES + AGING ---
        # [cite: 573, 581]
        update_config "ALGORITMO_PLANIFICACION" "PRIORIDADES" "$CONF_MASTER"
        update_config "TIEMPO_AGING" "500" "$CONF_MASTER"
        update_config "FRESH_START" "FALSE" "$CONF_STORAGE"
        fi
        echo "Configuración aplicada. Ahora inicie los módulos correspondientes."
        ;;

    3)
        echo ">>> CONFIGURANDO PRUEBA MEMORIA WORKER <<<"
        echo "a) Parte 1: CLOCK-M"
        echo "b) Parte 2: LRU"
        read -p "Seleccione sub-test: " SUB
        if [ "$SUB" == "a" ]; then
        # [cite: 594]
        update_config "TAM_MEMORIA" "48" "$CONF_WORKER" # 3 Páginas (48 / 16)
        update_config "RETARDO_MEMORIA" "25" "$CONF_WORKER"
        update_config "ALGORITMO_REEMPLAZO" "CLOCK-M" "$CONF_WORKER"
        
        update_config "RETARDO_OPERACION" "50" "$CONF_STORAGE"
        update_config "RETARDO_ACCESO_BLOQUE" "50" "$CONF_STORAGE"
        update_config "FRESH_START" "FALSE" "$CONF_STORAGE"
        
        elif [ "$SUB" == "b" ]; then
        # [cite: 590]
        update_config "ALGORITMO_REEMPLAZO" "LRU" "$CONF_WORKER"
        fi
        echo "Configuración aplicada."
        ;;

    4)
        echo ">>> CONFIGURANDO PRUEBA ERRORES <<<"
        # [cite: 609]
        update_config "ALGORITMO_PLANIFICACION" "FIFO" "$CONF_MASTER"
        update_config "TAM_MEMORIA" "256" "$CONF_WORKER"
        update_config "ALGORITMO_REEMPLAZO" "LRU" "$CONF_WORKER"
        update_config "FRESH_START" "FALSE" "$CONF_STORAGE"
        echo "Configuración aplicada."
        ;;

    5)
        echo ">>> CONFIGURANDO PRUEBA STORAGE <<<"
        # [cite: 620]
        update_config "ALGORITMO_PLANIFICACION" "PRIORIDADES" "$CONF_MASTER"
        update_config "TAM_MEMORIA" "128" "$CONF_WORKER"
        update_config "ALGORITMO_REEMPLAZO" "LRU" "$CONF_WORKER"
        update_config "FRESH_START" "FALSE" "$CONF_STORAGE"
        echo "Configuración aplicada."
        ;;

    6)
        echo ">>> CONFIGURANDO PRUEBA ESTABILIDAD <<<"
        # [cite: 632]
        update_config "ALGORITMO_PLANIFICACION" "PRIORIDADES" "$CONF_MASTER"
        update_config "TIEMPO_AGING" "25" "$CONF_MASTER"
        
        update_config "RETARDO_OPERACION" "100" "$CONF_STORAGE"
        update_config "RETARDO_ACCESO_BLOQUE" "10" "$CONF_STORAGE"
        update_config "FRESH_START" "FALSE" "$CONF_STORAGE"
        
        update_config "TAM_MEMORIA" "128" "$CONF_WORKER"
        update_config "RETARDO_MEMORIA" "10" "$CONF_WORKER"
        update_config "ALGORITMO_REEMPLAZO" "LRU" "$CONF_WORKER"
        echo "Configuración aplicada."
        ;;
        
    7)
        start_module "master"
        ;;
    8)
        start_module "storage"
        ;;
    9)
        start_module "worker"
        ;;
    10)
        kill_all
        ;;
    *)
        echo "Opción no válida"
        ;;
esac