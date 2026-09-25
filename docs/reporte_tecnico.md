# Reporte Técnico: Aceleración y Eficiencia en Procesamiento Digital de Imágenes (Ley de Amdahl)

**Materia:** Cómputo Paralelo y Distribuido  
**Proyecto:** Parcial 1  
**Integrantes del Equipo:**  
- Christian Luna Ortiz
- Misael Reynoso Aguayo
- Camila Serrano Trinidad
- Irais Gisela Macuil Chapuli

---

## a. Explicación del Problema

El modelo computacional tradicional de Von Neumann ejecuta instrucciones de manera secuencial. Por consiguiente, el tiempo total resulta ser la suma lineal de las operaciones. Al procesar volúmenes masivos de datos —como colecciones médicas de radiografías— la linealidad constituye un cuello de botella inasumible.

Para mitigar esta limitación, el presente sistema emplea un modelo de **Memoria Compartida**. Se fundamenta en la arquitectura **PRAM (Parallel Random Access Machine)**, donde múltiples unidades de procesamiento acceden a un espacio de direcciones uniforme. Bajo este paradigma, se aplica un pipeline de visión computacional y filtrado digital compuesto por tres etapas:

1. **Conversión a Escala de Grises (Luminancia):** Transformación de canales de color mediante la ponderación perceptiva ITU-R BT.601:
   $$Y = 0.299R + 0.587G + 0.114B$$
2. **Desenfoque Gaussiano (Filtro 5x5):** Suavizado espacial y eliminación de ruido de alta frecuencia a través de una convolución con un factor de normalización de 273.
3. **Detección de Bordes Sobel:** Estimación del gradiente bidimensional ($G_x$ y $G_y$) para aislar contornos patológicos y anatómicos:
   $$G = \min\left(255, \sqrt{G_x^2 + G_y^2}\right)$$

---

## b. Dataset Utilizado y Entornos de Pruebas

Las pruebas experimentales se condujeron en dos entornos de sistemas operativos diferentes para contrastar el comportamiento del planificador (*scheduler*) y las bibliotecas del sistema. En efecto, la heterogeneidad de los datos y el hardware introducen asimetría computacional.

* **Entorno 1 (Linux/macOS - Christian):** Se procesaron **624 radiografías de tórax reales** extraídas del dataset *Chest X-Ray Images - Pneumonia* de Kaggle. Las imágenes varían en resoluciones desde $1024 \times 1024$ hasta más de $2000 \times 2000$ píxeles.
* **Entorno 2 (Windows - Misael):** Se procesaron **20 radiografías sintéticas** de $1024 \times 1024$ píxeles para validar la ejecución cruzada del binario nativo (`img_processor.exe`) bajo MinGW y WinLibs.

Ambos entornos prescinden de dependencias externas pesadas, operando nativamente en C99 e integrando de manera monolítica `stb_image.h` y `stb_image_write.h`.

---

## c. Estrategia de Paralelización y Ciclo de Ejecución

La descomposición algorítmica siguió el ciclo canónico de computación concurrente:
$$\text{\textbf{Repartir (Scatter)}} \longrightarrow \text{\textbf{Calcular (Workers)}} \longrightarrow \text{\textbf{Recolectar (Gather)}}$$

El código distribuye la carga mediante la API de OpenMP utilizando un enfoque asimétrico dinámico. En un reparto estático estricto, un hilo asignado a imágenes pesadas rezaga a los demás generándoles inanición (*idle time*). Para resolver la asimetría, se implementó la directiva `#pragma omp parallel for schedule(dynamic, 1)`. La cola de archivos iterados permanece compartida; de este modo, cuando un hilo finaliza su tarea en curso, captura de inmediato el siguiente archivo. En consecuencia, el sistema emula un reparto adaptativo que minimiza el tiempo inactivo en la barrera implícita final (`#pragma omp barrier`).

Asimismo, las etapas convolucionales operan en *buffers* de memoria desacoplados. La lectura de vecindad para los filtros Gaussiano y Sobel se efectúa sobre matrices inmutables, anulando carreras críticas (*data races*) al escribir exclusivamente en el bloque preasignado de destino.

---

## d. Tiempos de Ejecución (3 Corridas)

