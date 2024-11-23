#!/bin/bash 

default=`cat default.txt`

if [ $# -eq 1 ] && [ "$1" = "default" ]; then
	if [[ $default == make* ]] || [[ $default == gcc* ]]; then
		eval "$default"
	else
		echo "DANGEROUS CONTENT IN DEFAULT FILE!"
	fi
	exit
elif [ "$1" = "set" ] && [ "$2" = "default" ]; then
	echo "$3" > default.txt
	exit
elif [ $# -ne 2 ]; then
	echo "usage: $0 <file name> <extra flag>"
	exit
fi

rm hacking_my.o
gcc hacking_my.c -o hacking_my.o -c -g
make clean FILENAME="$1"
make debug FILENAME="$1" EXTRAFLAGS="$2"
