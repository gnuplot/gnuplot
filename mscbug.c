#include <stdio.h>

main()
{
double x;

	printf("look at the fourth, fifth and sixth columns\n");
	printf("%%e\t\t %%f\t\t %%g\t %%.g\t %%.1g\t %%.2g\t %%.3g\n\n");
	for ( x = 1e-6; x < 1e+4; x *= 10.0 )
		printf("%e\t %f\t %g\t %.g\t %.1g\t %.2g\t %.3g\n",
			x,x,x,x,x,x,x);
}
