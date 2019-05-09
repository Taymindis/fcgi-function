#Download base image NGINX
# docker build -t nginx_ffunc -f Dockerfile_ngx_ffunc .
# docker run -d -v ~/fcgi-function/DockerExample:/tmp -p 80:80 --name testffunc nginx_ffunc
FROM nginx

# Update Software repository, install more libraries if you need it
RUN apt-get update
#RUN ["apt-get", "install", "-y", "vim"]
#RUN ["apt-get", "install", "-y", "procps"]
#RUN ["apt-get", "install", "-y", "net-tools"]
RUN ["apt-get", "install", "-y", "wget"]
RUN ["apt-get", "install", "-y", "build-essential"]
RUN ["apt-get", "install", "-y", "make"]
RUN ["apt-get", "install", "-y", "cmake"]
RUN ["apt-get", "install", "-y", "git"]

WORKDIR "/opt"
RUN wget https://github.com/FastCGI-Archives/FastCGI.com/raw/master/original_snapshot/fcgi-2.4.1-SNAP-0910052249.tar.gz
RUN tar -zxvf fcgi-2.4.1-SNAP-0910052249.tar.gz
WORKDIR "/opt/fcgi-2.4.1-SNAP-0910052249"
RUN ./configure
RUN make && make install
WORKDIR "/opt"
RUN git clone https://github.com/Taymindis/fcgi-function.git
WORKDIR "/opt/fcgi-function"

RUN ldconfig
#RUN gcc -I./ examples/profile_service_daemonize.c ffunc.c ffunc.h -lfcgi -pthread -ldl -rdynamic -o ffuncsample
#RUN ./ffuncsample

#RUN sed -i '/listen[[:space:]]\+80;/a include fcgi_func.conf;' /etc/nginx/conf.d/default.conf


# Configure Services
#COPY profile_service_daemonize.c /tmp/profile_service_daemonize.c
#COPY ffunc_profile /etc/nginx/conf.d/ffunc_profile
#COPY start_ffunc.sh /tmp/start_ffunc.sh
#RUN chmod 755 /tmp/start_ffunc.sh
WORKDIR "/tmp"
CMD chmod 755 /tmp/start_ffunc.sh && ./start_ffunc.sh
