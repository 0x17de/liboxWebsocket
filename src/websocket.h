#ifndef WEBSOCKET_H
#define WEBSOCKET_H


#include <stdint.h>


struct ws_header {
	uint8_t finalFragment;
	uint8_t opcode;
	uint64_t payloadLength;
	char* data;
};

struct readLineResult {
	char* current;
	char* data, *dataEnd;
	char* start, *end;
};


typedef void(*ws_dataCallback)(void* ws, struct ws_header* header, void* lParam);
typedef void(*ws_closeCallback)(void* ws, void* lParam);


void* ws_create();
void ws_free(void* ws);

int ws_handle(void* ws, int timeout);

void ws_setDataCallback(void* ws, ws_dataCallback onData, void* lParam);
void ws_setCloseCallback(void* ws, ws_closeCallback onClose, void* lParam);
void ws_disconnect(void* ws);
int ws_connect(void* ws, const char* hostname, int port, const char* uri);

/* UTILS */
void ws_base64encode(char dest[], const char source[], int length);
int ws_readLine(struct readLineResult* res);

#endif /* WEBSOCKET_H */
