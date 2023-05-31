#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo -e "Usage:   $0 /path/to/repository/to/check"
    exit 1
fi

listpath=$(mktemp)
# find $1 -name '*.c' | shuf | head -n500 > $listpath
find $1 -name '*.c' > $listpath

resultpath=$(mktemp)
parallel --progress --eta --bar -a $listpath "python3 helper_scripts/check_file.py" > $resultpath

python3 collate.py $resultpath

rm $listpath
rm $resultpath
