/*#include <stdio.h>
int main()
{
    int i,n,j,a,b,dis;
    printf("enter the size");
    scanf("%d",&n);
    int arr[n];
    printf("enter the elments");
    for(i=0;i<n;i++)
    {
        scanf("%d",&arr[i]);
    }
    printf("enter the elment a and b");        // NOT OPTIMAL MC AND TO LONG
    scanf("%d%d",&a,&b);
    for(i=0;i<n;i++)
    {
        if(arr[i]==a)
        {
            for(j=0;j<n;j++)
            {
                if(arr[j]==b)
                {
                   dis=j-i;
                }

            }
    
    
        } 
    }
    printf("the minimum distance is %d",dis);
}*/
#include <stdio.h>
int main()
{
    int n, i, a, b;
    int min_dist = -1;
    int last_idx = -1;
    printf("Enter size of array: ");
    scanf("%d", &n);
    int arr[n];
    printf("Enter %d elements: ", n);
    for (i = 0; i < n; i++)
    {
        scanf("%d", &arr[i]);
    }
    printf("Enter value of a and b: ");
    scanf("%d %d", &a, &b);
    for (i = 0; i < n; i++)
    {
        if (arr[i] == a || arr[i] == b) 
        {
            if (last_idx != -1 &&  arr[i] != arr[last_idx])       // i should +1 than last index and last index should not be -1
            {
                int dist = i - last_idx;          
                if (min_dist == -1 || dist < min_dist) 
                {   // for caluclaitng minimum distnace                
                    min_dist = dist;
                }
            }
            last_idx = i;   // updating last index
        }
    }
    if (min_dist == -1)
    {
        printf("-1\n");
    }
    else
    {
        printf("%d\n", min_dist);
    }
    return 0;
}
