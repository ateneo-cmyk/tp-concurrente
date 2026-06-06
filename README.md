##  Descripción del Proyecto
Este sistema simula una granja de renderizado en la nube utilizando programacion concurrente en **C++**.esta diseñado bajo un modelo de alta disponibilidad capaz de admitir multiples nodos de entrada (*API Gateways*) y multiples nodos de procesamiento (*Worker Nodes*) operando simultaneamente sobre recursos compartidos.

---

##  Arquitectura y Patrones de Diseño
El desarrollo se estructuró de forma estrictamente modular aplicando los siguientes patrones de software:

1. **Pipes and Filters (Tuberías y Filtros):** Los componentes  actuan como filtros independientes. Los datos (instancias de `Job`) fluyen secuencialmente a traves de tuberías (buffers de memoria compartidos), lo que desacopla la generación de tareas de su procesamiento físico.
2. **Productor-Consumidor:** * **Productores (API Gateways):** Hilos concurrentes que simulan el ingreso de peticiones. Generan IDs globales unificados, determinan prioridades aleatorias (Premium/Free) y marcan cronológicamente el tiempo de creacion.
   * **Consumidores (Worker Nodes):** Hilos concurrentes que drenan el buffer principal, aplican politicas de equidad y simulan la carga de procesamiento pesado en las GPUs.

---

## 🛠️ Mecanismos de Concurrencia y Sincronización
se implementaron tecnicas de exclusión mutua:

* **Exclusión Mutua Explícita (`.lock()` / `.unlock()`):** La protección de recursos críticos (Buffer 1, ID global, memoria VRAM y estadísticas) se realiza mediante llamadas explícitas manuales sobre instancias dedicadas de mutex
* **Semáforos Pasivos:** Se utiliza una primitiva `Semaforo` (basada en `std::condition_variable`) para coordinar la sincronizacion de hilos. El consumidor realiza una espera pasiva cuando el sistema está vacío, eliminando por completo el consumo inútil de recursos de CPU (*Busy Waiting*).
* **Optimización del Buffer 2 (Pool de VRAM):** La memoria de video activa es un arreglo estatico de 5 posiciones. Para maximizar el rendimiento y evitar bloqueos cruzados (*Deadlocks* o *Livelocks*), se aislaron las fases mediante dos candados independientes:
  * `mtx_asignacion`: Controla el cuello de botella secuencial de entrada (450ms).
  * `mtx_liberacion`: Controla el cuello de botella secuencial de salida (250ms).
  Esto permite que la fase intermedia de renderizado puro en GPU (600ms) se ejecute de forma 100% concurrente por hasta 5 hilos en paralelo.

---

##  Algoritmo de Equidad (Anti-Starvation)
Para resolver la **Inanición (Starvation)** de tareas de baja prioridad (Free) frente al monopolio de tareas de alta prioridad (Premium), el **Buffer 1** se construyó utilizando un `std::vector<Job>` dinamico. 

Antes de tomar una tarea, el consumidor ejecuta una función analítica de envejecimiento (`sufreInanicion`). El hilo recorre el vector completo inspeccionando el `timestamp_creacion` de los Jobs Free. Si una tarea Free supera los **5000ms** de espera en la cola, el despachador interrumpe el orden de prioridades estándar, la rescata inmediatamente y la envía a renderizar, garantizando un entorno de procesamiento justo.

---

## Estructura del Código Fuente
El proyecto se encuentra modularizado bajo buenas prácticas de ingeniería de software para facilitar su mantenimiento y auditoría:
* `main.cpp`: Punto de entrada del sistema. Controla la interfaz de usuario y el menú de pruebas. No posee lógica.
* `sistema_render.h/.cpp`: Contiene el core concurrente. Orquesta los hilos de los productores, consumidores y la memoria de VRAM.
* `semaforo.h/.cpp`: Implementación de la primitiva de sincronización por semáforos y variables de condición.
* `logger.h/.cpp`: Módulo de logging persistente. Registra eventos de forma atómica en consola y en el archivo `sistema.log`.

---

## Instrucciones de Ejecución y Pruebas
El sistema cuenta con un menú interactivo en consola para evaluar los escenarios de estrés obligatorios solicitados:

1. **Configuración C (Concurrencia Simétrica):** Lanza 3 Productores y 3 Consumidores procesando un flujo estable (15 tareas c/u). Demuestra un cierre de programa elegante mediante `.join()` sincrónico al vaciarse el sistema.
2. **Prueba de Carga Masiva:** Un solo API Gateway inyecta un bloque de 1500 tareas simultáneas para verificar la resistencia de los mutex y la ausencia de condiciones de carrera en los contadores globales.
3. **Prueba de Vacuidad:** Inicia el sistema con 0 tareas para comprobar que los hilos consumidores permanecen bloqueados de forma segura en modo durmiente sin consumir CPU.

### Compilación (Code::Blocks / GCC)
1. Abrir el archivo de proyecto `tpconcurrente.cbp` en Code::Blocks.
2. Asegurarse de tener habilitado el estándar de C++ mínimo (C++11 o superior).
3. Presionar `Build and Run` (F9).
4. El historial detallado de estados (`CREADO`, `EN_COLA`, `RESCATE_INANICION`, `ASIGNADO_VRAM`, `FINALIZADO`) se visualiza en la consola y se guarda automáticamente en el archivo local `sistema.log`.
