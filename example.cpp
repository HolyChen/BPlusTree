#include <iostream>
#include <functional>

#include "BPlusTree.h"

int main()
{
    int n;
    std::cout << "How many elements do you want to insert: ";
    std::cin >> n;

    int x;
    BPlusTree<int, 3, std::less<int>> t(std::less<int>{});
    for (int i = 0; i < n; i++)
    {
        std::cin >> x;
        auto insert_result = t.insert(x);
        if (!insert_result.second)
        {
            std::cout << "Key " << x << " exitses" << std::endl;
        }
        else
        {
            std::cout << "After insert " << x << ":" << std::endl;
            t.print();
        }
        std::cout << std::endl;
    }


    for (auto v : t)
    {
        std::cout << v << " ";
    }
    std::cout << std::endl << std::endl;

    std::cout << "How many elements do you want to erase: ";
    std::cin >> n;

    for (size_t i = 0; i < n; i++)
    {
        std::cin >> x;
        auto iter = t.find(x);
        if (iter != t.end())
        {
            t.erase(iter);
            std::cout << "After erase " << x << std::endl;
            t.print();
            std::cout << std::endl;
        }
        else
        {
            std::cout << "Key " << x << " doesn't exits\n" << std::endl;
        }
    }

    return 0;
}