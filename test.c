#include <stdlib.h>
#include <stdio.h>

int main()
{
	char *s;
	s=malloc(10);
	free(s);
	return 0;
}

