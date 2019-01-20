# B+ Tree

![-std=c++14](https://img.shields.io/badge/std-c++14-blue.svg)
![cmake passed](https://img.shields.io/badge/cmake-Pass-brightgreen.svg)
![g++ 7.3 passed](https://img.shields.io/badge/g++_7.3-Pass-brightgreen.svg)
![clang++ 6.0 passed](https://img.shields.io/badge/clang++_6.0-Pass-brightgreen.svg)
![vs 15.9 passed](https://img.shields.io/badge/vs_15.9-Pass-brightgreen.svg)

A header only B+ Tree container `BPlusTree` whose APIs are similar to `std::set` in STL.

Structure:
1. The elements in a non-leaf node are maximum of its respective children;
2. All leaves are linked as a bidirectional linked list;
3. All elements are in leaves node;
4. No duplicated keys;
5. The elements and pointers in the node of `BPlusTree` are stored in red-black trees, that is, `std::map`.


For example
If `order = 3`, after inserting `1 2 3 -5 -3 4 2 5 6 7`, the tree is like:

``` plain-text   
layer=1:          [3,            7]
                   |             |
layer=2:      [1,      3]    [5,    7]
               |       |      |     |
layer=3:   [-5,-3,1] [2,3]  [4,5] [6,7]
```

When removing element, there are two different strategies for two cases: `order = 2` or `order > 2`.
In the former case, it attempts to merge a node with its slibing node first, but int the latter case, 
borrowing a element from left or right node in the same layer is first choice.

## Example

A simple example:

```cpp
#include <iostream>
#include <cassert>

#include "BPlusTree.h"

int main()
{
    BPlusTree<int, 3, std::less<int>> tree(std::less<int>{});
    tree.insert(0);
    tree.insert(3);
    tree.insert(1);
    tree.insert(2);
    tree.insert(4);
    tree.insert(5);
    assert(tree.find(3) != tree.end()); // True
    tree.print();
    // Should be:
    // [1,3,5]
    // [0,1][2,3][4,5]

    return 0;
}

```

More details can be found in `BPlusTree/example.cpp`.

## APIs

Classes:

```cpp
// <key's type, order of the tree, 
template <typename T, std::size_t order = 3u, typename Compare = std::less<T>>
class BPlusTree;

// Bidirectional iterator
// <BPlusTree, is the iterator const or not>
template <typename _BPlusTree, bool is_const>
struct BPlusTreeIterator
{
    Tree* tree;                     // pointer to BPlusTree
    Node* node;                     // pointer to the node in the tree
    RecordIterator record_iterator; // std::map's iterator to the element in node
}
```

Functions and classes in `BPlusTree`:

```cpp
// ---------- Node in the BPlusTree ----------
struct Node
{
    Container records;      // elements and pointers to children
    bool is_leaf;    // is leaf node or not
    Node* next;   // right node in the same layer
    Node* pre;    // left node in the same layer
    Node* parent; // parent node
};

// ---------- Constructors & Destructor----------

// default one, the comparator is std::less<key_type>
BPlusTree(); 
// one with user specified comparator
BPlusTree(const Compare& keycomp);

// clear all nodes
~BPlusTree();

// ---------- Lookup ----------

iterator find(const key_type& key);
const_iterator find(const key_type& key) const;

iterator lower_bound(const key_type& key);
const_iterator lower_bound(const key_type& key) const;

iterator upper_bound(const key_type& key);
const_iterator upper_bound(const key_type& key) const;

std::pair<iterator, iterator> equal_range(const key_type& key);
std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const;

// ---------- Iterators ---------- 

iterator begin();
iterator end()

const_iterator begin() const
const_iterator end() const

const_iterator cbegin() const
const_iterator cend() const

// ----------Modifiers ----------

// Return <iterator to inserted key, insertion happended or not
std::pair<iterator, bool> insert(const key_type& key);

iterator erase(iterator pos);
iterator erase(const_iterator pos)

void clear();

// ---------- Capacity ----------

bool empty() const;

sizt_type size() const;

// ---------- Observer ----------

// print the tree
void print() const;

```

## License

[<img src="https://img.shields.io/badge/Lisence-GPL%20v3-red.svg" alt="GPLv3" >](http://www.gnu.org/licenses/gpl-3.0.html)

The code is licensed under GNU General Public License v3.0 (GPLv3).
