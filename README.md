This is an [Overview](https://github.com/overview/overview-server) converter.
It extracts the contents of archives.

What it does
============

1. Downloads a task from Overview's task handler
2. Streams the zipfile from the task blob URL, extracts the stream, and sends
   extracted files and progress events in one big multipart/form-data POST to
   Overview.

Usage
=====

In an Overview cluster, you'll want to use the Docker container:

`docker run -e POLL_URL=http://worker-url:9032/Archive overview/overview-archive-converter:latest`

Developing
==========

`./dev` will connect to the `overviewserver_default` network and run with
`POLL_URL=http://overview-web:9032/Archive`.

`./test` will run tests by spinning up a fake HTTP server.

`src/stream-archive.cc` is in C++. That's because we rely on
[libarchive](https://github.com/libarchive/libarchive) to do the extraction, and
the wrappers around it don't inspire confidence. (The Python wrapper in
particular includes comments about how memory may not be freed.)
