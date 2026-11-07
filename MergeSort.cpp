#include <iostream>
using namespace std;

int main()
{
    int a[3] = {1, 3, 5};
    int b[3] = {2, 4, 6};
    int merged[6];
    int i = 0, j = 0, k = 0;

    while (i < 3 && j < 3)
    {
        if (a[i] < b[j])
            merged[k++] = a[i++];
        else
            merged[k++] = b[j++];
    }

    while (i < 3)
        merged[k++] = a[i++];

    while (j < 3)
        merged[k++] = b[j++];

    cout << "Merged array: ";
    for (int x = 0; x < 6; x++)
        cout << merged[x] << " ";

    return 0;
}
