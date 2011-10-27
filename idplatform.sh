#!/bin/sh

UNAME_S=`uname -s |sed 's/\([^_]*\).*/\1/'`

if [ "x$UNAME_S" = "xDarwin" ]; then
	echo osx
elif [ "x$UNAME_S" = "xLinux" ]; then
	echo linux
elif [ "x$UNAME_S" = "xMINGW32" ]; then
	echo windows-mingw
elif [ "x$UNAME_S" = "xCYGWIN" ]; then
	echo windows-cygwin
else
	if [ "x$1" = "xdebug" ]; then
		echo "UNAME_S = [$UNAME_S]"
	else
		echo unknown
	fi
fi

