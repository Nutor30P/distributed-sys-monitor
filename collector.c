#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

// --- Estructuras y Variables Globales ---

typedef struct {
    char ip_logica[32];
    int active;
    // Tiempos para saber si está desconectado
    time_t last_update; 
    
    // Métricas
    float cpu_usage;
    float cpu_user;
    float cpu_sys;
    float cpu_idle;
    float mem_used;
    float mem_free;
    float swap_total;
    float swap_free;
} HostInfo;

#define MAX_HOSTS 10
HostInfo hosts[MAX_HOSTS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// --- Funciones Auxiliares ---

int get_host_index(char *ip_logica) {
    pthread_mutex_lock(&lock);
    
    // 1. Buscar existente
    for (int i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].active && strcmp(hosts[i].ip_logica, ip_logica) == 0) {
            hosts[i].last_update = time(NULL); // Actualizamos tiempo de vida
            pthread_mutex_unlock(&lock);
            return i;
        }
    }
    // 2. Buscar hueco
    for (int i = 0; i < MAX_HOSTS; i++) {
        if (!hosts[i].active) {
            strcpy(hosts[i].ip_logica, ip_logica);
            hosts[i].active = 1;
            hosts[i].last_update = time(NULL);
            pthread_mutex_unlock(&lock);
            return i;
        }
    }
    pthread_mutex_unlock(&lock);
    return -1;
}

// --- Hilo Visualizador (Dibuja la tabla) ---
// Implementa el requisito y 
void *display_thread_func(void *arg) {
    while (1) {
        system("clear"); // Limpia pantalla 
        
        printf("=== MONITOR DE SISTEMA DISTRIBUIDO ===\n\n");
        // Encabezados de la tabla
        printf("%-15s | %-6s %-6s %-6s %-6s | %-9s %-9s\n", 
               "IP/Nombre", "CPU%", "User", "Sys", "Idle", "MemUsed", "MemFree");
        printf("-----------------------------------------------------------------------\n");

        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_HOSTS; i++) {
            if (hosts[i].active) {
                // Verificar si sigue vivo (si no actualiza en 5 seg, marcamos "Sin Datos")
                double seconds_since = difftime(time(NULL), hosts[i].last_update);
                
                if (seconds_since > 5.0) {
                     printf("%-15s | [SIN DATOS / DESCONECTADO] \n", hosts[i].ip_logica);
                } else {
                    printf("%-15s | %5.1f%% %5.1f%% %5.1f%% %5.1f%% | %6.0f MB %6.0f MB\n",
                        hosts[i].ip_logica,
                        hosts[i].cpu_usage, hosts[i].cpu_user, hosts[i].cpu_sys, hosts[i].cpu_idle,
                        hosts[i].mem_used, hosts[i].mem_free
                    );
                }
            }
        }
        pthread_mutex_unlock(&lock);

        printf("\n(Presiona Ctrl+C para salir)\n");
        sleep(2); // Refrescar cada 2 segundos 
    }
    return NULL;
}

// --- Hilo Cliente (Recibe datos) ---
void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    free(socket_desc);
    char buffer[1024];
    int read_size;

    while ((read_size = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        
        char temp[1024];
        strcpy(temp, buffer);
        
        char *type = strtok(temp, ";");
        char *ip = strtok(NULL, ";");
        
        if (type && ip) {
            int idx = get_host_index(ip);
            if (idx != -1) {
                pthread_mutex_lock(&lock);
                if (strcmp(type, "MEM") == 0) {
                    hosts[idx].mem_used = atof(strtok(NULL, ";"));
                    hosts[idx].mem_free = atof(strtok(NULL, ";"));
                    // Leer el resto para limpiar buffer aunque no lo mostremos todo en la tabla simple
                } else if (strcmp(type, "CPU") == 0) {
                    hosts[idx].cpu_usage = atof(strtok(NULL, ";"));
                    hosts[idx].cpu_user = atof(strtok(NULL, ";"));
                    hosts[idx].cpu_sys = atof(strtok(NULL, ";"));
                    hosts[idx].cpu_idle = atof(strtok(NULL, ";"));
                }
                pthread_mutex_unlock(&lock);
            }
        }
    }
    close(sock);
    return 0;
}

// --- Main ---
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <puerto>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);

    // 1. Iniciar hilo visualizador
    pthread_t display_thread;
    if (pthread_create(&display_thread, NULL, display_thread_func, NULL) < 0) {
        perror("Error creando hilo visualizador"); return 1;
    }

    // 2. Configurar Server
    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server, client;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind falló"); return 1;
    }
    listen(server_sock, 4);

    // 3. Aceptar clientes
    int c = sizeof(struct sockaddr_in);
    while ((client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t*)&c))) {
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;
        
        if (pthread_create(&sniffer_thread, NULL, client_handler, (void*)new_sock) < 0) {
            perror("Error hilo cliente");
            return 1;
        }
        pthread_detach(sniffer_thread);
    }
    return 0;
}
