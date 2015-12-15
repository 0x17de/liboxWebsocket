#include "websocket.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


void testBase64() {
	int i;
	char in[] = "testtesttest";
	int len = strlen(in);
	char* out = malloc(((len+2)/3*4)+1);

	const char* testResults[] = {
		"dGVzdHRlc3R0ZXN0",
		"dGVzdHRlc3R0ZXM=",
		"dGVzdHRlc3R0ZQ==",
		"dGVzdHRlc3R0"
	};

	for (i = 0; i < 4; ++i) {
		ws_base64encode(out, in, strlen(in));
		assert(strcmp(out, testResults[i]) == 0);
		--len;
		in[len] = 0;
	}

	free(out);
}

int main() {
	testBase64();
	return 0;
}

