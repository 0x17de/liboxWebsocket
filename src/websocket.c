#include "websocket.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <endian.h>

#include <sys/stat.h>
#include <fcntl.h>


#define WS_KEY_SIZE 20

#define WS_STATE_CLEAN        0
#define WS_STATE_DISCONNECTED 1
#define WS_STATE_INITIALIZED  2
#define WS_STATE_CONNECTED    3
#define WS_STATE_SHAKEREQUEST 4


/* endianess */
const int isNetworkByteOrder() { const int bIsNetworkByteOrder = 1 == htonl(1); return bIsNetworkByteOrder; }
#define htonll(x) (isNetworkByteOrder() ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (isNetworkByteOrder() ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


struct websocket {
	int socket;
	int state;
	struct pollfd fd;

	ws_dataCallback onData;
	void* onDataLParam;
	ws_closeCallback onClose;
	void* onCloseLParam;
};


void* ws_create() {
	struct websocket* ws = calloc(1, sizeof(struct websocket));
	ws->socket = -1;
	ws->fd.events = POLLIN;
	return ws;
}

void ws_free(void* ws) {
	ws_disconnect(ws);
	free(ws);
}

int _ws_handle_websocket(struct websocket* ws, char* data, int dataLength) {
	struct ws_header header;
	char* nextPart;
	unsigned int payloadLength;
	size_t unmaskIndex;
	uint8_t mask;
	char maskingKey[4];

	header.finalFragment = data[0] >> 7 & 0x1;
	header.opcode = data[0] & 0xf;
	mask = data[1] >> 7 & 0x1;
	payloadLength = data[1] & 0x7f;

	assert(payloadLength <= 127);

	if (payloadLength < 126) {
		header.payloadLength = payloadLength;
		nextPart = data + 2;
	} else if (payloadLength == 126) {
		header.payloadLength = ntohs(*(uint16_t*)&data[2]);
		nextPart = data + 2 + 2;
	} else if (payloadLength == 127) {
		header.payloadLength = ntohll(*(uint64_t*)&data[2]);
		nextPart = data + 2 + 8;
	}
	header.data = nextPart;

	if (mask != 0) {
		memcpy(maskingKey, nextPart, 4);
		header.data += 4; /* mask */

		/* unmask data */
		for (unmaskIndex = 0; unmaskIndex < header.payloadLength; ++unmaskIndex)
			header.data[unmaskIndex] ^= maskingKey[unmaskIndex % 4];
	}

	ws->onData(ws, &header, ws->onDataLParam);

	return nextPart + payloadLength - data; /* return processed data count */
}

int ws_readLine(struct readLineResult* res) {
	char c;

	res->start = res->current;
	while (res->current != res->dataEnd) {
		c = *res->current;
		++res->current;

		if (c == '\n' || c == '\r') {
			res->end = res->current;
			if (c == '\r' && res->current != res->dataEnd && *res->current == '\n')
				++res->current; /* skip continuation of linebreak in next call */
			return 1; /* OK: Found something */
		}
	}
	res->end = res->current;

	return res->start != res->end; /* OK: return true on non-empty ending data */
}

int _ws_handle_upgrade(struct websocket* ws, char* data, int dataLength) {
	int lineLength;
	struct readLineResult line = {0};
	line.current = data;
	line.data = data;
	line.dataEnd = data + dataLength;

	while (ws_readLine(&line) != 0) { /* read till empty line */
		char* buffer;
		lineLength = line.end - line.start;
		
		buffer = strndup(line.start, lineLength);
		printf("Ok: %s\n", buffer);
		free(buffer);
		/* TODO: check some headers */
	}

	/* TODO: on success */
	ws->state = WS_STATE_CONNECTED;

	return line.current - data; /* processed byte count */
}

int ws_handle(void* ws_, int timeout) {
	struct websocket* ws;
	int pollResult;
	int dataLength;
	char* data;
	int readResult;
	int amountOfProcessedData;

	ws = ws_;
	pollResult = poll(&ws->fd, 1, timeout);
	if (pollResult > 0) {
		ioctl(ws->socket, FIONREAD, &dataLength);
	} else if (pollResult == 0) {
		return 0; /* OK: Timeout */
	} else if (ws->socket < 0) {
		return -1; /* ERROR */
	}

	data = malloc(dataLength);
	readResult = read(ws->socket, data, dataLength);
	if (readResult != dataLength) {
		free(data);
		return -2; /* ERROR: Could not read enough data */
	}

	if (readResult > 0) {
		amountOfProcessedData = 0;
		while (amountOfProcessedData < dataLength) {
			char* currentData;
			int currentLength;
			
			currentData = data + amountOfProcessedData;
			currentLength = dataLength - amountOfProcessedData;

			if (ws->state == WS_STATE_CONNECTED) {
				amountOfProcessedData += _ws_handle_websocket(ws, currentData, currentLength);
			} else if (ws->state == WS_STATE_SHAKEREQUEST) {
				amountOfProcessedData += _ws_handle_upgrade(ws, currentData, currentLength);
			} else {
				fprintf(stderr, "Unprocessed data of state %d!\n", ws->state);
				printf("-----\n%s\n-----\n", data);
				amountOfProcessedData = dataLength;
			}
		}
	} else if (readResult == 0) {
		ws->state = WS_STATE_DISCONNECTED;
		ws_disconnect(ws_);
		ws->onClose(ws_, ws->onCloseLParam);
	} else { /* ERROR: Negative read results */
		ws->state = WS_STATE_DISCONNECTED;
		ws_disconnect(ws_);
		ws->onClose(ws_, ws->onCloseLParam);
		return -3; /* ERROR: Read error */
	}

	free(data);
	return 1; /* OK: Data */
}

