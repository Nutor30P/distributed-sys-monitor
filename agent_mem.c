#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // Librería para sockets

// --- 1. Lógica de Métricas (Lo que ya probamos) ---

typedef struct {
    long mem_total;
    long mem_available;
    long swap_total;
    long swap_free;
} MemoryMetrics;

int get_memory_metrics(MemoryMetrics *m) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1;

    char line[256];
    // Reiniciamos valores
    m->mem_total = 0; m->mem_available = 0; m->swap_total = 0; m->swap_free = 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %ld kB", &m->mem_total) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &m->mem_available) == 1) continue;
        if (sscanf(line, "SwapTotal: %ld kB", &m->swap_total) == 1) continue;
        if (sscanf(line, "SwapFree: %ld kB", &m->swap_free) == 1) continue;
    }
    fclose(f);
    return 0;
}

// --- 2. Lógica de Red y Principal ---

int main(int argc, char *argv[]) {
    // Validación de argumentos según PDF [cite: 34]
    if (argc != 4) {
        printf("Uso: %s <ip_recolector> <puerto> <ip_logica_agente>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *agent_name = argv[3];

    // Crear el socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        return 1;
    }

    // Configurar la dirección del servidor
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    // Convertir IP string a binario
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Dirección IP inválida");
        return 1;
    }

    // Conectar
    printf("Conectando a %s:%d...\n", server_ip, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error de conexión (¿Está encendido el servidor?)");
        return 1;
    }
    printf("Conectado exitosamente.\n");

    // Bucle principal de envío
    MemoryMetrics m;
    char buffer[256];

    while (1) {
        if (get_memory_metrics(&m) == 0) {
            // Cálculos
            float mem_used = (m.mem_total - m.mem_available) / 1024.0;
            float mem_free = m.mem_available / 1024.0;
            float swap_total = m.swap_total / 1024.0;
            float swap_free = m.swap_free / 1024.0;

            // Formato CSV requerido por PDF [cite: 38]
            // MEM;<ip logica agente>; <mem_used MB>; <MemFree MB>; <SwapTotal MB>; <SwapFree MB>\n
            snprintf(buffer, sizeof(buffer), "MEM;%s;%.2f;%.2f;%.2f;%.2f\n",
                     agent_name, mem_used, mem_free, swap_total, swap_free);

            // Enviar por el socket
            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                perror("Error enviando datos");
                break;
            }
            printf("Enviado: %s", buffer); // Log local para ver qué pasa
        }
        
        sleep(2); // Esperar 2 segundos antes del siguiente envío [cite: 24]
    }

    close(sock);
    return 0;
}
