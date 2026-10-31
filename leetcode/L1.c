// temperature convertor 
/*#include <stdio.h>
#include<stdlib.h>
double * temperature_convertor(double celsius, int *returnsize)
{
    double *ans=(double*)malloc(2*sizeof(double));
    if (ans == NULL)
{
    *returnsize = 0;
    return NULL;
}
    ans[0]=celsius+273.15;
    ans[1]=celsius*1.80+32.00;
    *returnsize=2;
    return ans;
}
int main ()
{
    double celsius;
    int returnsize=0;
    printf("ENTER the temperature->");
    scanf("%lf",&celsius);
    double * result =temperature_convertor(celsius,&returnsize);
    if (result == NULL)
    {
        printf("Memory allocation failed!");
        return 1;
    }
    printf("IN KELVIN->%lf",result[0]);
    printf("IN FARHENHEIT->%lf",result[1]);
    free(result);
    return 0;
}*/

#include <stdio.h>
#include <stdlib.h>

double* temperature_convertor(double celsius, int* returnsize)
{
    double* ans = malloc(2 * sizeof(double));

    if (ans == NULL)
    {
        *returnsize = 0;
        return NULL;
    }

    ans[0] = celsius + 273.15;
    ans[1] = celsius * 1.80 + 32.00;

    *returnsize = 2;

    return ans;
}

int main()
{
    double celsius;
    int returnsize = 0;

    printf("Enter temperature: ");
    scanf("%lf", &celsius);

    double* result = temperature_convertor(celsius, &returnsize);

    if (result == NULL)
    {
        printf("Memory allocation failed!");
        return 1;
    }

    printf("Return size: %d\n", returnsize);
    printf("Kelvin: %.2lf\n", result[0]);
    printf("Fahrenheit: %.2lf\n", result[1]);

    free(result);

    return 0;
}
