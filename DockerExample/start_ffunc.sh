#!/bin/sh

sed -i '/listen[[:space:]]\+80;/a include conf.d/ffunc_profile;' /etc/nginx/conf.d/default.conf

gcc -I/tmp/fcgi-function /tmp/profile_service_daemonize.c /tmp/fcgi-function/ffunc.c -lfcgi -pthread -ldl -rdynamic -o ffuncsample
./ffuncsample


nginx -g "daemon off;"
