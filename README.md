# liboxWebsocket
A simple to use websocket library written in C.

OpenSSL will be supported. Currently a library is generated via automake.
Sending pings from time to time is not handled yet (need to look up docs first); Pongs are handled.

## Setup via Git
``````
git clone https://github.com/0x17de/liboxWebsocket
cd liboxWebsocket

aclocal
libtoolize
automake -a
autoconf
./configure
make
# optional:
make install
``````
