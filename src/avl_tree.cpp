#include "avl_tree.hpp"
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <queue>

template <typename T>
AVLTree<T>::AVLTree()
    : total_nodes(0), root(nullptr) {}

template <typename T>
AVLNode<T>::AVLNode(T value)
    : value(value), count(1), height(1), left(nullptr), right(nullptr) {}

template <typename T>
void AVLNode<T>::update_values()
{
    int left_count = left ? left->count : 0;
    int right_count = right ? right->count : 0;
    count = left_count + right_count + 1;

    int left_height = left ? left->height : 0;
    int right_height = right ? right->height : 0;
    height = std::max(left_height, right_height) + 1;
}

template <typename T>
int AVLNode<T>::balance_factor()
{
    int left_height = left ? left->height : 0;
    int right_height = right ? right->height : 0;
    return left_height - right_height;
}

template <typename T>
void AVLTree<T>::insert(T &value)
{
    AVLNode<T> **indirect = &root;
    std::vector<AVLNode<T> **> path;

    while (*indirect)
    {
        path.push_back(indirect);

        if ((*indirect)->value > value)
            indirect = &((*indirect)->left);
        else
            indirect = &((*indirect)->right);
    }

    *indirect = new AVLNode<T>(value);
    path.push_back(indirect);

    balance(path);
    total_nodes++;
}

template <typename T>
void AVLTree<T>::erase(T &value)
{
    AVLNode<T> **indirect = &root;
    std::vector<AVLNode<T> **> path;

    while (*indirect && (*indirect)->value != value)
    {
        path.push_back(indirect);

        if ((*indirect)->value > value)
            indirect = &((*indirect)->left);
        else
            indirect = &((*indirect)->right);
    }

    if (!*indirect)
        return;
    else
        path.push_back(indirect);

    std::size_t index = path.size();

    if (!(*indirect)->left && !(*indirect)->right)
    {
        delete *indirect;
        *indirect = nullptr;
        path.pop_back();
    }

    else if (!(*indirect)->right)
    {
        AVLNode<T> *to_rem = *indirect;

        (*indirect) = (*indirect)->left;
        delete to_rem;

        path.pop_back();
    }

    else
    {
        AVLNode<T> **successor = &((*indirect)->right);

        while ((*successor)->left)
        {
            path.push_back(successor);
            successor = &((*successor)->left);
        }

        if (*successor == (*indirect)->right)
        {
            (*successor)->left = (*indirect)->left;

            AVLNode<T> *to_rem = *indirect;
            *indirect = *successor;
            delete to_rem;
        }

        else
        {
            AVLNode<T> *tmp = *path.back();
            AVLNode<T> *suc = *successor;

            tmp->left = (*successor)->right;
            suc->left = (*indirect)->left;
            suc->right = (*indirect)->right;

            delete *indirect;
            *indirect = suc;
            path[index] = &(suc->right);
        }
    }

    balance(path);
    total_nodes--;
}

template <typename T>
void AVLTree<T>::balance(std::vector<AVLNode<T> **> &nodes)
{
    std::reverse(nodes.begin(), nodes.end());

    for (auto ind : nodes)
    {
        (*ind)->update_values();

        if ((*ind)->balance_factor() >= 2 && (*ind)->left->balance_factor() >= 0)
            *ind = (*ind)->right_rotate();

        else if ((*ind)->balance_factor() >= 2)
        {
            (*ind)->left = (*ind)->left->left_rotate();
            *ind = (*ind)->right_rotate();
        }

        else if ((*ind)->balance_factor() <= -2 && (*ind)->right->balance_factor() <= 0)
            *ind = (*ind)->left_rotate();

        else if ((*ind)->balance_factor() <= -2)
        {
            (*ind)->right = ((*ind)->right)->right_rotate();
            *ind = (*ind)->left_rotate();
        }
    }
}

template <typename T>
bool AVLTree<T>::empty()
{
    return total_nodes == 0;
}

template <typename T>
size_t AVLTree<T>::size()
{
    return total_nodes;
}

template <typename T>
int AVLTree<T>::find(T &value)
{
    AVLNode<T> *direct = root;
    int idx = 0;

    while (direct && direct->value != value)
    {
        if (direct->value > value)
            direct = direct->left;
        else
        {
            idx += (direct->left ? direct->left->count : 0) + 1;
            direct = direct->right;
        }
    }

    if (!direct)
        return AVL_NOT_FOUND;

    else
        return idx + (direct->left ? direct->left->count : 0);
}

template <typename T>
uint64_t AVLTree<T>::first_lower(AVLNode<T> *node, T &value)
{
    if (!node)
        return AVL_EMPTY_NODE;

    if (!node->left && !node->right && node->value > value)
        return AVL_EMPTY_NODE;

    if ((node->value <= value && !node->right) || (node->value <= value && node->right->value > value))
        return node->value;

    if (node->value >= value)
        return first_lower(node->left, value);
    else
        return first_lower(node->right, value);
}

