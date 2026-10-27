#include <stdio.h>

int binarySearch(int arr[], int left, int right, int key)
{
    if(left > right)
    {
        return -1;
    }

    int mid = (left + right) / 2;

    if(arr[mid] == key)
    {
        return mid;
    }
    else if(key < arr[mid])
    {
        return binarySearch(arr, left, mid - 1, key);
    }
    else
    {
        return binarySearch(arr, mid + 1, right, key);
    }
}

int main()
{
    int arr[] = {1, 3, 5, 7, 9, 11};

    int n = 6;
    int key = 7;

    int result = binarySearch(arr, 0, n - 1, key);

    if(result != -1)
    {
        printf("Element found at index: %d", result);
    }
    else
    {
        printf("Element not found");
    }

    return 0;
}