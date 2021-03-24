#include "Algorithms.h"
#include <cmath>

#ifdef USING_EASY_PROFILER
#include <easy/profiler.h>
#endif

using namespace std;

namespace PP2
{
vector<int> CountSort(const vector<Tank*>& in)
{
    vector<int> Counters(TANK_MAX_HEALTH + 1, 0);
    vector<int> Results;

    for (auto x : in)
        Counters.at(x->health <= 0 ? 0 : x->health)++;

    for (int i = 0; i < TANK_MAX_HEALTH + 1; ++i)
        if (Counters[i] != 0)
            for (int y = 0; y < Counters[i]; ++y)
                Results.push_back(i);

    return Results;
}

template <class T>
void LinkedList<T>::InsertValue(T value)
{
    auto* new_node = new Node<T>(value);

    if (head == nullptr || value <= head->value)
    {
        new_node->next = head;
        head = new_node;
        return;
    }

    Node<T>* current = head;
    while (current->next != nullptr && value >= current->next->value) { current = current->next; }

    //Add node
    new_node->next = current->next;
    current->next = new_node;
}

// -----------------------------------------------------------
// Sort tanks by health value using bucket sort
// -----------------------------------------------------------
template <>
vector<LinkedList<int>> LinkedList<int>::Sort(vector<Tank*>& input, int n_buckets)
{
    vector<LinkedList<int>> buckets(n_buckets);
    for (auto tank : input) { buckets.at(tank->health / n_buckets).InsertValue(tank->health); }
    return buckets;
}