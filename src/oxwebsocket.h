#ifndef WEBSOCKET_H
#define WEBSOCKET_H


#include <stdint.h>


#define WS_OP_CONTINUATION	0x0
#define WS_OP_TEXT			0x1
#define WS_OP_BINARY		0x2
/* 0x3 - 0x7 RESERVED */
#define WS_OP_CLOSE			0x8
#define WS_OP_PING			0x9
#define WS_OP_PONG			0xA
/* 0xB - 0xF Control Frames */


struct ws_frame {
	uint8_t finalFragment;
	uint8_t opcode;
	uint8_t masked;
	uint64_t payloadLength;
	char* data;
};

struct readLineResult {
	char* current;
	char* data, *dataEnd;
	char* start, *end;
};


typedef void(*ws_dataCallback)(void* ws, struct ws_frame* header, void* lParam);
typedef void(*ws_closeCallback)(void* ws, void* lParam);


void* ws_create();
void ws_free(void* ws);

int ws_handle(void* ws, int timeout);
void ws_send(void* ws_, struct ws_frame* header);
void ws_sendText(void* ws_, char* text, uint64_t length);
void ws_sendData(void* ws_, char* data, uint64_t length);

void ws_setDataCallback(void* ws, ws_dataCallback onData, void* lParam);
void ws_setCloseCallback(void* ws, ws_closeCallback onClose, void* lParam);
void ws_disconnect(void* ws);
int ws_connect(void* ws, const char* hostname, int port, const char* uri);

/* UTILS */
void ws_base64encode(char dest[], const char source[], int length);
int ws_readLine(struct readLineResult* res);
void ws_genrandom(char* dest, int length);


#endif /* WEBSOCKET_H */
