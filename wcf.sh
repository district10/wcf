#!/bin/bash

FILE=$1

sed -e 's/\([A-Z]\)/ \L\1/g' $FILE | \
gawk -f wcf.awk | sort -k 2nr
