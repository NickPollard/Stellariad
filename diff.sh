#!/bin/zsh
git diff HEAD --word-diff=color -U1 | grep -v '[^A-Za-z\d]*\(\-\-\-\|+++\|index\)' | sed -e 's/@@//g' -e 's/--git//' -e 's/diff/\ndiff/g' -e 's/.*/\t&/g' -e 's/\-\([0-9]*\),[0-9] +\([0-9]*\),[0-9]*/\1 -> \2\n\t/'
