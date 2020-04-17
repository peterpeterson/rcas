FROM trzeci/emscripten:latest

RUN mkdir -p /home/mcsc/build
WORKDIR /home/mcsc
COPY . /home/mcsc

ENTRYPOINT ["/emsdk_portable/entrypoint"]

CMD ["sh", "./build.sh"]