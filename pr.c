#include <stdio.h>
#include <string.h>

#define MAX 100

int main()
{
    char str[MAX];
    char stack[MAX];
    int top = -1;

    printf("Enter brackets: ");
    scanf("%s", str);

    for (int i = 0; i < strlen(str); i++)
    {
        // If opening bracket, push into stack
        if (str[i] == '(' || str[i] == '[' || str[i] == '{')
        {
            top++;
            stack[top] = str[i];
        }

        // If closing bracket
        else if (str[i] == ')' || str[i] == ']' || str[i] == '}')
        {
            // Stack is empty
            if (top == -1)
            {
                printf("Not Balanced\n");
                return 0;
            }

            // Check matching brackets
            if ((str[i] == ')' && stack[top] == '(') ||
                (str[i] == ']' && stack[top] == '[') ||
                (str[i] == '}' && stack[top] == '{'))
            {
                top--;  // Pop
            }
            else
            {
                printf("Not Balanced\n");
                return 0;
            }
        }
    }

    // If stack is empty
    if (top == -1)
        printf("Balanced\n");
    else
        printf("Not Balanced\n");

    return 0;
}