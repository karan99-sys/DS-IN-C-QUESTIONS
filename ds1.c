#include<stdio.h>
#define max 6
int front=-1;
int rear=-1;
int qu[max];
void create()
{
    front=-1;
    rear=-1;
}
void enqueue(int value)
{
    if(rear==max-1)
    {
        printf("overflow\n");
        return;
    }
    if(front==-1)
    {
        front=0;
    }
        rear++;
        qu[rear]=value;
        printf("inserted value is %d\n",qu[rear]);
}
void dequeue()
{
    if(front==-1)
    {
        printf("invalid ! underflow condition\n");
        return;
     
    }
    printf("deleted value is %d\n",qu[front]);
    front++;
    if (front > rear)
    {
        front = -1;
        rear = -1;
    }
    
}
void display()
{
    if(front==-1)
    {
        printf("invalid ! empty condition\n");
        return;
     
    }
    else
    {
        for(int i=front;i<=rear;i++)
        {
        printf(" %d-",qu[i]);   
        }  
        printf("\n");  
    }
}
int main()
{
    create();
    enqueue(20);
    enqueue(2);
    enqueue(24); 
    enqueue(50); 
    enqueue(90);
    display();
    dequeue ();

    dequeue();
    dequeue();
    dequeue();
    dequeue();
    display();
    return 0;

}