#include "src/avl_tree.hpp"
#include <utility>
#include <iostream>
#include <filesystem>
#include <vector>
#include <stdint.h>
#include <limits>

bool test_insert_one()
{
    AVLTree<uint64_t> tree;
    uint64_t val = 100;

    if (tree.exists(val))
        return false;

    tree.insert(val);

    if (!tree.exists(val))
        return false;

    return tree[tree.find(val)] == val;
}

bool test_insert_many()
{
    AVLTree<uint64_t> tree;
    uint64_t val1 = 100;
    uint64_t val2 = 200;

    if (tree.exists(val1))
        return false;

    if (tree.exists(val2))
        return false;

    tree.insert(val1);
    tree.insert(val2);

    if (!tree.exists(val1))
        return false;

    if (!tree.exists(val2))
        return false;

    return tree[tree.find(val1)] == val1 && tree[tree.find(val2)] == val2;
}

bool test_serialization()
{
    AVLTree<uint64_t> tree;
    uint64_t val1 = 100;
    uint64_t val2 = 200;
    uint64_t val3 = 300;

    tree.insert(val1);
    tree.insert(val2);
    tree.insert(val3);

    std::vector<uint64_t> serialized = tree.serialize();
    return serialized[0] == val2 && serialized[1] == val1 && serialized[2] == val3;
}

bool test_deserialization()
{
    AVLTree<uint64_t> tree;
    uint64_t val1 = 100;
    uint64_t val2 = 200;
    uint64_t val3 = 300;

    tree.insert(val1);
    tree.insert(val2);
    tree.insert(val3);

    std::vector<uint64_t> serialized = tree.serialize();
    if (serialized[0] != val2 || serialized[1] != val1 || serialized[2] != val3)
        return false;

    tree.deserialize(serialized);
    return tree.root->value == val2 &&
           tree.root->left->value == val1 &&
           tree.root->right->value == val3 &&
           !tree.root->left->left &&
           !tree.root->left->right &&
           !tree.root->right->left &&
           !tree.root->right->right;
}

bool test_first_higher()
{
    AVLTree<uint64_t> tree;
    uint64_t val1 = 100;
    uint64_t val2 = 200;
    uint64_t val3 = 300;

    tree.insert(val1);
    tree.insert(val2);
    tree.insert(val3);

    uint64_t check_val1 = 90;
    uint64_t check_val2 = 190;
    uint64_t check_val3 = 290;
    return tree.first_higher(tree.root, check_val1) == val1 &&
           tree.first_higher(tree.root, check_val2) == val2 &&
           tree.first_higher(tree.root, check_val3) == val3;
}

bool test_first_lower()
{
    AVLTree<uint64_t> tree;
    uint64_t val1 = 100;
    uint64_t val2 = 200;
    uint64_t val3 = 300;

    tree.insert(val1);
    tree.insert(val2);
    tree.insert(val3);

    uint64_t check_val1 = 110;
    uint64_t check_val2 = 210;
    uint64_t check_val3 = 310;
    return tree.first_lower(tree.root, check_val1) == val1 &&
           tree.first_lower(tree.root, check_val2) == val2 &&
           tree.first_lower(tree.root, check_val3) == val3;
}

std::pair<int, int> test_avl()
{
    std::cout << "Testing AVL Tree...\n";
    std::vector<std::pair<std::string, bool>> tests;

    tests.push_back({"Inserting one element", test_insert_one()});
    tests.push_back({"Inserting many elements", test_insert_many()});
    tests.push_back({"Serializing AVL tree", test_serialization()});
    tests.push_back({"Deserializing AVL tree", test_deserialization()});
    tests.push_back({"Testing search for first higher node", test_first_higher()});
    tests.push_back({"Testing search for first lower node", test_first_lower()});

    int passed = 0;
    for (std::pair<std::string, bool> &test : tests)
    {
        if (test.second)
        {
            std::cout << "PASSED: " << test.first << "\n";
            passed++;
        }
        else
        {
            std::cout << "FAILED: " << test.first << "\n";
        }
    }

    std::cout << "Finished testing AVL Tree: " << passed << "/" << tests.size() << " cases passed.\n\n";
    return {passed, tests.size()};
}