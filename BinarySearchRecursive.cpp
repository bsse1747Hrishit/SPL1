
#include <iostream>
using namespace std;

struct Node
{
    int data;
    Node *next;
};

Node *insertatbegining(Node *head, int val)
{
    Node *newnode = new Node;
    newnode->data = val;
    newnode->next = head;
    return newnode;
}

Node *insertatend(Node *head, int val)
{
    Node *newnode = new Node;
    newnode->data = val;
    newnode->next = nullptr;

    if (head == nullptr)
        return newnode;

    Node *ptr = head;

    while (ptr->next != nullptr)
    {
        ptr = ptr->next;
    }

    ptr->next = newnode;
    return head;
}

Node *insertatmiddle(Node *head, int pos, int val)
{
    if (head == nullptr)
        return nullptr;

    Node *newnode = new Node;
    newnode->data = val;

    Node *ptr = head;
    int a = 1;

    while (ptr->next != nullptr && a < pos)
    {
        ptr = ptr->next;
        a++;
    }

    newnode->next = ptr->next;
    ptr->next = newnode;

    return head;
}

Node *insertsor(Node *head, int val)
{
    Node *newnode = new Node;
    newnode->data = val;

    if (head == nullptr || val < head->data)
    {
        newnode->next = head;
        return newnode;
    }

    Node *ptr = head;

    while (ptr->next != nullptr &&
           val > ptr->next->data)
    {
        ptr = ptr->next;
    }

    newnode->next = ptr->next;
    ptr->next = newnode;

    return head;
}

Node *insertbeforegivennode(Node *head, Node *node, int val)
{
    if (head == nullptr || node == nullptr)
        return head;

    if (head == node)
        return insertatbegining(head, val);

    Node *ptr = head;

    while (ptr->next != nullptr && ptr->next != node)
    {
        ptr = ptr->next;
    }

    if (ptr->next == node)
    {
        Node *newnode = new Node;
        newnode->data = val;
        newnode->next = node;
        ptr->next = newnode;
    }

    return head;
}

void search(Node *head, int val)
{
    Node *ptr = head;
    int a = 1;

    while (ptr != nullptr)
    {
        if (val == ptr->data)
        {
            cout << "The node has been found. It is at position "
                 << a << endl;
            return;
        }

        ptr = ptr->next;
        a++;
    }

    cout << "Not found" << endl;
}

void traverse(Node *head)
{
    Node *ptr = head;

    while (ptr != nullptr)
    {
        cout << ptr->data << " ";
        ptr = ptr->next;
    }

    cout << endl;
}

int main()
{
    Node *head = new Node;
    Node *second = new Node;
    Node *third = new Node;

    head->data = 10;
    head->next = second;

    second->data = 12;
    second->next = third;

    third->data = 15;
    third->next = nullptr;

    head = insertatbegining(head, 8);
    head = insertatend(head, 16);
    head = insertatmiddle(head, 3, 13);
    head = insertsor(head, 14);
    head = insertbeforegivennode(head, second, 11);

    search(head, 13);

    cout << "Linked list: ";
    traverse(head);

    return 0;
}
