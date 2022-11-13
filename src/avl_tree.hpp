#ifndef AVLTree_HPP
#define AVLTree_HPP

#include <vector>
#include <stdint.h>
#include <limits>

static const uint64_t AVL_EMPTY_NODE = std::numeric_limits<uint64_t>::max();
static const int AVL_NOT_FOUND = -1;

// Templated in case new indexes are made in the future

template <typename T>
struct AVLNode
{
    AVLNode<T> *left;
    AVLNode<T> *right;

    T value;
    int count;
    int height;

    AVLNode(T value);

    AVLNode<T> *left_rotate();
    AVLNode<T> *right_rotate();
    void update_values();
    int balance_factor();
};

// AVL tree to store indices of all chunk epoch windows (which are also file names)
// Every operation is at a fast O(log n) complexity, with slightly more read-biased due to 
// flatter structure than a red-black tree.
template <typename T>
struct AVLTree
{
    size_t total_nodes;
    AVLNode<T> *root;

    AVLTree();
    ~AVLTree();

    T &operator[](std::size_t idx);
    void balance(std::vector<AVLNode<T> **> &nodes);
    void clear();
    bool empty();
    size_t size();
    void erase(T &value);
    void insert(T &value);
    bool exists(T &value);
    int find(T &value);
    uint64_t first_higher(AVLNode<T> *node, T &value);
    uint64_t first_lower(AVLNode<T> *node, T &value);
    void deserialize(std::vector<T> &data);
    std::vector<T> serialize();
    std::vector<T> serialize_inorder();
    std::vector<T> serialize_inorder_incl();
    std::vector<T> serialize_inorder_from(T &value);
};

#endif