#!/usr/bin/env python3
"""
Harness automatizado de Benchmarking y Analisis de Rendimiento (Ley de Amdahl)
Ejecuta N corridas para hilos [1, 2, 4, 8], calcula aceleracion (S), eficiencia (E),
fraccion secuencial (f) y tiempos de sincronizacion, y genera graficas para el reporte.
"""
import os
import sys
import subprocess
import argparse
import json
import numpy as np
import matplotlib
matplotlib.use('Agg')  # Render sin GUI/X11
import matplotlib.pyplot as plt

def run_command_csv(binary, input_path, output_path, threads, mode):
    cmd = [
        binary,
        "-i", input_path,
        "-o", output_path,
        "-t", str(threads),
        "-m", mode,
        "-c"
    ]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Error al ejecutar binario: {res.stderr}")
    
    # Formato: threads,items,total_time,compute_time,io_time,sync_time
    line = res.stdout.strip().split("\n")[-1]
    parts = line.split(",")
    return {
        "threads": int(parts[0]),
        "items": int(parts[1]),
        "total_time": float(parts[2]),
        "compute_time": float(parts[3]),
        "io_time": float(parts[4]),
        "sync_time": float(parts[5]),
    }

def main():
    parser = argparse.ArgumentParser(description="Automatizacion de benchmarks y Ley de Amdahl.")
    parser.add_argument("--bin", default="bin/img_processor", help="Ruta al binario C")
    parser.add_argument("-i", "--input", default="data/input", help="Directorio o archivo de entrada")
    parser.add_argument("-o", "--output", default="data/output", help="Carpeta de salida")
    parser.add_argument("--runs", type=int, default=3, help="Numero de corridas por configuracion de hilos (minimo 3)")
    parser.add_argument("--threads", nargs="+", type=int, default=[1, 2, 4, 8], help="Lista de hilos a evaluar")
    parser.add_argument("-m", "--mode", choices=["batch", "pixel"], default="batch", help="Estrategia de paralelismo")
    parser.add_argument("--plot-dir", default="docs/figures", help="Directorio para guardar las graficas")
    parser.add_argument("--quick", action="store_true", help="Modo rapido (1 corrida, para tests)")
    args = parser.parse_args()

    if args.quick:
        args.runs = 1
        args.threads = [1, 2, 4]

    os.makedirs(args.plot_dir, exist_ok=True)
    os.makedirs(args.output, exist_ok=True)

    print("=" * 65)
    print(" INICIANDO SUITE DE BENCHMARKING (LEY DE AMDAHL)")
    print(f" Binario:        {args.bin}")
    print(f" Entrada:        {args.input}")
    print(f" Modo:           {args.mode}")
    print(f" Hilos:          {args.threads}")
    print(f" Repeticiones:   {args.runs} por configuracion")
    print("=" * 65)

    results_by_threads = {}

    for t in args.threads:
        print(f"\n[INFO] Ejecutando configuracion: {t} hilos ({args.runs} repeticiones)...")
        totals, computes, ios, syncs = [], [], [], []
        items_cnt = 0
        for r in range(1, args.runs + 1):
            data = run_command_csv(args.bin, args.input, args.output, t, args.mode)
            items_cnt = data["items"]
            totals.append(data["total_time"])
            computes.append(data["compute_time"])
            ios.append(data["io_time"])
            syncs.append(data["sync_time"])
            print(f"   Corrida {r}: Total={data['total_time']:.4f}s | Computo={data['compute_time']:.4f}s | I/O={data['io_time']:.4f}s | Sync={data['sync_time']:.6f}s")
        
        results_by_threads[t] = {
            "items": items_cnt,
            "total_mean": float(np.mean(totals)),
            "total_std": float(np.std(totals)),
            "compute_mean": float(np.mean(computes)),
            "compute_std": float(np.std(computes)),
            "io_mean": float(np.mean(ios)),
            "sync_mean": float(np.mean(syncs)),
        }

    # Baseline T1 (1 hilo)
    t1_total = results_by_threads[1]["total_mean"]
    t1_comp = results_by_threads[1]["compute_mean"]

    # Calculos de Speedup (S), Eficiencia (E) y Fraccion Secuencial (f) de Amdahl
    f_estimates = []
    for t in args.threads:
        tp_comp = results_by_threads[t]["compute_mean"]
        tp_total = results_by_threads[t]["total_mean"]
        
        speedup_comp = t1_comp / tp_comp if tp_comp > 0 else 1.0
        speedup_total = t1_total / tp_total if tp_total > 0 else 1.0
        efficiency_comp = (speedup_comp / t) * 100.0
        efficiency_total = (speedup_total / t) * 100.0
        
        # Fraccion secuencial f por Ley de Amdahl:
        # S = 1 / (f + (1-f)/p)  =>  f = (1/S - 1/p) / (1 - 1/p)
        if t > 1 and speedup_comp > 0:
            f_val = ((1.0 / speedup_comp) - (1.0 / t)) / (1.0 - (1.0 / t))
            f_val = max(0.0, min(1.0, f_val))
            f_estimates.append(f_val)
        else:
            f_val = 0.0

        results_by_threads[t]["speedup_comp"] = speedup_comp
        results_by_threads[t]["speedup_total"] = speedup_total
        results_by_threads[t]["efficiency_comp"] = efficiency_comp
        results_by_threads[t]["efficiency_total"] = efficiency_total
        results_by_threads[t]["f_fraction"] = f_val

    f_mean = float(np.mean(f_estimates)) if f_estimates else 0.05

    print("\n" + "=" * 80)
    print(" TABLA RESUMEN PARA EL REPORTE TECNICO (MARKDOWN)")
    print("=" * 80)
    header = "| Hilos (p) | T. Total (s) | T. Cómputo (s) | T. E/S (s) | T. Sync (s) | Speedup (S) | Eficiencia (%) | Fracc. Sec (f) |"
    sep    = "| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |"
    print(header)
    print(sep)
    for t in args.threads:
        r = results_by_threads[t]
        print(f"| {t} | {r['total_mean']:.4f} ± {r['total_std']:.3f} | {r['compute_mean']:.4f} | {r['io_mean']:.4f} | {r['sync_mean']:.6f} | {r['speedup_comp']:.2f}x | {r['efficiency_comp']:.1f}% | {r['f_fraction']:.4f} |")
    print("=" * 80)
    print(f"[METRICA] Fraccion secuencial promedio calculada (Amdahl f): {f_mean:.4f} ({f_mean*100:.2f}%)")

    # Guardar JSON con resultados
    json_path = os.path.join("docs", "benchmark_results.json")
    with open(json_path, "w") as jf:
        json.dump({
            "mode": args.mode,
            "f_mean": f_mean,
            "results": results_by_threads
        }, jf, indent=2)
    print(f"[OK] Datos crudos guardados en: {json_path}")

    # ==========================================
    # GENERACION DE GRAFICAS PARA EL REPORTE
    # ==========================================
    plt.style.use('ggplot')

    # 1. Grafica de Speedup (Ideal vs Real vs Teorico Amdahl)
    plt.figure(figsize=(8, 5))
    threads_arr = np.array(args.threads)
    speedup_real = [results_by_threads[t]["speedup_comp"] for t in args.threads]
    speedup_ideal = threads_arr
    speedup_amdahl = 1.0 / (f_mean + (1.0 - f_mean) / threads_arr)

    plt.plot(threads_arr, speedup_ideal, 'k--', label="Ideal Lineal (S = p)", linewidth=1.8)
    plt.plot(threads_arr, speedup_amdahl, 'b-.', label=f"Teórico Amdahl (f={f_mean:.2f})", linewidth=1.8)
    plt.plot(threads_arr, speedup_real, 'ro-', label="Real Observado (Cómputo)", linewidth=2.2, markersize=8)

    plt.title("Aceleración vs Número de Hilos (Speedup)", fontsize=13, fontweight='bold')
    plt.xlabel("Número de Hilos (p)", fontsize=11)
    plt.ylabel("Speedup (S)", fontsize=11)
    plt.xticks(threads_arr)
    plt.legend(fontsize=10)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.tight_layout()
    p_speedup = os.path.join(args.plot_dir, "speedup.png")
    plt.savefig(p_speedup, dpi=300)
    plt.close()
    print(f"[OK] Grafica generada: {p_speedup}")

    # 2. Grafica de Eficiencia (%)
    plt.figure(figsize=(8, 5))
    eff_real = [results_by_threads[t]["efficiency_comp"] for t in args.threads]
    plt.plot(threads_arr, [100]*len(threads_arr), 'k--', label="Ideal (100%)", linewidth=1.5)
    plt.plot(threads_arr, eff_real, 'go-', label="Eficiencia Observada", linewidth=2.2, markersize=8)
    for x, y in zip(threads_arr, eff_real):
        plt.text(x, y + 2, f"{y:.1f}%", ha='center', fontweight='bold', color='darkgreen')

    plt.title("Eficiencia Paralela vs Número de Hilos", fontsize=13, fontweight='bold')
    plt.xlabel("Número de Hilos (p)", fontsize=11)
    plt.ylabel("Eficiencia E (%)", fontsize=11)
    plt.ylim(0, 115)
    plt.xticks(threads_arr)
    plt.legend(fontsize=10)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.tight_layout()
    p_eff = os.path.join(args.plot_dir, "eficiencia.png")
    plt.savefig(p_eff, dpi=300)
    plt.close()
    print(f"[OK] Grafica generada: {p_eff}")

    # 3. Grafica de Desglose de Tiempos (Computo, I/O, Sincronizacion)
    plt.figure(figsize=(8, 5))
    x_indices = np.arange(len(threads_arr))
    comp_times = [results_by_threads[t]["compute_mean"] for t in args.threads]
    io_times = [results_by_threads[t]["io_mean"] for t in args.threads]
    sync_times = [results_by_threads[t]["sync_mean"] for t in args.threads]

    plt.bar(x_indices, comp_times, label="Cómputo (Filtros)", color="#3498db")
    plt.bar(x_indices, io_times, bottom=comp_times, label="E/S (Carga/Guardado)", color="#e67e22")
    bottom_sync = np.array(comp_times) + np.array(io_times)
    plt.bar(x_indices, sync_times, bottom=bottom_sync, label="Sincronización (Overhead)", color="#e74c3c")

    plt.title("Desglose de Tiempos de Ejecución", fontsize=13, fontweight='bold')
    plt.xlabel("Número de Hilos (p)", fontsize=11)
    plt.ylabel("Tiempo Promedio (segundos)", fontsize=11)
    plt.xticks(x_indices, [f"{t} Hilos" for t in threads_arr])
    plt.legend(fontsize=10)
    plt.grid(axis='y', linestyle=':', alpha=0.6)
    plt.tight_layout()
    p_times = os.path.join(args.plot_dir, "tiempos_desglose.png")
    plt.savefig(p_times, dpi=300)
    plt.close()
    print(f"[OK] Grafica generada: {p_times}")

    print("\n[FIN] Todas las pruebas y graficas se generaron correctamente.")

if __name__ == "__main__":
    main()
