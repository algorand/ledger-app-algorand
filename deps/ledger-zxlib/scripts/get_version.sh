#!/bin/bash

zxpath="$(dirname "$0")/../include/zxversion.h"
version_numbers=$(sed '/[0-9]/!d; /*/d; s/[A-Za-z#_ ]//g' "$zxpath" | tr '\n' '.')
echo "v${version_numbers%?}" # %? stands for removing the last character of the file as `tr` adds an extra `.`
