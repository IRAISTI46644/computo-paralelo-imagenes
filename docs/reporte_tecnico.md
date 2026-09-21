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

El modelo computacional tradicional (secuencial de Von Neumann) ejecuta una única instrucción a la vez sobre un único procesador, calculando el tiempo total como la sumatoria lineal de todas las operaciones. En problemas de procesamiento masivo de datos —como el preprocesamiento de colecciones médicas de radiografías— la linealidad se convierte en un cuello de botella inasumible.

Para romper esta linealidad, este proyecto implementa un modelo de **Memoria Compartida** (fundamentado teóricamente en la arquitectura **PRAM - Parallel Random Access Machine**, donde múltiples unidades de procesamiento acceden a un espacio de direcciones de memoria uniforme) para aplicar un pipeline de visión computacional y filtrado digital:

1. **Conversión a Escala de Grises (Luminancia):** Transformación de canales de color mediante la ponderación perceptiva ITU-R BT.601:
   $$Y = 0.299R + 0.587G + 0.114B$$
2. **Desenfoque Gaussiano (Filtro 5x5):** Suavizado y eliminación de ruido de alta frecuencia mediante una convolución bidimensional con una matriz de ponderación Gaussiana simétrica (factor de normalización = 273):
   $$\mathbf{K}_{gauss} = \frac{1}{273} \begin{bmatrix}
   1 & 4 & 7 & 4 & 1 \\
   4 & 16 & 26 & 16 & 4 \\
   7 & 26 & 41 & 26 & 7 \\
   4 & 16 & 26 & 16 & 4 \\
   1 & 4 & 7 & 4 & 1
   \end{bmatrix}$$
3. **Detección de Bordes Sobel:** Estimación del gradiente espacial bidimensional ($G_x$ y $G_y$) y cálculo de la magnitud del gradiente para aislar contornos patológicos y anatómicos:
   $$G = \min\left(255, \sqrt{G_x^2 + G_y^2}\right)$$

---

## b. Dataset Utilizado

* **Dataset Seleccionado:** *Chest X-Ray Images - Pneumonia* (Kaggle).
* **Volumen Evaluado:** **624 radiografías de tórax reales** del conjunto de prueba (*Test Set*), correspondientes a casos normales y patológicos (neumonía bacteriana y viral).
* **Heterogeneidad de los Datos:** Las imágenes poseen resoluciones variables (oscilando entre $1024 \times 1024$ y más de $2000 \times 2000$ píxeles), lo que introduce **asimetría computacional** en el tiempo de procesamiento de cada elemento.
* **Cero Dependencias Externas:** Integración directa mediante cabeceras monolíticas `stb_image.h` y `stb_image_write.h` compiladas de forma nativa en C99.

---

## c. Estrategia de Paralelización y Ciclo de Ejecución

Siguiendo el ciclo canónico de la computación paralela visto en clase:  
$$\text{\textbf{Repartir (Scatter)}} \longrightarrow \text{\textbf{Calcular (Workers)}} \longrightarrow \text{\textbf{Recolectar (Gather)}}$$

El sistema fue desarrollado en C con OpenMP en dos niveles de granularidad:

### 1. Nivel de Lote: Reparto Asimétrico y Balanceo Dinámico
* **El Problema de la Simetría:** En un reparto estático homogéneo (equivalente a un `MPI_Scatter` rígido), si un hilo recibe radiografías de $2000 \times 2000$ píxeles y otro de $800 \times 800$, los hilos más rápidos caen en tiempo ocioso (*idle time* / inanición) esperando en la barrera final.
* **Solución (Scatter Dinámico):** Se implementó `#pragma omp parallel for schedule(dynamic, 1)`. Las 624 imágenes forman una cola de trabajo compartida. Cuando un hilo termina su tarea, solicita de inmediato el siguiente archivo disponible. Esto emula un reparto heterogéneo adaptativo (equivalente conceptual a la flexibilidad de un `MPI_Scatterv` con desplazamientos y tamaños variables).
* **Recolección (Gather):** Al completar el lote, una barrera implícita (`#pragma omp barrier`) garantiza la recolección ordenada de todas las salidas en el directorio de destino sin condiciones de carrera.

### 2. Nivel de Matriz de Píxeles (Grano Fino)
* Para imágenes individuales gigantes, el hilo maestro divide la matriz de filas ($y \in [0, H)$) de manera contigua entre los $P$ hilos.
* Para garantizar que no existan carreras críticas (*data races*), las etapas del pipeline operan en buffers desacoplados: la lectura de vecindad para Gauss y Sobel se realiza sobre matrices de solo lectura, escribiendo exclusivamente en el bloque de filas asignado del buffer destino.

---

## d. Tiempos de Ejecución y Mediciones Experimentales

Las pruebas se ejecutaron bajo un protocolo de **3 corridas mínimas** por configuración ($p \in \{1, 2, 4, 8\}$) sobre las **624 radiografías reales**. Se instrumentaron marcas de tiempo de alta resolución con `omp_get_wtime()` para aislar las fases:
* **Tiempo Total ($T_{total}$):** Tiempo global de pared (*wall-clock time*).
* **Tiempo de Cómputo Puro ($T_{comp}$):** Cálculo convolucional en memoria RAM.
* **Tiempo de E/S ($T_{io}$):** Decodificación y guardado de archivos en disco.
* **Tiempo de Sincronización y Overhead ($T_{sync}$):** Tiempo dedicado a la creación de hilos, esperas en barreras y contención del despachador.

