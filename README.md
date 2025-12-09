# 📊 Distributed System Monitor

![Language](https://img.shields.io/badge/language-C-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-green.svg)
![Architecture](https://img.shields.io/badge/architecture-Client%2FServer-orange.svg)

Un sistema de monitoreo distribuido ligero desarrollado en **C**. Permite visualizar en tiempo real el estado de **CPU** y **Memoria** de múltiples máquinas remotas (agentes) reportando a un servidor central (recolector) en la nube.

[cite_start]Este proyecto utiliza **Sockets TCP** para la comunicación y lectura directa del sistema de archivos **/proc** de Linux para la extracción de métricas, sin depender de librerías externas de alto nivel[cite: 1, 92, 93].

---

## 🚀 Características

* **Arquitectura Cliente-Servidor:** Múltiples agentes envían datos a un recolector central.
* [cite_start]**Bajo Nivel:** Extracción de datos "crudos" parseando `/proc/stat` y `/proc/meminfo`[cite: 24, 41].
* [cite_start]**Visualización en Tiempo Real:** Tabla dinámica que se actualiza con las métricas de cada nodo conectado[cite: 13, 15].
* [cite_start]**Concurrencia:** El servidor maneja múltiples conexiones simultáneas[cite: 59].

## 🛠️ Arquitectura

[cite_start]El sistema consta de tres componentes principales[cite: 4]:

1.  [cite_start]**Agent CPU (`agent_cpu`):** Lee `/proc/stat`, calcula el porcentaje de uso de CPU y lo envía al servidor[cite: 39].
2.  [cite_start]**Agent Memoria (`agent_mem`):** Lee `/proc/meminfo`, extrae memoria total, libre y swap, y la envía al servidor[cite: 22].
3.  [cite_start]**Collector (Servidor):** Recibe los paquetes, procesa la información y muestra una tabla unificada en consola[cite: 54].

---

## 📋 Requisitos

* Sistema Operativo **Linux** (Ubuntu/Debian/Kali).
* Compilador **GCC**.
* Conexión a internet (para enviar datos a la nube).
* **Puerto 6000** abierto en el Firewall/Security Group del servidor.

---

## ⚙️ Instalación y Uso

### 1. Compilación
Puedes compilar todos los componentes usando el `Makefile` incluido:

```bash
make all
