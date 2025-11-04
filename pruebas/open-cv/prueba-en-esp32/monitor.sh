#!/bin/bash
# Script para abrir solo el monitor serial (sin flashear)

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Puerto serial (puedes cambiarlo)
PORT="${1:-/dev/ttyUSB0}"

echo -e "${BLUE}📺 Abriendo monitor serial en puerto ${PORT}...${NC}"

# Desactivar venv local si existe
if [[ "$VIRTUAL_ENV" != "" ]]; then
    deactivate 2>/dev/null || true
fi

# Cargar entorno ESP-IDF
if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    . $HOME/esp/esp-idf/export.sh > /dev/null 2>&1
else
    echo -e "${RED}✗ Error: No se encuentra ESP-IDF${NC}"
    exit 1
fi

# Verificar que el puerto existe
if [ ! -e "$PORT" ]; then
    echo -e "${RED}✗ Error: Puerto $PORT no encontrado${NC}"
    exit 1
fi

echo -e "${YELLOW}💡 Presiona Ctrl+] para salir del monitor${NC}"
echo ""

idf.py -p "$PORT" monitor
