#include <iostream>
#include <string>
#include <cctype>
using namespace std;

const int MAX = 100;

int stackArr[MAX];
int top = -1;

void push(int x)
{
    stackArr[++top] = x;
}

int pop()
{
    return stackArr[top--];
}

int main()
{
    string postfix;
    int a, b;

    cout << "Enter postfix: ";
    cin >> postfix;

    for (size_t i = 0; i < postfix.length(); i++)
    {
        char c = postfix[i];
        if (isdigit(static_cast<unsigned char>(c)))
        {
            push(c - '0');
        }
        else
        {
            b = pop();
            a = pop();

            switch (c)
            {
            case '+': push(a + b); break;
            case '-': push(a - b); break;
            case '*': push(a * b); break;
            case '/': push(a / b); break;
            }
        }
    }

    cout << "Result: " << pop() << "\n";
    return 0;
}
