#include <iostream>
#include <vector>
#include <climits>
using namespace std;

void mergeArrays(int a[], int n, int b[], int m)
{
    vector<int> left(n + 1);
    vector<int> right(m + 1);

    for (int i = 0; i < n; i++)
    {
        left[i] = a[i];
    }

    for (int i = 0; i < m; i++)
    {
        right[i] = b[i];
    }

    // sentinels
    left[n] = INT_MAX;
    right[m] = INT_MAX;

    int i = 0, j = 0;

    vector<int> merged(n + m);

    for (int k = 0; k < n + m; k++)
    {
        if (left[i] <= right[j])
        {
            merged[k] = left[i];
            i++;
        }
        else
        {
            merged[k] = right[j];
            j++;
        }
    }

    cout << "Merged Array: ";

    for (int x = 0; x < n + m; x++)
    {
        cout << merged[x] << " ";
    }
    cout << "\n";
}

int main()
{
    int a[] = {1, 3, 5, 7};
    int b[] = {2, 4, 6, 8};

    int n = 4;
    int m = 4;

    mergeArrays(a, n, b, m);

    return 0;
}
