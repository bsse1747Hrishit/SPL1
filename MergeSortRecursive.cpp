#include <iostream>
using namespace std;

void mergeRecursive(int a[], int b[], int merged[], int i, int j, int k)
{
    if (i == 3 && j == 3)
        return;

    if (i == 3)
        merged[k] = b[j++];
    else if (j == 3)
        merged[k] = a[i++];
    else if (a[i] < b[j])
        merged[k] = a[i++];
    else
        merged[k] = b[j++];

    mergeRecursive(a, b, merged, i, j, k + 1);
}

int main()
{
    int a[3] = {1, 3, 5};
    int b[3] = {2, 4, 6};
    int merged[6];

    mergeRecursive(a, b, merged, 0, 0, 0);

    cout << "Merged array: ";
    for (int x = 0; x < 6; x++)
        cout << merged[x] << " ";

    return 0;
}