La instrumentación evaluó métricas bajo $p \in \{1, 2, 4, 8\}$ hilos mediante la función `omp_get_wtime()`. Las variables de interés disgregan el tiempo total ($T_{total}$) en la duración del cómputo convolucional en CPU ($T_{comp}$), la lectura y escritura persistente en disco ($T_{io}$) y la latencia subyacente de sincronización multihilo ($T_{sync}$).

### Entorno 1: Resultados en Linux (624 Imágenes Reales)
| Hilos ($p$) | T. Total (s) | T. Cómputo (s) | T. E/S (s) | T. Sync (s) | Speedup ($S$) | Eficiencia ($E$) | Fracc. Sec. ($f$) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **1** | 43.0413 ± 0.222 | 8.8249 | 34.2046 | 0.011867 | **1.00x** | **100.0%** | — |
| **2** | 22.3102 ± 0.149 | 4.5597 | 17.7035 | 0.046945 | **1.94x** | **96.8%** | 0.0334 |
| **4** | 11.7046 ± 0.035 | 2.3994 | 9.2569 | 0.048327 | **3.68x** | **91.9%** | 0.0292 |
| **8** | 6.2481 ± 0.040 | 1.2746 | 4.9184 | 0.055066 | **6.92x** | **86.5%** | 0.0222 |
*Gráficas en `docs/figures/` (speedup, eficiencia, tiempos).*

### Entorno 2: Resultados en Windows (624 Imágenes Reales)
| Hilos ($p$) | T. Total (s) | T. Cómputo (s) | T. E/S (s) | T. Sync (s) | Speedup ($S$) | Eficiencia ($E$) | Fracc. Sec. ($f$) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **1** | 156.0977 ± 2.111 | 27.3139 | 128.6766 | 0.107158 | **1.00x** | **100.0%** | — |
| **2** | 82.7211 ± 0.387 | 15.0977 | 67.4667 | 0.156743 | **1.81x** | **90.5%** | 0.1055 |
| **4** | 50.7871 ± 0.628 | 8.8583 | 41.8103 | 0.118460 | **3.08x** | **77.1%** | 0.0991 |
| **8** | 43.7377 ± 2.039 | 6.3420 | 37.1382 | 0.257452 | **4.31x** | **53.8%** | 0.1225 |
*Gráficas en `docs/resultados_misael/figures/` (speedup, eficiencia, tiempos).*

---

## e. Aceleración (Speedup) y Eficiencia

La aceleración tipifica la ganancia temporal frente a un único procesador ($S(p) = T_1 / T_p$). Simultáneamente, la eficiencia denota el rendimiento efectivo del hardware escalado ($E(p) = S(p)/p \times 100\%$).

En el **Entorno 1**, la aceleración alcanzó **6.92x** con 8 hilos, garantizando una eficiencia del **86.5%**. Por su parte, en el **Entorno 2 (Windows)** sobre el conjunto completo de 624 imágenes reales, la aceleración de cómputo alcanzó **4.31x** con 8 hilos y **3.08x** con 4 hilos (eficiencia del **77.1%**). La diferencia de aceleración total frente al entorno Linux refleja principalmente el mayor overhead de E/S en disco del subsistema de archivos en Windows bajo accesos concurrentes de archivos PNG/JPEG.

---

## f. Fracción Secuencial, Sincronización y Ley de Amdahl

La formulación de Amdahl acota la mejora máxima posible, delimitada por la porción de ejecución rígidamente secuencial ($f$):
$$S(p) = \frac{1}{f + \frac{1 - f}{p}}$$

Para el escenario de Linux con carga alta, el desvío experimental promedió una fracción de $f \approx 0.0283$ ($2.83\%$). Esta evidencia prueba que el 97.17% del algoritmo puro logró paralelizarse. 

No obstante, en el ecosistema Windows con una carga ligera, el factor $f$ registró una aparente degradación al $0.1278$ ($12.78\%$). Semejante discrepancia no refleja un fallo del paralelismo, sino que cuantifica directamente el coste colateral de sincronización ($T_{sync}$). Al multiplicar la concurrencia, las peticiones masivas al subsistema de almacenamiento del disco (E/S) conforman una sección crítica *de facto*. Dicha contención satura el bus central, actuando como un cuello de botella equiparable al que padece el nodo receptor en rutinas de comunicación colectivas asimétricas (`MPI_Gather`) dentro de esquemas distribuidos.
