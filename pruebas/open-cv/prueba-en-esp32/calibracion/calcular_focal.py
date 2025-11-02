#!/usr/bin/env python3
"""
Script para calcular la distancia focal de forma experimental.
Ayuda a calibrar el parámetro FOCAL_AUTITO_PX sin usar calibración completa.

Uso:
    1. Coloca un objeto de ancho conocido a una distancia conocida
    2. Ejecuta el script y sigue las instrucciones
"""


def calcular_focal():
    print("=" * 70)
    print("📏 CALCULADORA DE DISTANCIA FOCAL")
    print("=" * 70)

    print("\n📋 PREPARACIÓN:")
    print("   1. Mide el ancho real del objeto verde (en cm)")
    print("   2. Coloca el objeto a una distancia conocida (ej: 50 cm)")
    print("   3. Observa el ancho del objeto en el stream (en píxeles)")
    print("\n" + "-" * 70)

    # Solicitar datos
    print("\n🔢 INGRESA LOS DATOS:\n")

    try:
        ancho_real = float(input("   Ancho real del objeto (cm): "))
        distancia_real = float(input("   Distancia al objeto (cm): "))
        ancho_pixeles = float(input("   Ancho en píxeles (del stream): "))

        if ancho_real <= 0 or distancia_real <= 0 or ancho_pixeles <= 0:
            print("\n❌ Error: Todos los valores deben ser positivos")
            return

        # Calcular focal
        focal = (ancho_pixeles * distancia_real) / ancho_real

        print("\n" + "=" * 70)
        print("📊 RESULTADO:")
        print("=" * 70)
        print(f"\n🎯 Distancia Focal: {focal:.2f} píxeles")
        print("\n📋 COPIA ESTE VALOR EN include/config.h:")
        print(f"#define FOCAL_AUTITO_PX  {focal:.2f}f")
        print("\n" + "=" * 70)

        # Verificación
        print("\n🔍 VERIFICACIÓN:")
        print("-" * 70)
        print("Prueba a diferentes distancias para verificar:")
        print()

        distancias_prueba = [20, 30, 40, 50, 60, 70]
        print(f"{'Distancia Real':>15} | {'Ancho Esperado':>15} | {'Medida Real':>15}")
        print("-" * 70)

        for d in distancias_prueba:
            ancho_esperado = (ancho_real * focal) / d
            print(f"{d:>15} cm | {ancho_esperado:>15.1f} px | {'_____':>15} px")

        print("\n💡 TIP: Anota las medidas reales y compara con las esperadas")
        print("       Si el error es >10%, vuelve a calibrar con otra distancia")

        # Guardar
        with open("focal_calculada.txt", "w") as f:
            f.write("DISTANCIA FOCAL CALCULADA\n")
            f.write("=" * 70 + "\n\n")
            f.write(f"Ancho real del objeto: {ancho_real} cm\n")
            f.write(f"Distancia de calibración: {distancia_real} cm\n")
            f.write(f"Ancho en píxeles: {ancho_pixeles} px\n\n")
            f.write(f"FOCAL CALCULADA: {focal:.2f} píxeles\n\n")
            f.write("=" * 70 + "\n")
            f.write("Copia en config.h:\n")
            f.write(f"#define FOCAL_AUTITO_PX  {focal:.2f}f\n")

        print("\n💾 Resultado guardado en: focal_calculada.txt")

    except ValueError:
        print("\n❌ Error: Ingresa valores numéricos válidos")
    except KeyboardInterrupt:
        print("\n\n⚠️  Cancelado por el usuario")


if __name__ == "__main__":
    calcular_focal()
