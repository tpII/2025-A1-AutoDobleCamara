# Sistema de Detección de Objetos y Medición de Distancia - ESP32

## 🎯 Características

- ✅ **Detección por color** (muy eficiente en memoria)
- ✅ **Cálculo de distancia** basado en tamaño del objeto
- ✅ **Interface web** con actualización en tiempo real
- ✅ **Stream de video** con bounding boxes
- ✅ **Optimizado para ESP32 sin PSRAM**

## 📐 Calibración del Sistema

### Paso 1: Ajustar el Color del Objeto

En `main.cpp`, cambia el color a detectar:

```cpp
// Opciones disponibles:
Colors::RED     // Rojo (por defecto)
Colors::GREEN   // Verde
Colors::BLUE    // Azul
Colors::YELLOW  // Amarillo
Colors::ORANGE  // Naranja
```

O crea tu propio rango de color en `color_detector.h`:

```cpp
const ColorRange MI_COLOR = {
    r_min, r_max,  // Rojo: 0-255
    g_min, g_max,  // Verde: 0-255
    b_min, b_max   // Azul: 0-255
};
```

### Paso 2: Medir el Ancho Real del Objeto

1. Mide el ancho del objeto que quieres detectar en **centímetros**
2. Actualiza en `main.cpp`:

```cpp
const float OBJETO_ANCHO_REAL_CM = 5.0;  // Tu medida aquí
```

### Paso 3: Calibrar la Longitud Focal

Este es el paso más importante para obtener distancias precisas:

1. Coloca el objeto a una **distancia conocida** (ej: 30 cm)
2. Ve al endpoint `/detect` y observa el valor de `width` (ancho en píxeles)
3. Calcula la longitud focal:

```
FOCAL_LENGTH = (ancho_en_pixels × distancia_real_cm) / ancho_real_cm
```

Ejemplo:

- Objeto a 30 cm de distancia
- Ancho real: 5 cm
- Ancho medido: 50 pixels

```
FOCAL_LENGTH = (50 × 30) / 5 = 300
```

4. Actualiza en `main.cpp`:

```cpp
const float FOCAL_LENGTH = 300.0;  // Tu valor calculado
```

### Paso 4: Verificar Precisión

1. Coloca el objeto a diferentes distancias conocidas (20cm, 40cm, 60cm)
2. Compara la distancia medida vs la distancia real
3. Si hay error sistemático, ajusta ligeramente FOCAL_LENGTH

## 🚀 Uso

### Conectarse al ESP32

1. Conecta al WiFi: **ESP32_AP**
2. Contraseña: **12345678**
3. Abre en navegador: **http://192.168.4.1**

### Endpoints Disponibles

- `/` - Interface web principal con medición de distancia
- `/detect` - JSON con datos de detección y distancia
- `/stream` - Stream de video con bounding boxes

## 💡 Tips de Optimización

### Para Mejorar Detección

1. **Iluminación uniforme**: Evita sombras y reflejos
2. **Fondo contrastante**: Usa un fondo de color diferente al objeto
3. **Objeto de color sólido**: Colores brillantes funcionan mejor
4. **Ajustar umbral**: En `color_detector.h`, cambia el umbral de píxeles mínimos:

```cpp
if (count > 50) {  // Aumentar = menos ruido, pero puede perder objetos pequeños
```

### Para Ahorrar Memoria

El sistema ya está optimizado, pero puedes:

1. Reducir resolución a FRAMESIZE_96x96 (en `setupCamera()`)
2. Reducir frecuencia de detección (aumentar delay en JavaScript)
3. Desactivar el stream y usar solo `/detect`

## 🔧 Resolución de Problemas

### "No Detectado" constantemente

- Verifica que el rango de color sea correcto
- Aumenta el rango de color (valores más amplios de min/max)
- Reduce el umbral de píxeles mínimos
- Mejora la iluminación

### Distancia Incorrecta

- Recalibra FOCAL_LENGTH con el procedimiento del Paso 3
- Verifica que OBJETO_ANCHO_REAL_CM sea correcto
- Asegúrate que la cámara esté perpendicular al objeto

### Memoria Insuficiente

- Usa FRAMESIZE_96x96 en lugar de QQVGA
- Desactiva WiFi AP después de conectarse
- Reduce fb_count a 1 (ya está configurado)

## 📊 Limitaciones

- **Rango de distancia**: ~10cm a 100cm (depende del tamaño del objeto)
- **Precisión**: ±5-10% con buena calibración
- **FPS**: ~2-5 fps con detección activa
- **Resolución**: 160×120 (QQVGA) para balance memoria/precisión

## 🎓 Alternativas para Mayor Precisión

Si necesitas mayor precisión, considera:

1. **Sensor ultrasónico HC-SR04**: Distancia directa sin cámara
2. **Sensor infrarrojo Sharp GP2Y0A21**: Medición analógica de distancia
3. **Cámara estereoscópica**: Dos cámaras para visión 3D (requiere más memoria)
4. **Procesamiento en servidor**: Envía frames por WiFi, procesa con OpenCV en PC
