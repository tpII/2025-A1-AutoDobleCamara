VENV=.venv
ACTIVATE=$(VENV)/bin/activate

.PHONY: venv install clean

venv:
	python3 -m venv $(VENV)

install: venv
	@echo "Activating and installing requirements..."
	@. $(ACTIVATE) && pip install --upgrade pip && pip install -r requirements.txt

clean:
	rm -rf $(VENV)
