#include <stdio.h>
#include <process.h>
#include <dos.h>

#define BOUNDARY 32768
#define segment(addr) (FP_SEG(m) + ((FP_OFF(m)+15) >> 4));
#define round(value,boundary) (((value) + (boundary) - 1) & ~((boundary) - 1))

char *malloc(),*realloc();

char prog[] = "gnuplot";
char corscreen[] = "CORSCREEN=0";

main()
{
register unsigned int segm,start;
char *m;
	if (!(m = malloc(BOUNDARY))) {
		printf("malloc() failed\n");
		exit(1);
	}
	segm = segment(m);
	start = round(segm,BOUNDARY/16);

	if (realloc(m,BOUNDARY+(start-segm)*16) != m) {
		printf("can't realloc() memory\n");
		exit(2);
	}

	if ((segm = start >> 11) >= 8) {
		printf("not enough room in first 256K\n");
		exit(3);
	}

	corscreen[sizeof(corscreen)-2] = '0' + segm;
	if (putenv(corscreen))
		perror("putenv");

	if (spawnlp(P_WAIT,prog,prog,NULL))
		perror("spawnlp");
}
