#include <iostream>
using namespace std;

struct Node
{
    int data;
    Node *left;
    Node *right;

    Node(int value) : data(value), left(nullptr), right(nullptr) {}
};

Node *createNode(int value)
{
    return new Node(value);
}

Node *mergeTrees(Node *root1, Node *root2)
{
    if (root1 == nullptr && root2 == nullptr)
        return nullptr;

    if (root1 == nullptr)
        return createNode(root2->data);

    if (root2 == nullptr)
        return createNode(root1->data);

    Node *newRoot = createNode(root1->data + root2->data);

    newRoot->left = mergeTrees(root1->left, root2->left);
    newRoot->right = mergeTrees(root1->right, root2->right);

    return newRoot;
}

void inorder(Node *root)
{
    if (root == nullptr)
        return;

    inorder(root->left);
    cout << root->data << " ";
    inorder(root->right);
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
    Node *root1 = createNode(1);
    root1->left = createNode(3);
    root1->right = createNode(2);
    root1->left->left = createNode(5);

    Node *root2 = createNode(2);
    root2->left = createNode(1);
    root2->right = createNode(3);
    root2->left->right = createNode(4);
    root2->right->right = createNode(7);

    Node *mergedRoot = mergeTrees(root1, root2);

    cout << "Merged Tree (Inorder): ";
    inorder(mergedRoot);

    deleteTree(root1);
    deleteTree(root2);
    deleteTree(mergedRoot);

    return 0;
}
