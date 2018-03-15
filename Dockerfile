FROM alpine:3.7 AS build

ENV LIBARCHIVE_VERSION 3.3.2

RUN apk add --update --no-cache \
      build-base \
      curl \
      zlib-dev \
      bzip2-dev \
      xz-dev \
      lz4-dev \
      acl-dev \
      libressl-dev \
      expat-dev \
  && mkdir -p /build/ \
  && cd /build \
  && curl -o - http://www.libarchive.org/downloads/libarchive-${LIBARCHIVE_VERSION}.tar.gz | tar zxf - \
  && cd libarchive-${LIBARCHIVE_VERSION} \
  && ./configure --without-xml2 \
  && make -j3

WORKDIR /build/overview-convert-archive
COPY Makefile Makefile
COPY src/ src/

RUN make


FROM alpine:3.7 AS base
RUN apk add --update --no-cache python3 py3-requests 

WORKDIR /app
COPY app/ /app/
COPY --from=build /build/overview-convert-archive/archive-to-multipart /app/
CMD [ "./run" ]


FROM base AS dev


FROM base AS test
WORKDIR /app
COPY /test/ /app/
CMD [ "./test" ]


FROM base AS production
