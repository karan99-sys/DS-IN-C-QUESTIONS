#include<stdio.h>
#include <stdlib.h>
struct node
{
    int data;
    struct node *next;
};
struct node *head=NULL;
void create(int value)
{
    struct node *newnode;
    newnode=(struct node*)malloc(sizeof(struct node));
    newnode->data=value;
    newnode->next=NULL;
    head=newnode;
}
void insertB(int value)
{
    struct node *newnode;
    newnode=(struct node*)malloc(sizeof(struct node));
    newnode->data=value;
    newnode->next=head;
    head=newnode;
}
void insertL(int value)
{
    struct node *newnode,*temp;
    newnode=(struct node*)malloc(sizeof(struct node));
    newnode->data=value;
    newnode->next=NULL;
    if(head==NULL)
    {
        head=newnode;
        return;
    }
    temp=head;
    while(temp->next!=NULL)
    {
        temp=temp->next;
    }
    temp->next=newnode;    
}
void deleteb()
{
    struct node *newnode,*temp;
    if(head==NULL)
    {
        printf("empty\n");
        return;
    }
    temp=head;
    head=head->next;
    free(temp) ;  
}
void deletel()
{
    struct node *prev,*temp;
    if(head==NULL)
    {
        printf("empty\n");
        return;
    }
    if(head->next==NULL)
    {
        free(head);
        head=NULL;
        return;
    }
    temp=head;
    while(temp->next!=NULL)
    {
        prev=temp;
        temp=temp->next;
    }
    prev->next=NULL;

    free(temp) ;  
}
void display()
{
    struct node *temp;
    if(head==NULL)
    {
        printf("empty\n");
        return;
    }
    temp =head;
    while(temp!=NULL)
    {
        printf("the value is->%d",temp->data);
        temp=temp->next;
        
    }
}
int main()
{
    create(10);
    insertB(20);
    insertB(2);
    insertB(24); 
    insertL(50); 
    insertL(90);
    display();
 
    deleteb();
    deleteb();
    display();
    deletel();
    display();
    deleteb();
    deletel();
    display();

}