#include <iostream>
#include <vector>
using namespace std;

const int N = 100;

int stackArr[N];
int top = -1;

void push(int val)
{
    if (top == N - 1)
        return;
    stackArr[++top] = val;
}

int pop()
{
    if (top == -1)
        return -1;
    return stackArr[top--];
}

void merge(int arr[], int st, int end, int mid)
{
    int n1 = mid - st + 1;
    int n2 = end - mid;

    vector<int> l(n1), r(n2); // vector instead of variable-length arrays

    for (int i = 0; i < n1; i++)
        l[i] = arr[st + i];

    for (int i = 0; i < n2; i++)
        r[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = st;

    while (i < n1 && j < n2)
    {
        if (l[i] <= r[j])
            arr[k++] = l[i++];
        else
            arr[k++] = r[j++];
    }

    while (i < n1)
        arr[k++] = l[i++];

    while (j < n2)
        arr[k++] = r[j++];
}

void mergeSortStack(int arr[], int n)
{
    push(0);
    push(n - 1);

    while (top != -1)
    {
        int high = pop();
        int low = pop();

        // merge operation marker: stack holds [mid, high, low, -1]
        // -1 is already in 'high' and low is already in 'low',
        // so only high and mid remain to be popped
        if (high == -1)
        {
            high = pop();
            int mid = pop();
            merge(arr, low, high, mid);
            continue;
        }

        if (low >= high)
            continue;

        int mid = (low + high) / 2;

        // push merge task (marker)
        push(mid);
        push(high);
        push(low);
        push(-1);

        // push right half
        push(mid + 1);
        push(high);

        // push left half
        push(low);
        push(mid);
    }
}

int main()
{
    int arr[] = {64, 25, 12, 22, 11, 90, 38, 47, 5, 73};
    int n = sizeof(arr) / sizeof(arr[0]);

    cout << "Before: ";
    for (int i = 0; i < n; i++)
        cout << arr[i] << " ";
    cout << "\n";

    mergeSortStack(arr, n);

    cout << "After : ";
    for (int i = 0; i < n; i++)
        cout << arr[i] << " ";
    cout << "\n";

    return 0;
}
