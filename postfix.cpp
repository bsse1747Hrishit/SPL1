#include <iostream>
#include <string>
#include <cctype>
using namespace std;

const int MAX = 100;

char stackArr[MAX];
int top = -1;

void push(char x)
{
    stackArr[++top] = x;
}

char pop()
{
    return stackArr[top--];
}

int precedence(char x)
{
    if (x == '+' || x == '-')
        return 1;
    if (x == '*' || x == '/')
        return 2;
    if (x == '^')
        return 3;
    return 0;
}

int main()
{
    string infix, postfix;

    cout << "Enter infix: ";
    cin >> infix;

    for (size_t i = 0; i < infix.length(); i++)
    {
        char c = infix[i];
        if (isalnum(static_cast<unsigned char>(c)))
        {
            postfix += c;
        }
        else if (c == '(')
        {
            push(c);
        }
        else if (c == ')')
        {
            while (stackArr[top] != '(')
                postfix += pop();
            pop(); // remove '('
        }
        else
        {
            while (top != -1 && precedence(stackArr[top]) >= precedence(c))
                postfix += pop();
            push(c);
        }
    }

    while (top != -1)
        postfix += pop();

    cout << "Postfix: " << postfix << "\n";
    return 0;
}
