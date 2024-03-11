FROM alpine:3.19 AS build
RUN apk update && apk add clang make
COPY . /tcp_server
RUN make -C /tcp_server CC='clang -static'

# This container is only for building tcp_server - it should not run anything.
CMD sleep inf
