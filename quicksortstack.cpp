#include <iostream>
#include <utility>
using namespace std;

const int N = 20;
int stackArr[N];
int top = -1;

void push(int val)
{
    if (top + 1 == N)
    {
        cout << "the stack is full";
        return;
    }
    stackArr[++top] = val;
}

int pop()
{
    if (top == -1)
    {
        cout << "the stack is empty";
        return -1;
    }
    int val = stackArr[top];
    top = top - 1;
    return val;
}

int partition(int arr[], int st, int end)
{
    int pivot = end;
    int i = st - 1;
    for (int j = st; j < end; j++)
    {
        if (arr[j] <= arr[pivot])
        {
            i++;
            swap(arr[i], arr[j]);
        }
    }
    i++;
    swap(arr[i], arr[pivot]);
    return i;
}

void quickSortUsingStack(int arr[], int n)
{
    int end = n - 1;
    push(0);
    push(end);
    while (top != -1)
    {
        int high = pop();
        int low = pop();
        if (low >= high)
            continue;
        int pivot = partition(arr, low, high);
        push(low);
        push(pivot - 1);
        push(pivot + 1);
        push(high);
    }

    for (int i = 0; i <= end; i++)
    {
        cout << arr[i] << " ";
    }
}

int main()
{
    int arr[10] = {64, 25, 12, 22, 11, 90, 38, 47, 5, 73};
    int n = sizeof(arr) / sizeof(arr[0]);

    cout << "before the quicksort :";
    for (int i = 0; i < n; i++)
    {
        cout << arr[i] << " ";
    }
    cout << "After the quicksort ";
    quickSortUsingStack(arr, n);
    return 0;
}