### Tabla de Resultados Experimentales (Promedio de 3 corridas ± Desviación Estándar)

| Hilos ($p$) | T. Total (s) | T. Cómputo (s) | T. E/S (s) | T. Sincronización (s) | Speedup Cómputo ($S$) | Eficiencia ($E$) | Fracción Sec. ($f$) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **1** | 43.0413 ± 0.222 | 8.8249 | 34.2046 | 0.011867 | **1.00x** | **100.0%** | — |
| **2** | 22.3102 ± 0.149 | 4.5597 | 17.7035 | 0.046945 | **1.94x** | **96.8%** | 0.0334 |
| **4** | 11.7046 ± 0.035 | 2.3994 | 9.2569 | 0.048327 | **3.68x** | **91.9%** | 0.0292 |
| **8** | 6.2481 ± 0.040 | 1.2746 | 4.9184 | 0.055066 | **6.92x** | **86.5%** | 0.0222 |

---

## e. Medición de Aceleración (Speedup) y Eficiencia

### 1. Aceleración (Speedup)
La aceleración observada mide la ganancia respecto al caso monohilo:
$$S(p) = \frac{T_1}{T_p}$$

* **Con 2 hilos:** $S(2) = 1.94\text{x}$ (96.8% de la aceleración lineal teórica).
* **Con 4 hilos:** $S(4) = 3.68\text{x}$ (91.9% de la aceleración lineal teórica).
* **Con 8 hilos:** $S(8) = 6.92\text{x}$ (86.5% de la aceleración lineal teórica).
* **Aceleración global de la aplicación:** Reducción del tiempo de 43.04 s a 6.25 s (**6.89x de ganancia total**).

![Gráfica de Speedup](figures/speedup.png)

### 2. Eficiencia Paralela
La eficiencia mide el porcentaje de aprovechamiento efectivo de los núcleos:
$$E(p) = \frac{S(p)}{p} \times 100\%$$

* La eficiencia se mantiene por encima del **86.5%** incluso al saturar los 8 hilos, confirmando que la sobrecarga de sincronización fue mínima frente al cómputo realizado.

![Gráfica de Eficiencia](figures/eficiencia.png)

---

## f. Fracción Secuencial, Ley de Amdahl y Análisis del Cuello de Botella

### 1. Ley de Amdahl
La Ley de Amdahl establece el límite máximo de aceleración alcanzable en función de la fracción secuencial intrínseca ($f$):
$$S(p) = \frac{1}{f + \frac{1 - f}{p}}$$

Despejando la fracción secuencial experimental a partir de los datos observados:
$$f = \frac{\frac{1}{S(p)} - \frac{1}{p}}{1 - \frac{1}{p}}$$

* Se obtuvo una **fracción secuencial experimental promedio de $f \approx 0.0283$ ($2.83\%$)**.
* Esto confirma empíricamente que el **$97.17\%$** del pipeline de procesamiento de imágenes es estrictamente paralelizable.

### 2. Análisis del Cuello de Botella y Tiempo de Sincronización
* **El Cuello de Botella de la E/S y la Raíz:** Tal como se analizó en clase respecto a la saturación del proceso raíz en operaciones colectivas (`MPI_Gather`), al incrementar el número de procesadores concurrentes, la fase de sincronización y de acceso al bus de E/S crece:
  * Con 1 hilo: $T_{sync} \approx 11.8 \text{ ms}$.
  * Con 8 hilos: $T_{sync} \approx 55.0 \text{ ms}$.
* La gráfica de barras evidencia que la porción de cómputo puro disminuye de forma drástica (de 8.82 s a 1.27 s), pero la E/S de almacenamiento domina el tiempo restante, demostrando la ley de rendimientos decrecientes de Amdahl.

![Desglose de Tiempos](figures/tiempos_desglose.png)

---

## g. Discusión Teórica: Memoria Compartida (OpenMP) vs. Memoria Distribuida (MPI)

Con base en los conceptos revisados en las sesiones de clase:

| Dimensión | Enfoque Implementado: Memoria Compartida (OpenMP) | Alternativa Distribuida: Memoria Distribuida (MPI) |
| :--- | :--- | :--- |
| **Espacio de Memoria** | **PRAM uniforme:** Todos los hilos acceden al mismo espacio de direcciones virtual. Cero costo de copiado. | **RAM privada aislada:** Cada proceso vive en su propio espacio. Requiere paso de mensajes explícito. |
| **Fase de Reparto** | Despacho dinámico por punteros compartidos (`schedule(dynamic)`). | Requiere empaquetar buffers y usar `MPI_Scatterv` con listas de desplazamientos (`displs`). |
| **Fase de Recolección** | Los hilos escriben directamente en el buffer de salida o archivos en disco con sincronización por barrera. | Operación colectiva y bloqueante `MPI_Gather` o `MPI_Gatherv`, sufriendo memoria asimétrica $O(P)$ en el nodo raíz. |
| **Costos y Sobrecarga** | Limitado al ancho de banda del bus de memoria local (RAM/disco). | Sobrecarga de red, latencia de sockets/interconexión y serialización de datos. |

### Conclusión Final
La elección de un esquema de memoria compartida con OpenMP para este problema local permitió eliminar la sobrecarga de serialización y transferencia por red que sufriría una solución con `MPI_Scatter`/`MPI_Gather`. El balanceo de carga dinámico mitigó la asimetría natural del dataset de radiografías, alcanzando un factor de aceleración de 6.92x y validando la predicción teórica de la Ley de Amdahl con una fracción secuencial de solo 2.83%.
