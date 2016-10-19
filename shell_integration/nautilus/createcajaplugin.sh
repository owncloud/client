#!/bin/sh

# this script creates a plugin for nemo, just be replacing
# all occurences of Nautilus with Nemo.

sed -i.org -e 's/Nautilus/Caja/g' syncstate_caja.py
sed -i.org -e 's/nautilus/caja/g' syncstate_caja.py
