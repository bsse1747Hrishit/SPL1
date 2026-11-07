#include <iostream>
#include <cstdlib>
using namespace std;

struct Node
{
    int data;
    Node *left;
    Node *right;

    Node(int value) : data(value), left(nullptr), right(nullptr) {}
};

int search(int inorder[], int start, int end, int value)
{
    for (int i = start; i <= end; i++)
        if (inorder[i] == value)
            return i;
    return -1;
}

Node *buildFromPostIn(int postorder[], int inorder[], int start, int end, int &postIndex)
{
    if (start > end)
        return nullptr;

    Node *node = new Node(postorder[postIndex]);

    postIndex--;

    if (start == end)
        return node;

    int inIndex = search(inorder, start, end, node->data);

    if (inIndex == -1)
    {
        cout << "Error: Invalid traversal input\n";
        exit(1);
    }

    // right subtree first, because postorder is read from the end
    node->right = buildFromPostIn(postorder, inorder, inIndex + 1, end, postIndex);
    node->left = buildFromPostIn(postorder, inorder, start, inIndex - 1, postIndex);

    return node;
}

void printInorder(Node *root)
{
    if (root == nullptr)
        return;
    printInorder(root->left);
    cout << root->data << " ";
    printInorder(root->right);
}

void deleteTree(Node *root)
{
    if (root == nullptr)
        return;
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

int main()
{
    int inorder[] = {4, 2, 5, 1, 3};
    int postorder[] = {4, 5, 2, 3, 1};

    int n = sizeof(postorder) / sizeof(postorder[0]);
    int postIndex = n - 1;

    Node *root = buildFromPostIn(postorder, inorder, 0, n - 1, postIndex);

    cout << "Inorder of constructed tree:\n";
    printInorder(root);

    deleteTree(root);

    return 0;
}
