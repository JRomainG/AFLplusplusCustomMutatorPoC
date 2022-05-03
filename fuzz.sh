#!/bin/bash
BASEPATH=`dirname $(realpath -s $0)`

afl_path="$BASEPATH/aflplusplus"
corpus_path="$BASEPATH/corpus"
output_path="$BASEPATH/output"

## Use custom mutator

export AFL_DISABLE_TRIM=1
export AFL_CUSTOM_MUTATOR_ONLY=1
export AFL_CUSTOM_MUTATOR_LIBRARY="$BASEPATH/libmutator.so"

## Store .cur_input in /dev/shm

export AFL_TMPDIR=/dev/shm

if [[ -f "$AFL_TMPDIR/.cur_input" ]]; then
    echo "Deleting existing temporary input file at $AFL_TMPDIR/.cur_input."
    rm "$AFL_TMPDIR/.cur_input"
fi

## Debug mode

export AFL_DEBUG=1
export AFL_DEBUG_CHILD=1
export AFL_SKIP_CPUFREQ=1
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1

## Run AFL++

"$afl_path"/afl-fuzz -i "$corpus_path" -o "$output_path" -- "$BASEPATH/target.o" @@
