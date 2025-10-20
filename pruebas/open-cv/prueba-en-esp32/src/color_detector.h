#ifndef COLOR_DETECTOR_H
#define COLOR_DETECTOR_H

#include <Arduino.h>

/*
 * 🟡 DETECTOR DE COLOR AMARILLO - OPTIMIZADO
 * 
 * Configuración de cámara en main.cpp:
 * - Saturación: +3 (MUY ALTA)
 * - Contraste: +2 (ALTO)
 * - Brillo: 0 (neutro)
 * - Balance de blancos: SUNNY (modo 1)
 * - Exposición: -1 (reducida)
 * 
 * Ajustes para diferentes condiciones:
 * - Mucha luz / exterior: usar YELLOW_BRIGHT
 * - Luz normal / interior: usar YELLOW (por defecto)
 * - Poca luz / sombra: usar YELLOW_DARK
 * - Muy poca luz: cambiar wb_mode a 3 (Office) o 4 (Home)
 */

// Estructura para almacenar rangos de color en RGB565
struct ColorRange {
    uint16_t r_min, r_max;
    uint16_t g_min, g_max;
    uint16_t b_min, b_max;
};

// Resultado de detección
struct DetectionResult {
    bool found;
    int x_center;
    int y_center;
    int width;
    int height;
    int pixel_count;
};

// Convertir RGB565 a componentes RGB
inline void rgb565_to_rgb(uint16_t rgb565, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (rgb565 >> 11) & 0x1F;  // 5 bits de rojo
    g = (rgb565 >> 5) & 0x3F;   // 6 bits de verde
    b = rgb565 & 0x1F;          // 5 bits de azul
    
    // Expandir a 8 bits
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
}

// Verificar si un pixel está en el rango de color
inline bool is_in_color_range(uint16_t pixel, const ColorRange &range) {
    uint8_t r, g, b;
    rgb565_to_rgb(pixel, r, g, b);
    
    return (r >= range.r_min && r <= range.r_max &&
            g >= range.g_min && g <= range.g_max &&
            b >= range.b_min && b <= range.b_max);
}

// Detectar objeto por color (procesamiento eficiente línea por línea)
DetectionResult detect_colored_object(uint16_t* frame, int width, int height, const ColorRange &range) {
    DetectionResult result = {false, 0, 0, 0, 0, 0};
    
    int x_min = width, x_max = 0;
    int y_min = height, y_max = 0;
    int x_sum = 0, y_sum = 0;
    int count = 0;
    
    // Procesar cada pixel
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint16_t pixel = frame[y * width + x];
            
            if (is_in_color_range(pixel, range)) {
                count++;
                x_sum += x;
                y_sum += y;
                
                // Actualizar bounding box
                if (x < x_min) x_min = x;
                if (x > x_max) x_max = x;
                if (y < y_min) y_min = y;
                if (y > y_max) y_max = y;
            }
        }
    }
    
    // Si encontramos suficientes pixels (filtro de ruido)
    // Umbral MÍNIMO para máxima sensibilidad
    if (count > 10) {  // Umbral muy bajo para capturar cualquier cosa
        result.found = true;
        result.x_center = x_sum / count;
        result.y_center = y_sum / count;
        result.width = x_max - x_min + 1;
        result.height = y_max - y_min + 1;
        result.pixel_count = count;
    }
    
    return result;
}

// Calcular distancia basada en el tamaño del objeto
// Formula: distancia = (ancho_real_objeto * focal_length) / ancho_en_pixels
float calculate_distance(int pixel_width, float real_width_cm, float focal_length) {
    if (pixel_width <= 0) return -1;
    return (real_width_cm * focal_length) / pixel_width;
}

// Predefinidos: Colores comunes (optimizados para cámara con saturación +2)
namespace Colors {
    // Rojo (para objetos rojos brillantes) - Ajustado para mejor detección
    const ColorRange RED = {140, 255, 0, 90, 0, 90};
    
    // Verde - Ajustado para colores más vivos
    const ColorRange GREEN = {0, 90, 120, 255, 0, 90};
    
    // Azul - Ajustado para mejor detección
    const ColorRange BLUE = {0, 90, 0, 120, 140, 255};
    
    // 🟡 AMARILLO - RANGO EXTREMADAMENTE AMPLIO
    // Con el tinte magenta de la cámara, el amarillo se ve blanco/rosa/magenta claro
    // Este rango captura prácticamente cualquier color claro incluyendo blanco
    const ColorRange YELLOW = {80, 255, 80, 255, 80, 255};
    
    // Variante: Amarillo brillante (para objetos muy iluminados)
    const ColorRange YELLOW_BRIGHT = {180, 255, 180, 255, 0, 90};
    
    // Variante: Amarillo oscuro/oro (para sombras o amarillo apagado)
    const ColorRange YELLOW_DARK = {100, 180, 100, 180, 0, 100};
    
    // Naranja - Mejorado para distinción de rojo
    const ColorRange ORANGE = {200, 255, 100, 180, 0, 70};
    
    // Cian (nuevo) - Útil para detección
    const ColorRange CYAN = {0, 80, 200, 255, 180, 255};
    
    // Magenta (nuevo) - Útil para detección
    const ColorRange MAGENTA = {180, 255, 0, 80, 180, 255};
    
    // Blanco - Para objetos blancos o muy claros
    const ColorRange WHITE = {200, 255, 200, 255, 200, 255};
    
    // Negro - Para objetos oscuros
    const ColorRange BLACK = {0, 50, 0, 50, 0, 50};
}

#endif
