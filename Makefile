# --- CONFIGURACIÓN FIJA ---
IP = 3.138.107.123
PORT = 6000

# --- CONFIGURACIÓN DINÁMICA ---
# Si no escribes nada, usará "Agente_Kali" por defecto.
# Puedes cambiarlo al ejecutar: make run NAME=MiNombre
NAME ?= Agente_Kali

# --- REGLAS ---

# 1. Compilar todo (opcional, pero útil)
all:
	gcc agent_mem.c -o agent_mem
	gcc agent_cpu.c -o agent_cpu

# 2. Ejecutar los dos agentes simultáneamente
# El '&' manda al agente de memoria al fondo para que el de CPU corra también
run: all
	@echo "--- Iniciando agentes para: $(NAME) ---"
	@echo "Conectando a $(IP):$(PORT)"
	./agent_mem $(IP) $(PORT) "$(NAME)" & ./agent_cpu $(IP) $(PORT) "$(NAME)"

# 3. Detener los agentes (IMPORTANTE)
# Como uno queda en background, Ctrl+C a veces no mata los dos. Usa esto para limpiar.
stop:
	@echo "Matando procesos de agentes..."
	-pkill -f agent_mem
	-pkill -f agent_cpu
