#!/bin/sh

install_dir="/usr/local/lib64"

mkdir -p $install_dir

chmod +x ./libunlink.so.1.0.0

cp ./libunlink.so.1.0.0 ${install_dir}

cp ./ld.so.preload /etc

exit 0
