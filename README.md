# Sistema Paralelo de Procesamiento de Imágenes (OpenMP)

> **Cómputo Paralelo y Distribuido — Proyecto Parcial 1**  
> Implementación de filtros digitales en memoria compartida (C + OpenMP), medición experimental de la Ley de Amdahl y suite de análisis automatizado.

---

## Características del Proyecto

* **Pipeline Convolucional Completo:**
  1. Conversión de color a Escala de Grises (Fórmula ITU-R BT.601).
  2. Desenfoque Gaussiano 5x5 ($273$ factor normalizador).
  3. Detección de Bordes con Operador Sobel ($G = \sqrt{G_x^2 + G_y^2}$).
* **Paralelismo en Memoria Compartida (OpenMP):**
  - **Modo Lote (`--mode batch`):** Reparte equitativamente lotes de imágenes entre hilos con balanceo dinámico.
  - **Modo Píxeles (`--mode pixel`):** Divide la matriz bidimensional de píxeles entre los hilos para imágenes de alta resolución.
* **Cero dependencias externas:** Utiliza `stb_image` y `stb_image_write` incluidas directamente en el proyecto. No requiere instalar OpenCV ni librerías complejas.
* **Suite de Benchmarking (Python):** Automatiza 3 corridas con 1, 2, 4 y 8 hilos, calcula aceleración ($S$), eficiencia ($E$), fracción secuencial de Amdahl ($f$) y genera gráficas de publicación en alta resolución.

---

## Guía de Instalación y Compilación

### 1. Requisitos Previos

Dependiendo de tu sistema operativo, asegúrate de tener las herramientas necesarias instaladas:

**Para Linux / macOS:**
* Compilador C con soporte OpenMP (`gcc` o `clang` con `libomp`)
* Herramienta `make`
* Python 3 y pip

**Para Windows:**
* Entorno MinGW-w64 (como WinLibs) que incluya `gcc` y soporte de POSIX threads. Se puede instalar vía WinGet: `winget install BrechtSanders.WinLibs.POSIX.UCRT`
* Herramienta `mingw32-make` (incluida generalmente con MinGW)
* Python 3 y pip

### 2. Dependencias de Python
Para generar gráficas de benchmarking, instala los paquetes requeridos:
```bash
pip install -r scripts/requirements.txt
```

### 3. Compilación

Para compilar el proyecto, abre tu terminal en la raíz del repositorio y ejecuta:

**Linux / macOS:**
```bash
make
```

**Windows:**
```powershell
mingw32-make
```

El binario resultante se encontrará en la carpeta `bin/` (`bin/img_processor` en Linux/Mac o `bin\img_processor.exe` en Windows).

---

## Ejecución Básica

### 1. Generar Imágenes de Prueba
Si aún no has descargado el dataset completo, puedes generar radiografías sintéticas de prueba:
```bash
python scripts/generate_samples.py -n 20
```

### 2. Ejecución Manual
Asegúrate de usar la ruta correcta según tu sistema operativo (`./bin/img_processor` en Linux/macOS, `.\bin\img_processor.exe` en Windows):

```bash
# Procesar lote con 4 hilos
./bin/img_processor -i data/input -o data/output -t 4 --mode batch

# Procesar una sola imagen con 8 hilos (modo píxel)
./bin/img_processor -i data/input/sample_xray_001.png -o data/output/res.png -t 8 --mode pixel
```

### 3. Ejecutar Benchmark Automatizado
El script de benchmark correrá las pruebas y generará gráficas actualizadas. 
**Nota en Windows:** Es necesario pasar la ruta exacta del ejecutable con extensión `.exe` al script.

**Linux / macOS:**
```bash
python scripts/benchmark.py --runs 3
```

**Windows:**
```powershell
python scripts/benchmark.py --runs 3 --bin bin/img_processor.exe
```

Las gráficas resultantes se guardarán en `docs/figures/`.

---

## Estructura del Repositorio

```text
.
├── Makefile                      # Reglas de compilacion (make, make test, make clean)
├── README.md                     # Guia de usuario y despliegue
├── data/
│   ├── input/                    # Carpeta para colocar radiografias originales
│   └── output/                   # Carpeta de imagenes procesadas
├── docs/
│   ├── reporte_tecnico.md        # Reporte formal con las 6 secciones (a-f)
│   ├── benchmark_results.json    # Datos numericos de las pruebas
│   └── figures/                  # Graficas de Speedup, Eficiencia y Tiempos
├── scripts/
│   ├── benchmark.py              # Suite de medicion y generador de graficas
│   ├── generate_samples.py       # Generador de radiografias sinteticas
│   └── requirements.txt          # Dependencias de Python
└── src/
    ├── filters.h / filters.c     # Implementacion matematica de filtros
    ├── main.c                    # Punto de entrada y CLI
    ├── pipeline.h / pipeline.c   # Logica de procesamiento en lote y pixeles
    ├── timer.h / timer.c         # Medicion precisa de tiempos y sync
    └── vendor/                   # Cabeceras libres stb_image
```

---

## Organización del Equipo (4 Personas)

| Rol | Responsable | Área Principal de Trabajo |
| :--- | :--- | :--- |
| **1. Algoritmos & Convoluciones** | [Persona 1] | `src/filters.c` — Optimización y validación matemática de filtros. |
| **2. Concurrencia & Memoria** | [Persona 2] | `src/pipeline.c` — Balanceo OpenMP, barreras y control de contención. |
| **3. Benchmarking & Métricas** | [Persona 3] | `scripts/benchmark.py` — Pruebas con dataset masivo y Ley de Amdahl. |
| **4. Integración & Reporte** | [Persona 4] | `docs/reporte_tecnico.md` — Redacción final, diagramas y entrega. |
