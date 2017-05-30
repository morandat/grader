#!/bin/bash

SILENT="false"
[ "$1" = "--silent" ] && SILENT="true" && shift

if [ $# = 0 ]
then
	echo "Need action. {} replaced by login and // by path"
	exit
fi

ACTION="$@"

for i in `find -maxdepth 1 -mindepth 1 -type d  | xargs`
do
	$SILENT || echo -n "$i "
	pushd $i > /dev/null
	login=`basename $i`
	sh -c "`echo "$ACTION" | sed "s:{}:$login:" | sed "s://:$i:"`" 
	popd  > /dev/null
done
