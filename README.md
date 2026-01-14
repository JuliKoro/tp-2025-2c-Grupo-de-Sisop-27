# TP-UTN.SO-2C2025: "Master of Files"

Trabajo Práctico de la materia Sistemas Operativos de la carrera Ingeniería en Sistemas de la Información de la UTN FRBA.

El TP consistió en desarrollar una solución que permita la simulación de un sistema distribuido, donde se tuvo que planificar procesos, resolver peticiones al sistema, y administrar de manera adecuada una memoria bajo los esquemas explicados en sus correspondientes módulos.

- [Enunciado TP](https://docs.google.com/document/d/1mEGwXjwtQzD0T6Pl8gEhgJcmQR_3nmVI6BxukAmumsM/edit?usp=sharing "Master of Files")
- [Documento de pruebas Finales](https://docs.google.com/document/d/1ZK_5ZxZA0PYuC1uaomcghz3bCzollkjo-vx7t6Kb6qI/edit?usp=sharing "Master of Files")
- [Repositorio de pruebas Finales](https://github.com/sisoputnfrba/master-of-files-pruebas.git)

## Despliegue

Para facilitar la instalación y ejecución del trabajo práctico en el entorno de laboratorio o máquinas virtuales, se incluyen dos scripts de utilidad:

### Script de Despliegue (`deploy-27.sh`)
Este script automatiza la descarga de dependencias (commons, so-deploy), la configuración de IPs para el entorno distribuido y la compilación de todos los módulos.

**Uso:**
```bash
chmod +x deploy-27.sh
./deploy-27.sh
```
Al ejecutarlo, solicitará las IPs de los módulos `Master` y `Storage` (presionar Enter para usar `127.0.0.1` por defecto).

### Script de Pruebas (`scripts-pruebas.sh`)
Este script permite configurar rápidamente los archivos de configuración (`.config`) para los distintos escenarios de prueba (Planificación, Memoria, etc.) y ejecutar los módulos individualmente.

**Uso:**
```bash
chmod +x scripts-pruebas.sh
./scripts-pruebas.sh
```
Ofrece un menú interactivo para:
1. Configurar IPs en todos los módulos.
2. Seleccionar escenarios de prueba (FIFO, LRU, etc.).
3. Iniciar módulos específicos (Master, Storage, Worker).

## Dependencias

Para poder compilar y ejecutar el proyecto, es necesario tener instalada la
biblioteca [so-commons-library] de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
make debug
make install
```

## Compilación y ejecución

Cada módulo del proyecto se compila de forma independiente a través de un
archivo `makefile`. Para compilar un módulo, es necesario ejecutar el comando
`make` desde la carpeta correspondiente.

El ejecutable resultante de la compilación se guardará en la carpeta `bin` del
módulo. Ejemplo:

```sh
cd kernel
make
./bin/kernel
```

## Importar desde Visual Studio Code

Para importar el workspace, debemos abrir el archivo `tp.code-workspace` desde
la interfaz o ejecutando el siguiente comando desde la carpeta raíz del
repositorio:

```bash
code tp.code-workspace
```

## Entrega

Para desplegar el proyecto en una máquina Ubuntu Server, podemos utilizar el
script [so-deploy] de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-deploy.git
cd so-deploy
./deploy.sh -r=release -p=utils -p=query_control -p=master -p=worker -p=storage "tp-{año}-{cuatri}-{grupo}"
```

El mismo se encargará de instalar las Commons, clonar el repositorio del grupo
y compilar el proyecto en la máquina remota.

> [!NOTE]
> Ante cualquier duda, pueden consultar la documentación en el repositorio de
> [so-deploy], o utilizar el comando `./deploy.sh --help`.

## Links útiles

- [Blog UTN.SO](https://www.utnso.com.ar/)
- [Docs UTN.SO](https://docs.utnso.com.ar/)
- [Foro del TP](https://github.com/sisoputnfrba/foro/issues)
- [Cómo interpretar errores de compilación](https://docs.utnso.com.ar/primeros-pasos/primer-proyecto-c#errores-de-compilacion)
- [Cómo utilizar el debugger](https://docs.utnso.com.ar/guias/herramientas/debugger)
- [Cómo configuramos Visual Studio Code](https://docs.utnso.com.ar/guias/herramientas/code)
- **[Guía de despliegue de TP](https://docs.utnso.com.ar/guías/herramientas/deploy)**

[so-commons-library]: https://github.com/sisoputnfrba/so-commons-library
[so-deploy]: https://github.com/sisoputnfrba/so-deploy
