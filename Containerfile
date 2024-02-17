FROM fedora:latest AS build
RUN dnf -y update
RUN dnf -y install \
	musl-clang \
	musl-libc-static \
	make \
	;

ADD . /tcp_server

RUN make -C /tcp_server CC='musl-clang -static'

FROM scratch AS tcp_server
COPY --from=build /tcp_server/tcp_server /usr/local/bin/tcp_server