template <typename T>
T &AVLTree<T>::operator[](std::size_t idx)
{
    AVLNode<T> *cur = root;
    int left = cur->left ? cur->left->count : 0;

    while (left != idx)
    {
        if (left < idx)
        {
            idx -= left + 1;

            cur = cur->right;
            left = cur->left ? cur->left->count : 0;
        }

        else
        {
            cur = cur->left;
            left = cur->left ? cur->left->count : 0;
        }
    }

    return cur->value;
}

template <typename T>
uint64_t AVLTree<T>::first_higher(AVLNode<T> *node, T &value)
{
    if (!node)
        return AVL_EMPTY_NODE;

    if (!node->left && !node->right && node->value < value)
        return AVL_EMPTY_NODE;

    if ((node->value >= value && !node->left) || (node->value >= value && node->left->value < value))
        return node->value;

    if (node->value <= value)
        return first_higher(node->right, value);
    else
        return first_higher(node->left, value);
}

template <typename T>
AVLNode<T> *AVLNode<T>::left_rotate()
{
    AVLNode<T> *r = right;
    right = right->left;
    r->left = this;
    this->update_values();
    r->update_values();
    return r;
}

template <typename T>
AVLNode<T> *AVLNode<T>::right_rotate()
{
    AVLNode<T> *l = left;
    left = left->right;
    l->right = this;
    this->update_values();
    l->update_values();
    return l;
}

template <typename T>
inline bool AVLTree<T>::exists(T &value) { return find(value) > AVL_NOT_FOUND; }

template <typename T>
void AVLTree<T>::clear()
{
    std::vector<AVLNode<T> *> stack;

    if (root)
        stack.push_back(root);

    while (!stack.empty())
    {
        AVLNode<T> *node = stack.back();
        stack.pop_back();

        if (node->left)
            stack.push_back(node->left);

        if (node->right)
            stack.push_back(node->right);

        total_nodes--;
        delete node;
    }

    root = nullptr;
}

template <typename T>
AVLTree<T>::~AVLTree() { clear(); }

template <typename T>
void AVLTree<T>::deserialize(std::vector<T> &data)
{
    if (data.empty())
    {
        root = nullptr;
        return;
    }

    root = new AVLNode<T>(data[0]);
    total_nodes = 1;

    std::queue<AVLNode<T> *> bfs_q;
    bfs_q.push(root);

    uint32_t added = 1;

    while (added < data.size())
    {
        int level_size = bfs_q.size();
        for (int i = 0; i < level_size; i++)
        {
            AVLNode<T> *front = bfs_q.front();
            bfs_q.pop();

            // left
            if (data[added] != AVL_EMPTY_NODE)
            {
                AVLNode<T> *l = new AVLNode<T>(data[added]);
                front->left = l;
                bfs_q.push(l);
            }

            added++;

            // right
            if (data[added] != AVL_EMPTY_NODE)
            {
                AVLNode<T> *r = new AVLNode<T>(data[added]);
                front->right = r;
                bfs_q.push(r);
            }

            added++;
        }
    }
}

template <typename T>
std::vector<T> AVLTree<T>::serialize()
{
    std::queue<AVLNode<T> *> bfs_q;
    bfs_q.push(root);
    std::vector<T> serialized;

    while (!bfs_q.empty())
    {
        int level_size = bfs_q.size();
        for (int i = 0; i < level_size; i++)
        {
            AVLNode<T> *front = bfs_q.front();
            bfs_q.pop();

            if (front)
                serialized.push_back(front->value);
            else
            {
                serialized.push_back(AVL_EMPTY_NODE);
                continue;
            }

            if (front->left)
                bfs_q.push(front->left);
            else
                bfs_q.push(nullptr);

            if (front->right)
                bfs_q.push(front->right);
            else
                bfs_q.push(nullptr);
        }
    }

    return serialized;
}

template <typename T>
void inorder_recursive_from(AVLNode<T> *root, std::vector<T> &result, T &value)
{
    if (!root)
        return;

    if (root->left)
        inorder_recursive_from(root->left, result, value);

    if (root->value > value)
        result.push_back(root->value);

    if (root->right)
        inorder_recursive_from(root->right, result, value);
}

template <typename T>
void inorder_recursive_from_incl(AVLNode<T> *root, std::vector<T> &result, T &value)
{
    if (!root)
        return;

    if (root->left)
        inorder_recursive_from(root->left, result, value);

    if (root->value >= value)
        result.push_back(root->value);

    if (root->right)
        inorder_recursive_from(root->right, result, value);
}

template <typename T>
std::vector<T> AVLTree<T>::serialize_inorder()
{
    uint64_t zero = 0;
    std::vector<T> result;
    inorder_recursive_from(root, result, zero);
    return result;
}

template <typename T>
std::vector<T> AVLTree<T>::serialize_inorder_incl()
{
    uint64_t zero = 0;
    std::vector<T> result;
    inorder_recursive_from_incl(root, result, zero);
    return result;
}

template <typename T>
std::vector<T> AVLTree<T>::serialize_inorder_from(T &value)
{
    std::vector<T> result;
    inorder_recursive_from(root, result, value);
    return result;
}

// for now we're only using it for epoch
template class AVLTree<uint64_t>;
template class AVLNode<uint64_t>;