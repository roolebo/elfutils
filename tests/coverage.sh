#! /bin/bash

cd ..

for d in lib libasm libdw libdwfl libebl libelf src; do
  tmp=../$d-data
  cd $d
  unused=0
  for f in *.gcno; do
    base="$(basename $f .gcno)"
    fc="$base.c"
    gcda="$base.gcda"
    if [ -f "$gcda" ]; then
      gcov -n -a "$fc" |
      gawk "/$d.$fc/ { getline; co=gensub(/.*:(.*)% .*/, \"\\\\1\", \"g\"); co=co+0.0; li=\$4+0; printf \"%-35s  %6.2f %5d\n\", \"$d/$fc\", co, li } " >> $tmp
    else
      unused=$(($unused + 1))
    fi
  done
  gawk "{ copct=\$2; co=(\$3*copct)/100; toco+=(co+0); toli += (\$3+0); } END { printf \"%-12s %6.2f%% covered       unused files: %3d\n\", \"$d\", (toco*100)/toli, \"$unused\" }" $tmp
  rm -f $tmp
  cd ..
done
