#!/bin/bash
# Script para limpiar el proyecto (equivalente a make clean)

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${YELLOW}🧹 Limpiando proyecto...${NC}"

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

# Limpiar
idf.py fullclean

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Proyecto limpio${NC}"
else
    echo -e "${RED}✗ Error al limpiar${NC}"
    exit 1
fi
