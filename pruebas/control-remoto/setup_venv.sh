#!/usr/bin/env bash
set -euo pipefail

VEENV_DIR=".venv"

if [ -d "$VEENV_DIR" ]; then
  echo "Virtualenv '$VEENV_DIR' already exists. Activate it with: source $VEENV_DIR/bin/activate"
  exit 0
fi

python3 -m venv "$VEENV_DIR"
echo "Created virtualenv in $VEENV_DIR"
echo "Activating and installing requirements..."

source "$VEENV_DIR/bin/activate"
pip install --upgrade pip
if [ -f requirements.txt ]; then
  pip install -r requirements.txt
else
  echo "No requirements.txt found, skipping pip install"
fi

echo "Setup complete. Activate with: source $VEENV_DIR/bin/activate"
