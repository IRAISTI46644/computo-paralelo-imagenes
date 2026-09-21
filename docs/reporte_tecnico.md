# Reporte Técnico: Aceleración y Eficiencia en Procesamiento Digital de Imágenes (Ley de Amdahl)

**Materia:** Cómputo Paralelo y Distribuido  
**Proyecto:** Parcial 1  
**Integrantes del Equipo:**  
- Christian (VonLuna)
- [Nombre Compañero 2]
- [Nombre Compañero 3]
- [Nombre Compañero 4]

---

## a. Explicación del Problema

El procesamiento digital de imágenes sobre grandes volúmenes de datos (datasets médicos, satelitales o científicos) requiere una alta capacidad de cómputo secuencial cuando se aplican transformaciones convolucionales. 

En este proyecto se implementó un sistema de cómputo en memoria compartida para aplicar de manera acelerada un pipeline estándar de preprocesamiento de imágenes:
1. **Conversión a Escala de Grises (Luminancia):** Reducción de dimensionalidad de color a intensidades mediante la fórmula ponderada $Y = 0.299R + 0.587G + 0.114B$.
2. **Desenfoque Gaussiano (Filtro 5x5):** Suavizado espacial y mitigación de ruido aleatorio mediante convolución bidimensional con una matriz de ponderación Gaussiana (suma = 273).
3. **Detección de Bordes Sobel:** Cálculo del gradiente espacial bidimensional ($G_x$ y $G_y$) y aproximación de la magnitud del gradiente $G = \sqrt{G_x^2 + G_y^2}$ para extraer características anatómicas o contornos.

---

## b. Dataset Utilizado

* **Dataset Base:** *Chest X-Ray Images - Pneumonia* (Radiografías de tórax de pacientes sanos y con neumonía).
* **Características:** Imágenes en formato PNG/JPEG con resoluciones variables (típicamente entre $1024 \times 1024$ y $2048 \times 2048$ píxeles).
* **Generador Sintético Complementario:** Se integró además una suite generadora (`scripts/generate_samples.py`) para producir lotes sintéticos reproducibles con ruido gaussiano, estructuras anatómicas cilíndricas y matrices de alta densidad de píxeles.

---

## c. Estrategia de Paralelización

Se diseñó una arquitectura de **Memoria Compartida** implementada en lenguaje **C** con **OpenMP**, estructurada en dos niveles de granularidad:

### 1. Nivel de Lote (Batch - Grano Grueso)
* El hilo maestro lee los metadatos del directorio de entrada y construye un vector compartido de rutas de archivos en memoria.
* Mediante `#pragma omp parallel for schedule(dynamic, 1)`, el lote de imágenes se despacha dinámicamente entre los $P$ hilos trabajadores. Cada trabajador carga, procesa y almacena las imágenes de forma concurrente, minimizando la contención.
* Una barrera de sincronización implícita (`#pragma omp barrier`) garantiza la recolección total de las imágenes procesadas.

### 2. Nivel de Matriz de Píxeles (Pixel - Grano Fino)
* Para imágenes individuales de gran resolución, cada filtro convolucional reparte las filas de la matriz ($y \in [0, H)$) entre los hilos OpenMP.
* El acceso a memoria es seguro ante condiciones de carrera (*race conditions*) debido a que las operaciones de lectura (búfer fuente) y escritura (búfer destino) se realizan en matrices desacopladas con sincronización de barrera entre etapas.

---

## d. Tiempos de Ejecución y Mediciones Experimentales

Las pruebas se realizaron bajo un protocolo de **3 corridas mínimas** por cada número de hilos ($p \in \{1, 2, 4, 8\}$). Se midieron con precisión de microsegundos (`omp_get_wtime()`) los siguientes componentes:
* **Tiempo Puro de Cómputo ($T_{comp}$):** Transformación matemática y convoluciones.
* **Tiempo de E/S ($T_{io}$):** Lectura y codificación de imágenes en disco.
* **Tiempo de Sincronización ($T_{sync}$):** Overhead de creación de hilos, barreras y desbalance de carga.

### Tabla de Resultados Experimentales

| Hilos ($p$) | T. Total (s) | T. Cómputo (s) | T. E/S (s) | T. Sincronización (s) | Speedup ($S_p$) | Eficiencia ($E_p$) | Fracción Sec. ($f$) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **1** | 0.6097 ± 0.002 | 0.1125 | 0.4971 | 0.000087 | **1.00x** | **100.0%** | — |
| **2** | 0.3148 ± 0.000 | 0.0569 | 0.2554 | 0.002448 | **1.98x** | **98.8%** | 0.0120 |
| **4** | 0.1677 ± 0.002 | 0.0294 | 0.1336 | 0.004732 | **3.83x** | **95.7%** | 0.0151 |
| **8** | 0.1102 ± 0.002 | 0.0151 | 0.0682 | 0.026880 | **7.44x** | **93.0%** | 0.0108 |

---

## e. Medición de Aceleración (Speedup) y Eficiencia

### 1. Aceleración (Speedup)
La aceleración observada se define como:
$$S(p) = \frac{T_1}{T_p}$$

* Para 2 hilos se obtuvo un Speedup de **1.98x** (99% del ideal).
* Para 4 hilos se obtuvo un Speedup de **3.83x** (95.7% del ideal).
* Para 8 hilos se obtuvo un Speedup de **7.44x** (93.0% del ideal).

![Gráfica de Speedup](figures/speedup.png)

### 2. Eficiencia
La eficiencia evalúa el aprovechamiento del hardware respecto al número de núcleos dedicados:
$$E(p) = \frac{S(p)}{p} \times 100\%$$

* La eficiencia se mantiene por encima del **93%** incluso al escalar a 8 hilos, lo que demuestra un excelente balanceo de carga y baja contención en el acceso a memoria.

![Gráfica de Eficiencia](figures/eficiencia.png)

---

## f. Fracción Secuencial y Ley de Amdahl

### 1. Modelado Teórico de la Ley de Amdahl
La Ley de Amdahl establece el límite superior del speedup teórico de un algoritmo en función de su fracción secuencial ($f$):
$$S(p) = \frac{1}{f + \frac{1 - f}{p}}$$

Despejando la fracción secuencial experimental para cada configuración concurrente:
$$f = \frac{\frac{1}{S(p)} - \frac{1}{p}}{1 - \frac{1}{p}}$$

A partir de las mediciones se identificó una **fracción secuencial promedio de $f \approx 0.0126$ (1.26%)** en la fase de cómputo puro.

### 2. Tiempo de Sincronización y Overhead
Conforme incrementa el número de hilos a 8, el tiempo de sincronización aumenta de $0.08 \text{ ms}$ a $26.88 \text{ ms}$. Esto se atribuye al costo de sincronización de la barrera final de OpenMP y el acceso concurrente al subsistema de archivos en disco.

![Desglose de Tiempos](figures/tiempos_desglose.png)

### 3. Conclusión
El pipeline convolucional implementado en C con OpenMP demostró un comportamiento altamente paralelizable con una fracción secuencial mínima (1.26%), logrando una aceleración casi lineal (7.44x con 8 hilos). El cuello de botella residual radica primordialmente en las operaciones de E/S de disco, las cuales pueden mitigarse en sistemas distribuidos mediante pipelines asíncronos de prefetching.
