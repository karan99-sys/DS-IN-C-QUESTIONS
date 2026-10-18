#include <stdio.h>
int main()
{
    int n, i, k;
    printf("enter the size of  array: ");
    scanf("%d", &n);
    int arr[n];
    printf("enter the %d elements for array: ", n);
    for (i = 0; i < n; i++)
    {
        scanf("%d", &arr[i]);
    }
    printf("the elemnents array are ");
    for (i = 0; i < n; i++)
    {
        printf("%d ", arr[i]);
    }
    printf("\nenter rotation times: ");
    scanf("%d", &k);
    k = k % n;  // if n=5 and we have to 7 times roation it will be same at 5 times rotation so for saving time we will do  k%n
    int rr[n];
    for (i = 0; i < n; i++)
    {
        rr[i] = arr[(i + k) % n];// 7-5=2 so it will directly revrse it from index  2 save time 
    }
    for (i = 0; i < n; i++)
    {
        arr[i] = rr[i];
    }
    printf("the rotated elemnents array are ");
    for (i = 0; i < n; i++)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
    return 0;
}