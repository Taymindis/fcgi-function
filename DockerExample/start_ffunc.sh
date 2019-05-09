#!/bin/sh

cp ffunc_profile /etc/nginx/conf.d/ffunc_profile

sed -i '/include conf.d\/ffunc_profile;/d  ' /etc/nginx/conf.d/default.conf
sed -i '/listen[[:space:]]\+80;/a include conf.d/ffunc_profile;' /etc/nginx/conf.d/default.conf

gcc -I/opt/fcgi-function /tmp/profile_service_daemonize.c /opt/fcgi-function/ffunc.c -lfcgi -pthread -ldl -rdynamic -o ffuncsample
./ffuncsample

nginx -g "daemon off;"
