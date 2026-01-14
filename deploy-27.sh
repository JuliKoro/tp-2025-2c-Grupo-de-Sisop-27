#!/bin/bash

# =========================================================
# CONFIGURACIÓN DEL REPOSITORIO
# =========================================================
TOKEN="token_de_github_aqui"
USER="username"
REPO="tp-2025-2c-Grupo-de-Sisop-27"

# =========================================================
# CONFIGURACIÓN DE RED (IPs del Laboratorio)
# =========================================================
echo "Ingrese IP del STORAGE (Enter para 127.0.0.1):"
read IP_STORAGE_INPUT
IP_STORAGE=${IP_STORAGE_INPUT:-127.0.0.1}

echo "Ingrese IP del MASTER (Enter para 127.0.0.1):"
read IP_MASTER_INPUT
IP_MASTER=${IP_MASTER_INPUT:-127.0.0.1}

# =========================================================
# PREPARACIÓN DE CREDENCIALES
# =========================================================
# Esto evita que el script pida contraseña al clonar
git config --global credential.helper store
echo "https://$USER:$TOKEN@github.com" > ~/.git-credentials

# =========================================================
# EJECUCIÓN DEL DEPLOY
# =========================================================

# 1. Ir al directorio HOME
cd ~

# 2. Clonar herramientas necesarias si no existen
if [ ! -d "so-deploy" ]; then
    echo "Clonando so-deploy..."
    git clone https://github.com/sisoputnfrba/so-deploy.git
fi

if [ ! -d "master-of-files-pruebas" ]; then
    echo "Clonando pruebas..."
    git clone https://github.com/sisoputnfrba/master-of-files-pruebas.git
fi

# 3. Ejecutar el deploy
echo "Iniciando despliegue con so-deploy..."
cd so-deploy

# Instala commons y clona el TP
./deploy.sh \
    -r=release \
    -p=storage \
    -p=worker \
    -p=master \
    -p=utils \
    -p=query_control \
    -c=IP_STORAGE=$IP_STORAGE \
    -c=IP_MASTER=$IP_MASTER \
    -c=LOG_LEVEL=DEBUG \
    "$REPO"

# 4. Mover el repo al HOME (para cumplir el requisito de no usar subdirectorios extra)
cd ~
if [ -d "so-deploy/$REPO" ]; then
    echo "Moviendo repositorio a $HOME/$REPO ..."
    mv "so-deploy/$REPO" .
else
    echo "El repositorio ya parece estar en el lugar correcto o hubo un error en el deploy."
fi

# =========================================================
# NOTA SOBRE REQUERIMIENTOS DEL TP
# =========================================================
# Recordá que el Storage debe inicializar el FS si FRESH_START=TRUE [cite: 374, 375]
# El script configurará automáticamente las IPs para que los módulos
# puedan conectarse entre sí en el entorno distribuido[cite: 115, 116].