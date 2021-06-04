#!/bin/sh

set -ue

max_count=$1

echo "const char *HashValues[] = {"
for N in `seq 1 ${max_count}`; do
	H=$(echo $N | md5sum | sed 's/ .*//g')
	echo '    "'${H^^}'",';
done
echo "};"
