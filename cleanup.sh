#!/bin/bash

echo "Clean SEGGER Embedded Studio project files, replace \\\\ by /"
find . -type f -name "*.emProject" -exec perl -pi -e 's|\\\\|/|g' {} +

echo "Clean '#include', replace \\ by /"
find . -type f \( -name "*.c" -o -name "*.h" \) -exec perl -pi -e 's|\\|/|g if /^\s*#include/' {} +

echo "Remove whitespaces"
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.htm" \) -exec perl -pi -e 's/[ \t]+$//' {} +





