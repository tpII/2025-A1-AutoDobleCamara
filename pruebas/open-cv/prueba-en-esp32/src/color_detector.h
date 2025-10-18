#ifndef COLOR_DETECTOR_H
#define COLOR_DETECTOR_H

#include <Arduino.h>

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
    if (count > 50) {  // Ajustar este umbral según necesidad
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

// Predefinidos: Colores comunes
namespace Colors {
    // Rojo (para objetos rojos brillantes)
    const ColorRange RED = {150, 255, 0, 100, 0, 100};
    
    // Verde
    const ColorRange GREEN = {0, 100, 100, 255, 0, 100};
    
    // Azul
    const ColorRange BLUE = {0, 100, 0, 100, 150, 255};
    
    // Amarillo
    const ColorRange YELLOW = {150, 255, 150, 255, 0, 100};
    
    // Naranja
    const ColorRange ORANGE = {200, 255, 80, 150, 0, 80};
}

#endif
