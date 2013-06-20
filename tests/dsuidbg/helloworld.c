#include <stdlib.h>
#include <stdio.h>
#define __USE_GNU


#include "helloworld_dsui.h"


int main(int argc, char** argv)
{


	DSUI_BEGIN(&argc, &argv);


	printf("HelloWorld!\n");

	/* Record the end of main() */
	DSTRM_EVENT(FUNC_MAIN, IN_MAIN_FUNC, 0);


	DSUI_CLEANUP();


	exit(EXIT_SUCCESS);
}
