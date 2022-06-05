#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCKS_CNT	50
#define BUFFER_SZ	1048576

int main(int argc, char* argv[]) 
{
	char *pBuff[BLOCKS_CNT];
	for (int ind = 0; ind < BLOCKS_CNT; ++ind)
	{
		pBuff[ind] = malloc(BUFFER_SZ);
		if (pBuff[ind] == NULL)
			printf("\nCannot allocate block %d\n", ind);
		else
        {
			memset(pBuff[ind], 0, BUFFER_SZ);
            if(ind)
			    printf(" %d", ind);
            else
                printf("Allocating block %d", ind);    
        }
	}
    printf("\n");

	for (int ind = 0; ind < BLOCKS_CNT; ++ind)
    {
		if (pBuff[ind] != NULL)
			free(pBuff[ind]);
        if(ind)
            printf(" %d", ind);
        else    
            printf("Freeing Block %d", ind);
    }
    printf("\n");

	return EXIT_SUCCESS;
}
