#include <iostream>
#include <utility>
using namespace std;

void selectionSort(int arr[], int start, int n)
{
    if (start >= n - 1)
        return;

    int minIndex = start;

    for (int i = start + 1; i < n; i++)
    {
        if (arr[i] < arr[minIndex])
            minIndex = i;
    }

    swap(arr[start], arr[minIndex]);

    selectionSort(arr, start + 1, n);
}

void printArray(int arr[], int n)
{
    for (int i = 0; i < n; i++)
        cout << arr[i] << " ";
}

int main()
{
    int arr[] = {5, 2, 9, 1, 3};
    int n = 5;

    selectionSort(arr, 0, n);

    printArray(arr, n);

    return 0;
}
