#!/bin/bash
# Script para compilar el proyecto ESP-IDF con OpenCV

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${YELLOW}🔨 Compilando proyecto ESP-IDF con OpenCV...${NC}"

# Desactivar venv local si existe
if [[ "$VIRTUAL_ENV" != "" ]]; then
    echo -e "${YELLOW}⚠️  Desactivando entorno virtual local...${NC}"
    deactivate 2>/dev/null || true
fi

# Cargar entorno ESP-IDF
if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    echo -e "${GREEN}✓${NC} Cargando entorno ESP-IDF..."
    . $HOME/esp/esp-idf/export.sh
else
    echo -e "${RED}✗ Error: No se encuentra ESP-IDF en $HOME/esp/esp-idf${NC}"
    exit 1
fi

# Compilar
echo -e "${GREEN}✓${NC} Iniciando compilación..."
idf.py build

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓✓✓ Compilación exitosa! ✓✓✓${NC}"
    echo -e "${YELLOW}Binario generado en: build/opencv_esp32.bin${NC}"
    
    # Mostrar tamaño del binario
    SIZE=$(stat -f%z build/opencv_esp32.bin 2>/dev/null || stat -c%s build/opencv_esp32.bin 2>/dev/null)
    SIZE_KB=$((SIZE / 1024))
    echo -e "${GREEN}Tamaño: ${SIZE_KB} KB${NC}"
else
    echo -e "${RED}✗✗✗ Error en la compilación ✗✗✗${NC}"
    exit 1
fi
