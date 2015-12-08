#include "websocket.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void onData(void* ws, struct ws_header* header, void* lParam) {
	char* str = strndup(header->data, header->payloadLength);
	printf("Data: %s\n", str);
	free(str);
}
void onClose(void* ws, void* lParam) {
	printf("Close.\n");
}

int main() {
	/* char in[] = "testtesttest";
	char* out = malloc(((strlen(in)+2)/3*4)+1);
	ws_base64_encode(out, in, strlen(in));
	puts(out);
	free(out); */
	
	int handleResult;

	void* ws = ws_create();
	ws_setCloseCallback(ws, onClose, 0);
	ws_setDataCallback(ws, onData, 0);

	puts("Connecting.\n");
	if (ws_connect(ws, "localhost", 12321, "/") == 0) {
		puts("Connected.\n");
		while (handleResult = ws_handle(ws, 1000), handleResult >= 0);
	} else {
		puts("Error connecting.\n");
	}
	puts("End.\n");
	ws_free(ws);
	
	return 0;
}

