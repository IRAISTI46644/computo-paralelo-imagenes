#!/usr/bin/env python3
"""
Generador de imagenes de muestra sinteticas tipo radiografia (X-Ray)
para pruebas de estres, verificacion de algoritmos y benchmarking.
"""
import os
import argparse
import numpy as np
from PIL import Image

def generate_synthetic_xray(width=1024, height=1024):
    """Genera una imagen con gradientes, patrones circulares y estructuras de alta frecuencia."""
    y, x = np.ogrid[:height, :width]
    
    # Fondo con gradiente radial (simulando caja toracica)
    cx, cy = width / 2, height / 2
    r2 = (x - cx)**2 + (y - cy)**2
    bg = 200 * np.exp(-r2 / (2 * (width * 0.35)**2))
    
    # Estructuras tipo costillas/tejido
    ribs = 40 * np.sin(y / 25.0) * (np.exp(-((x - cx) / (width * 0.4))**4))
    
    # Lineas verticales (columna vertebral)
    spine = 50 * np.exp(-((x - cx) / 20.0)**2)
    
    # Ruido gaussiano
    noise = np.random.normal(0, 15, (height, width))
    
    img = bg + ribs + spine + noise
    img = np.clip(img, 0, 255).astype(np.uint8)
    return Image.fromarray(img)

def main():
    parser = argparse.ArgumentParser(description="Genera imagenes de prueba para el benchmark.")
    parser.add_argument("-n", "--count", type=int, default=20, help="Numero de imagenes a generar (def: 20)")
    parser.add_argument("-w", "--width", type=int, default=1024, help="Ancho de imagen (def: 1024)")
    parser.add_argument("-H", "--height", type=int, default=1024, help="Alto de imagen (def: 1024)")
    parser.add_argument("-o", "--output-dir", type=str, default="data/input", help="Directorio destino")
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)
    print(f"[INFO] Generando {args.count} imagenes de {args.width}x{args.height} en '{args.output_dir}'...")

    for i in range(1, args.count + 1):
        img = generate_synthetic_xray(args.width, args.height)
        filename = f"sample_xray_{i:03d}.png"
        path = os.path.join(args.output_dir, filename)
        img.save(path)

    print(f"[OK] {args.count} imagenes generadas exitosamente.")

if __name__ == "__main__":
    main()
