#include "websocket.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void onData(void* ws, struct ws_frame* header, void* lParam) {
	char* str = strndup(header->data, header->payloadLength);
	printf("Data: %s\n", str);
	free(str);
}
void onClose(void* ws, void* lParam) {
	printf("Close.\n");
}

int main() {
	int handleResult;

	void* ws = ws_create();
	ws_setCloseCallback(ws, onClose, 0);
	ws_setDataCallback(ws, onData, 0);

	puts("Connecting.\n");
	if (ws_connect(ws, "localhost", 12321, "/") == 0) {
		puts("Connected.\n");
		while (handleResult = ws_handle(ws, 1000), handleResult >= 0) {
			if (handleResult > 0) { /* anything received */
				char *s = strndup("send", 5); /* data must be mutable if masking enabled */
				ws_sendString(ws, s, 5); /* answer with test string */
				free(s);
			}
		}
	} else {
		puts("Error connecting.\n");
	}
	puts("End.\n");
	ws_free(ws);

	return 0;
}

