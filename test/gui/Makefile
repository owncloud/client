# Main Makefile for formatting python code for gui test
#
# Requirements to run make here:
# - black
#
# Installing black can be done via https://black.readthedocs.io/en/stable/getting_started.html
# configuration for black can be found inside pyproject.toml file under [tools.black] section.
#
# Usage:
# make
#	-auto format all the python files inside `test/gui`
# make check
#	-check which files will be reformatted
# make diff
#	-see what changes will be done
#

black:
	black .

.PHONY: diffFormat
diffFormat:
	black --diff .

.PHONY: checkFormat
checkFormat:
	black --check .
