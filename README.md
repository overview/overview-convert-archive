This is an [Overview](https://github.com/overview/overview-server) converter.
It extracts the contents of archives.

What it does
============

1. Writes stdin to input.blob in the CWD
2. Extracts it, streaming output as multipart/form-data on stdout

Usage
=====

In an Overview cluster, you'll want to use the Docker container:

`docker run -e POLL_URL=http://worker-url:9032/Archive overview/overview-convert-archive:2.1.0`

Developing
==========

`./dev` will connect to the `overviewserver_default` network and run with
`POLL_URL=http://overview-worker:9032/Archive`.

`./run-tests` will run tests by spinning up a fake HTTP server.

Design decisions
----------------

`src/archive-to-multipart.c` is in C. That's because we rely on
[libarchive](https://github.com/libarchive/libarchive) to do the extraction, and
the wrappers around it don't inspire confidence. (The Python wrapper in
particular includes comments that memory may not be freed.)

`archive-to-multipart` output is deterministic and it's a single file. That
makes testing easy.

`test/all-tests.py` runs within the Dockerfile. That's to make Docker Hub an
integration-test framework.

License
-------

This software is Copyright 2011-2018 Jonathan Stray, and distributed under the
terms of the GNU Affero General Public License. See the LICENSE file for details.
