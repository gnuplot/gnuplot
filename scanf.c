#include <stdio.h>

main()
{
    float x,y;

    sscanf("0 12", "%f %f\n", &x, &y);
    printf("%f, %f  should be %f, %f\n", x, y, 0., 12.);

    sscanf("00 12", "%f %f\n", &x, &y);
    printf("%f, %f  should be %f, %f\n", x, y, 0., 12.);
}
