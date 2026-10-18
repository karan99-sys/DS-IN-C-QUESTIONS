#include <stdio.h>
int main()
{
    int n=6,i,d,m;
    int s;
    int arr[n];
    printf("enter the elements");
    for(i=0;i<n;i++)
    {
        scanf("%d",&arr[i]);
    }
    printf("enter the purcahsing day");
    scanf("%d",&d);
    printf("enter the selling day");
    scanf("%d",&m);
    int max=0;
    s=arr[m]-arr[d];
    if (s>max)
    {
    max=s;
    }
    printf("maximum day is %d",max);
    return 0;

}