#!/bin/bash
pass=0
total=0;

for fname in test/*.o; do
    echo ======================================
    echo running test suite: $fname
    echo ======================================
    ./$fname
    rc=$?; if [[ $rc == 0 ]]; then ((pass++)); fi
    ((total++))
done

echo
echo ======================================
echo $pass/$total test suites passing
echo ======================================
echo
