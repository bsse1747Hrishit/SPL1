#include <iostream>
using namespace std;

struct Node
{
    int data;
    Node* next;

    Node(int value) : data(value), next(nullptr) {}
};

// Traverses the circular list exactly once, starting from ptr
void linkedlistTraversal(Node* ptr)
{
    if (ptr == nullptr)
        return;

    Node* temp = ptr;
    do
    {
        cout << "Element: " << temp->data << endl;
        temp = temp->next;
    } while (temp != ptr);
}

int main()
{
    Node* head = new Node(7);
    Node* second = new Node(8);
    Node* third = new Node(9);
    Node* fourth = new Node(10);

    head->next = second;
    second->next = third;
    third->next = fourth;
    fourth->next = head; // makes the list circular

    linkedlistTraversal(head);

    delete head;
    delete second;
    delete third;
    delete fourth;

    return 0;
}