void ws_setDataCallback(void* _ws, ws_dataCallback onData, void* lParam) {
	struct websocket* ws = _ws;
	ws->onData = onData;
	ws->onDataLParam = lParam;
}

void ws_setCloseCallback(void* _ws, ws_closeCallback onClose, void* lParam) {
	struct websocket* ws = _ws;
	ws->onClose = onClose;
	ws->onCloseLParam = lParam;
}

void ws_disconnect(void* _ws) {
	struct websocket* ws = _ws;

	if (ws->state == WS_STATE_CONNECTED) {
		/*
		 * TODO: shake it off
		 */
	}
	if (ws->state != WS_STATE_CLEAN) {
		close(ws->socket);
		ws->socket = -1;
	}

	ws->state = WS_STATE_CLEAN;
}

void ws_base64encode(char* dest, const char* source, int length) {
	const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789*/=";
	int rest;
	int parts;
	const char* in_iterator;
	char* out_iterator;

	parts = length / 3;
	rest = length % 3;

	in_iterator = source;
	out_iterator = dest;
	for(; parts > 0; --parts) {
		out_iterator[0] = base64chars[in_iterator[0] >> 2 & 0x3f];
		out_iterator[1] = base64chars[(in_iterator[0] << 4 & 0x30) + (in_iterator[1] >> 4 & 0xf)];
		out_iterator[2] = base64chars[(in_iterator[1] << 2 & 0x3c) + (in_iterator[2] >> 6 & 0x3)];
		out_iterator[3] = base64chars[in_iterator[2] & 0x3f];
		in_iterator += 3;
		out_iterator += 4;
	}
	if (rest > 0) {
		int a,b,c;
		a = b = c = 64; /* '=' */
		
		if (rest >= 1) {
			a = in_iterator[0] >> 2 & 0x3f;
			b = in_iterator[0] << 4 & 0x30;
			if (rest >= 2) {
				b |= in_iterator[1] >> 4 & 0xf;
				c = in_iterator[1] << 2 & 0x3c;
				if (rest >= 3) {
					c |= in_iterator[2] >> 6 & 0x3;
				}
			}
		}
		out_iterator[0] = base64chars[a];
		out_iterator[1] = base64chars[b];
		out_iterator[2] = base64chars[c];
		out_iterator[3] = '=';
		out_iterator += 4;
	}
	out_iterator[0] = 0;
}

void ws_genrandom(char* dest, int length) {
	int fd_random = open("/dev/urandom", O_RDONLY);
	assert(fd_random != -1);
	read(fd_random, dest, length);
	close(fd_random);
}

int ws_connect(void* ws_, const char* hostname, int port, const char* uri) {
	struct websocket* ws;
	const struct hostent* host;
	char** hostAddress;
	int s;
	struct sockaddr_in6 sin6 = {0};
	struct sockaddr_in sin4 = {0};
	struct sockaddr* saddr;
	int connectResult;
	const char* rawRequest;
	int requestLength;
	char* request;
	ssize_t sendResult;
	char rawKey[WS_KEY_SIZE];
	char key[((WS_KEY_SIZE+2)/3*4)+1];
	
	ws = ws_;

	if (ws->state != WS_STATE_CLEAN)
		return -1; /* ERROR: Call disconnect first */

	host = gethostbyname(hostname);
	hostAddress = host->h_addr_list;

	if (host->h_addrtype == AF_INET) {
		saddr = (struct sockaddr*)&sin4;
		sin4.sin_port = htons(port);
	} else if (host->h_addrtype == AF_INET6) {
		saddr = (struct sockaddr*)&sin6;
		sin6.sin6_port = htons(port);
	}
	saddr->sa_family = host->h_addrtype;

	/* create accordingly */
	ws->fd.fd = ws->socket = s = socket(host->h_addrtype, SOCK_STREAM, 0);
	ws->state = WS_STATE_INITIALIZED;

	/* try to connect to any resolved ip */
	while (*hostAddress != 0) {
		if (host->h_addrtype == AF_INET)
			memcpy(&sin4.sin_addr.s_addr, *hostAddress, 4);
		if (host->h_addrtype == AF_INET6)
			memcpy(&sin6.sin6_addr.s6_addr, *hostAddress, 16);
		
		connectResult = connect(s, saddr, sizeof(*saddr));
		if (connectResult == 0) break; /* OK */

		++hostAddress; /* next address */
	}
	if (*hostAddress == 0) return -2; /* ERROR: Could not connect */

	rawRequest =
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Upgrade: websocket\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Key: %s\r\n"
		"\r\n";

	/* websocket key */
	ws_genrandom(rawKey, WS_KEY_SIZE);
	ws_base64encode(key, rawKey, WS_KEY_SIZE);

	/* calculate length of request */
	requestLength = snprintf(0, 0, rawRequest, uri, hostname, key);
	if (requestLength <= 0)
		return -3; /* ERROR: Could not send data */

	/* prepare request */
	request = malloc(requestLength);
	snprintf(request, requestLength, rawRequest, uri, hostname, key);

	/* send and clean request */
	sendResult = send(s, request, requestLength, 0);
	free(request);

	if (sendResult != requestLength)
		return -4; /* ERROR: Could not send data */

	ws->state = WS_STATE_SHAKEREQUEST; /* waiting for completion of shake */
	return 0; /* OK */
}
