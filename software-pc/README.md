# Estación de Control Doble Cámara (MVP)

Instrucciones rápidas para poner en marcha el proyecto en Linux.

1. Crear y activar entorno virtual

```bash
cd /home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/software-pc
python3 -m venv venv
source venv/bin/activate
```

2. Instalar dependencias

```bash
pip install -r requirements.txt
```

3. Preparar archivo de calibración

Coloca `calibration_data.npz` en la carpeta `software-pc/`. Debe contener las matrices: `mtx1, dist1, mtx2, dist2, R, T`.

4. Ejecutar

```bash
python3 main.py
```

Notas:

- El servidor HTTP se expone en el puerto 5000 por defecto y espera que los ESP32 envíen POST con la imagen JPEG a `/video/cam1` y `/video/cam2`.
- Edita las IPs/puertos del robot en `src/control.py` si es necesario.
