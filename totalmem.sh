#!/bin/zsh
cat memlog | awk '{print $7 " " $11}' | sed 's/,//' | awk '{arr[$2] += $1}END{for( a in arr ) { print arr[a] " = " a }}' | sort -nk1 > memtotals
