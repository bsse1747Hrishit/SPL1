#include <iostream>
#include <string>
#include <cctype>
using namespace std;

const int N = 10;

int stackArr[N];
int top = -1;

void push(int value)
{
    if (top + 1 == N)
    {
        cout << "the stack is overflow ";
        return;
    }
    top = top + 1;
    stackArr[top] = value;
}

int pop()
{
    if (top == -1)
    {
        cout << "the stack is underflow";
        return -1;
    }
    int value = stackArr[top];
    top = top - 1;
    return value;
}

void evaluatePostfix(const string &exp)
{
    for (size_t i = 0; i < exp.length(); i++)
    {
        char c = exp[i];
        if (c == ' ')
        {
            continue;
        }
        else if (isdigit(static_cast<unsigned char>(c)))
        {
            push(c - '0');
        }
        else if (c == '+' || c == '-' || c == '*' || c == '/')
        {
            int b = pop();
            int a = pop();
            if (c == '+')
                push(a + b);
            if (c == '-')
                push(a - b);
            if (c == '*')
                push(a * b);
            if (c == '/')
            {
                if (b == 0)
                {
                    cout << "the division cannot be done";
                }
                else
                {
                    push(a / b);
                }
            }
        }
    }
    cout << "the result is " << stackArr[top];
}

int main()
{
    string exp;
    getline(cin, exp); // safe replacement for gets()
    cout << "the postfix expression is " << exp;
    evaluatePostfix(exp);
    return 0;
}
