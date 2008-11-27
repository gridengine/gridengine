#!/bin/sh

#Detect JAVA
if [ -n "$JAVA_HOME" -a -f "$JAVA_HOME"/bin/java ]; then
   JAVA_BIN="$JAVA_HOME"/bin/java
else
   JAVA_BIN=`which java`
fi

if [ ! -f "$JAVA_BIN" ]; then
   echo "Java not found! Specify JAVA_HOME or adjust your PATH and try again."
   exit 1
fi

$JAVA_BIN -Dswing.boldMetal=false -jar ./util/gui-installer/installer.jar
