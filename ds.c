#include<stdio.h>
#define max 6
int top;
int stk[max];
void create()
{
   int  top=-1;
  
}
void push(int value)
{
    if(top==max-1)
    {
        printf("it is full\n");
    }
    else if(top==-1)
    {
        printf("empty stack\n");
        top=0;
        return;
    }
    else
    {
        top++;
        stk[top]=value;
        printf("the inserted value%d\n",value);
    }
}
void pop()
{
    if(top==-1)
    {
        printf("empty stack\n");
        top=0;

    } 
    else 
    {
        printf("the poped value is %d\n",stk[top--]);
    }
}
void peek()
{
     if(top==-1)
    {
        printf("empty stack\n");
        top=0;

    } 
    else 
    {
        printf("the peek value is %d\n",stk[top]);
    }

}
void display()
{
      if(top==-1)
    {
        printf("empty stack\n");
        top=0;

    } 
    else 
    {
        printf("the elements in stack are\n");
        for(int i=top;i>=0;i--)
        {
            printf("%d-",stk[i]);
        }

    }
    
}
int main()
{
    create();
    push(20);
    push(2);
    push(24); 
    push(50); 
    push(90);
    display();
    peek();
    pop();
    peek();
    pop();
    pop();
    pop();
    pop();
    display();

}