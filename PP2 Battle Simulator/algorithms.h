#pragma once

#include "Grid.h"
#include "defines.h"
#include "explosion.h"
#include "particle_beam.h"
#include "rocket.h"
#include "smoke.h"
#include "tank.h"
#include <cstdint>
#include <iostream>

namespace PP2
{
/**
 * Node Used by the Bucketsort/LinkedList
 * @tparam T
 */
template <class T>
class Node
{
  public:
    Node() : value(nullptr), next(nullptr) {}

    Node(T value) : value(value), next(nullptr) {}

    ~Node()
    {
        if (next != nullptr) delete next;
    }

    T value;
    Node<T>* next;
};

 /*
 * Bucketsort/LinkedList
 * @tparam T
 */
template <class T>
class LinkedList
{
  public:
    LinkedList() : head(nullptr){};

    ~LinkedList()
    {
        delete head;
    }
    /**
 * Insert a value into the list
 * @param value Value to insert
 */
    void InsertValue(T value);

    /**
 * Sort the list
 * @param input List to sort
 * @param n_buckets Number of buckets to use for sorting
 * @return Sorted list
 */
    static std::vector<LinkedList<T>> Sort(std::vector<Tank*>& input, int n_buckets);

    /**
 * The head
 */
    Node<T>* head;
};
