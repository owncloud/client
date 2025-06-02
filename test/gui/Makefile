PYTHON_LINT_PATHS:="./**/*.py"

.PHONY: install
install: install
	python3.10 -m pip install -r requirements.txt
	playwright install chromium

.PHONY: python-lint
python-lint:
	black --check --diff .
	python3.10 -m pylint --rcfile ./.pylintrc $(PYTHON_LINT_PATHS)

.PHONY: python-lint-fix
python-lint-fix:
	black .
	python3.10 -m pylint --rcfile ./.pylintrc $(PYTHON_LINT_PATHS)

.PHONY: gherkin-lint
gherkin-lint:
	gherlint -c ./config/.gherlintrc.json .

.PHONY: gherkin-lint-fix
gherkin-lint-fix:
	gherlint --fix -c ./config/.gherlintrc.json .
