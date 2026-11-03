#include <iostream>
using namespace std;

void recursiveSort(int arr[], int n)
{
    if (n == 1)
        return;

    for (int j = 0; j < n - 1; j++)
    {
        if (arr[j] > arr[j + 1])
        {
            int temp = arr[j];
            arr[j] = arr[j + 1];
            arr[j + 1] = temp;
        }
    }

    recursiveSort(arr, n - 1);
}

int main()
{
    int arr[5] = {5, 2, 9, 1, 3};

    recursiveSort(arr, 5);

    cout << "Sorted array: ";
    for (int i = 0; i < 5; i++)
        cout << arr[i] << " ";
    cout << endl;

    return 0;
}
