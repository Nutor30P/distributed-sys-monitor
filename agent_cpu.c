#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Estructura de datos crudos
typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
} CpuRawData;

// Función de lectura
int get_cpu_raw_data(CpuRawData *data) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;
    char line[256];
    if (fgets(line, sizeof(line), f)) {
        sscanf(line, "cpu  %lu %lu %lu %lu %lu %lu %lu %lu",
               &data->user, &data->nice, &data->system, &data->idle,
               &data->iowait, &data->irq, &data->softirq, &data->steal);
    }
    fclose(f);
    return 0;
}

int main(int argc, char *argv[]) {
    // 1. Validar argumentos
    if (argc != 4) {
        printf("Uso: %s <ip_recolector> <puerto> <ip_logica_agente>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *agent_name = argv[3];

    // 2. Conectar al servidor
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Error Socket"); return 1; }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("IP Inválida"); return 1;
    }

    printf("Conectando CPU Agent a %s:%d...\n", server_ip, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error Connect");
        return 1;
    }
    printf("Conectado exitosamente.\n");

    // 3. Inicializar primera lectura
    CpuRawData prev, curr;
    if (get_cpu_raw_data(&prev) != 0) return 1;

    char buffer[256];

    // 4. Bucle principal
    while (1) {
        sleep(1); // Intervalo de muestreo
        
        if (get_cpu_raw_data(&curr) != 0) break;

        // Cálculos de Deltas
        unsigned long prev_total = prev.user + prev.nice + prev.system + prev.idle + 
                                   prev.iowait + prev.irq + prev.softirq + prev.steal;
        unsigned long curr_total = curr.user + curr.nice + curr.system + curr.idle + 
                                   curr.iowait + curr.irq + curr.softirq + curr.steal;

        unsigned long total_delta = curr_total - prev_total;
        unsigned long idle_delta  = curr.idle - prev.idle;
        unsigned long user_delta  = curr.user - prev.user;
        unsigned long sys_delta   = curr.system - prev.system;

        if (total_delta == 0) total_delta = 1; // Evitar div/0

        float cpu_usage = (float)(total_delta - idle_delta) / total_delta * 100.0;
        float user_pct  = (float)user_delta / total_delta * 100.0;
        float sys_pct   = (float)sys_delta / total_delta * 100.0;
        float idle_pct  = (float)idle_delta / total_delta * 100.0;

        // Formato según PDF 
        // CPU;<ip_logica_agente>; <CPU_usage>; <user_pct>; <system_pct>; <idle_pct>\n
        snprintf(buffer, sizeof(buffer), "CPU;%s;%.2f;%.2f;%.2f;%.2f\n",
                 agent_name, cpu_usage, user_pct, sys_pct, idle_pct);

        // Enviar
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("Error enviando");
            break;
        }
        printf("Enviado: %s", buffer);

        // Actualizar estado previo para la siguiente vuelta
        prev = curr;
    }

    close(sock);
    return 0;
}
