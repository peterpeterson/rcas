FROM trzeci/emscripten:latest

RUN mkdir -p /home/rcas/build
WORKDIR /home/rcas
COPY . /home/rcas

ENTRYPOINT ["/emsdk_portable/entrypoint"]

CMD ["sh", "./build.sh"]