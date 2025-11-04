#!/bin/bash
# Script para flashear el ESP32-S3 y abrir monitor serial

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Puerto serial (puedes cambiarlo)
PORT="${1:-/dev/ttyUSB0}"

echo -e "${BLUE}📡 Flasheando ESP32-S3 en puerto ${PORT}...${NC}"

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

# Verificar que el binario existe
if [ ! -f "build/opencv_esp32.bin" ]; then
    echo -e "${RED}✗ Error: No se encuentra build/opencv_esp32.bin${NC}"
    echo -e "${YELLOW}⚠️  Ejecuta primero: ./build.sh${NC}"
    exit 1
fi

# Verificar que el puerto existe
if [ ! -e "$PORT" ]; then
    echo -e "${RED}✗ Error: Puerto $PORT no encontrado${NC}"
    echo -e "${YELLOW}Puertos disponibles:${NC}"
    ls -la /dev/ttyUSB* 2>/dev/null || echo "  No hay puertos /dev/ttyUSB*"
    ls -la /dev/ttyACM* 2>/dev/null || echo "  No hay puertos /dev/ttyACM*"
    exit 1
fi

# Flashear y abrir monitor
echo -e "${GREEN}✓${NC} Flasheando y abriendo monitor serial..."
echo -e "${YELLOW}💡 Presiona Ctrl+] para salir del monitor${NC}"
echo ""

idf.py -p "$PORT" flash monitor

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Error durante el flasheo${NC}"
    exit 1
fi
