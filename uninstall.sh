#!/bin/sh

[ -f "/etc/ld.so.preload" ] && rm -rf "/etc/ld.so.preload"

[ -f "/usr/local/lib64/libunlink.so.1.0.0" ] && rm -rf "/usr/local/lib64/libunlink.so.1.0.0"

exit 0
