#pragma once

#include <type_traits>
#include <functional>
#include <algorithm>
#include <queue>
#include <set>
#include <stdexcept>
#include <cassert>

template <typename _BPlusTree, bool is_const>
struct BPlusTreeIterator
{
    using Tree = typename std::conditional<!is_const, _BPlusTree, const _BPlusTree>::type;
    using key_type = typename _BPlusTree::key_type;

    using NodeType = typename _BPlusTree::node_type;

    using Node = typename std::conditional<!is_const, NodeType, const NodeType>::type;
    using RecordIterator = typename std::conditional<!is_const, typename _BPlusTree::RecordIterator, typename _BPlusTree::RecordConstIterator>::type;

    Tree* tree = nullptr;
    Node* node = nullptr;
    RecordIterator record_iterator;

    explicit BPlusTreeIterator(Tree* tree = nullptr, Node* node = nullptr, const RecordIterator& rit = RecordIterator())
        : tree(tree), node(node), record_iterator(rit)
    {
    }

    BPlusTreeIterator(const BPlusTreeIterator&) = default;

    // enable non-const -> const only
    template <typename AnoBPlusTreeIterator,
              typename = typename std::enable_if<
                std::is_same<typename AnoBPlusTreeIterator::Tree, _BPlusTree>::value && is_const>::type>
    BPlusTreeIterator(const AnoBPlusTreeIterator& ano)
    {
        tree = ano.tree;
        node = ano.node;
        record_iterator = ano.record_iterator;
    }

    const key_type& operator*() const
    {
        assert(tree != nullptr && node != nullptr);

        // the relative order should not be changed
        return record_iterator->first;
    }

    const key_type& operator->() const
    {
        assert(tree != nullptr && node != nullptr);

        return record_iterator.operator->();
    }


    BPlusTreeIterator& operator=(const BPlusTreeIterator& ano) = default;

    // enable non-const -> const only
    template <typename AnoBPlusTreeIterator,
        typename = typename std::enable_if<
        std::is_same<typename AnoBPlusTreeIterator::Tree, _BPlusTree>::value && is_const>::type >
    BPlusTreeIterator& operator=(const AnoBPlusTreeIterator& ano)
    {
        tree = ano.tree;
        node = ano.node;
        record_iterator = ano.record_iterator;
    }

    template <bool ano_is_const>
    bool operator==(const BPlusTreeIterator<_BPlusTree, ano_is_const>& ano) const
    {
        assert(tree == ano.tree);

        bool this_is_end = (tree == nullptr || node == nullptr);
        bool ano_is_end = (ano.tree == nullptr || ano.node == nullptr);

        if (this_is_end && ano_is_end)
        {
            return true;
        }
        else if (this_is_end ^ ano_is_end)
        {
            return false;
        }
        else
        {
            return tree == ano.tree && node == ano.node && record_iterator == ano.record_iterator;
        }
    }

    template <bool ano_is_const>
    bool operator!=(const BPlusTreeIterator<_BPlusTree, ano_is_const>& ano) const
    {
        return !(this->operator==(ano));
    }

    BPlusTreeIterator& operator++()
    {
        if (tree == nullptr || tree->m_root == nullptr)
        {
            // at the end, do nothing
            return *this;
        }

        record_iterator++;

        if (record_iterator == node->records.end())
        {
            node = node->next;
            if (node != &tree->m_header)
            {
                record_iterator = node->records.begin();
            }
            else
            {
                node = nullptr;
            }
        }

        return *this;
    }

    BPlusTreeIterator operator++(int)
    {
        BPlusTreeIterator old = *this;
        ++(*this);
        return old;
    }

    BPlusTreeIterator& operator--()
    {
        if (tree == nullptr || tree->m_root == nullptr || (node == tree->m_header.next && record_iterator == node->records.begin()))
        {
            // at the begin, do nothing
            return *this;
        }

        if (node == nullptr) // end of tree
        {
            node = tree->m_header.pre;
            record_iterator = (--node->records.end());
        }
        else
        {
            if (record_iterator == node->records.begin())
            {
                node = node->pre;
                record_iterator = (--node->records.end());
            }
            else
            {
                record_iterator--;
            }
        }
        return *this;
    }

    BPlusTreeIterator operator--(int)
    {
        BPlusTreeIterator old = *this;
        --(*this);
        return old;
    }
};

// key_type, order, comparator
template <typename T, std::size_t order = 3u, typename Compare = std::less<T>>
class BPlusTree
{
    static_assert(order > 1u, "The order of B+ Tree must be at least 2");

private:
    struct InnerCompare;
    struct Node;

public:
    using key_type = T;
    using size_type = std::size_t;
    using key_compare = Compare;
    const size_type half_order = (order + 1) / 2;
    const size_type half_order_when_erase = 2 > half_order ? 2 : half_order;

    using iterator = BPlusTreeIterator<BPlusTree, false>;
    using const_iterator = BPlusTreeIterator<BPlusTree, true>;
    using node_type = Node;

private:

    friend iterator;
    friend const_iterator;

    using KeyRawCompare = Compare;

    struct InnerCompare
    {
        using is_transparent = key_type; // enable transparent compare
        using RecordPair = std::pair<key_type, node_type*>;

        KeyRawCompare keycomp;

        InnerCompare(const Compare& keycomp)
            : keycomp(keycomp)
        {
        }

        bool operator()(const RecordPair& lhs, const RecordPair& rhs) const
        {
            return keycomp(lhs.first, rhs.first);
        }

        bool operator()(const key_type& lhs, const RecordPair& rhs) const
        {
            return keycomp(lhs, rhs.first);
        }

        bool operator()(const RecordPair& lhs, const key_type& rhs) const
        {
            return keycomp(lhs.first, rhs);
        }

        bool operator()(const key_type& lhs, const key_type& rhs) const
        {
            return keycomp(lhs, rhs);
        }
    };

    struct Node
    {
    public:
        using RecordPair = std::pair<key_type, Node*>; // key and child
        using Container = std::set<RecordPair, InnerCompare>;
        using RecordIterator = typename Container::iterator;
        using RecordConstIterator = typename Container::const_iterator;

    public:
        Container records;      // elements and pointer to children
        bool is_leaf = true;    // is leaf node or not
        Node* next = nullptr;   // right node in the same layer
        Node* pre = nullptr;    // left node in the same layer
        Node* parent = nullptr; // parent node

    public:
        Node() = default;
        Node(const InnerCompare& comp)
            : records(comp)
        {}

    };

    using RecordIterator = typename node_type::RecordIterator;
    using RecordConstIterator = typename node_type::RecordConstIterator;

public:
    BPlusTree()
        : m_innercomp(KeyRawCompare()), m_header(m_innercomp)
    {
        clear();
    }

    BPlusTree(const KeyRawCompare& keycomp)
        : m_innercomp(keycomp), m_header(m_innercomp)
    {
        clear();
    }

    // TODO: CopyContructor, CopyAssign, MoveConstructor, MoveAssign

    ~BPlusTree()
    {
        clear();
    }

    // return { iterator pointing to inserted key, inserted or not (key exitses) }
    std::pair<iterator, bool> insert(const key_type& key)
    {
        if (m_root == nullptr)
        {
            m_root = make_node();
            m_root->is_leaf = true;

            m_root->next = m_root->pre = &m_header;
            m_header.next = m_root;
            m_header.pre = m_root;

            m_root->records.insert(std::make_pair(key, nullptr));

            m_size++;

            return { make_iterator_uncheck(m_root, m_root->records.begin()), true };
        }

        auto cur = m_root;

        while (true)
        {
            if (!cur->is_leaf)
            {
                auto find_result = cur->records.lower_bound(key);

                if (find_result == cur->records.end())
                {
                    --find_result;
                    const_cast<key_type&>(find_result->first) = key; // store max one
                }
                cur = find_result->second;

            }
            else
            {
                auto find_result = cur->records.find(key);

                if (find_result != cur->records.end())
                {
                    return { make_iterator_uncheck(cur, find_result), false };
                }
                else
                {
                    find_result = cur->records.insert(std::make_pair(key, nullptr)).first;
                    m_size++;

                    if (cur->records.size() <= order)
                    {
                        return { make_iterator_uncheck(cur, find_result), true };
                    }
                    else // split the leaf
                    {
                        node_type* insert_node = nullptr;
                        auto split_result = split(cur);
                        if (m_innercomp(key, cur->records.begin()->first)) // in left
                        {
                            insert_node = split_result.second;
                            find_result = insert_node->records.find(key);
                        }
                        else
                        {
                            insert_node = cur;
                            find_result = insert_node->records.find(key);
                        }
                        cur = split_result.first;

                        while (cur != nullptr && cur->records.size() > order)
                        {
                            cur = split(cur).first;
                        }
                        return { make_iterator_uncheck(insert_node, find_result), true };
                    }
                }
            }
        }
    }

    iterator erase(iterator pos)
    {
        //assert(pos.tree == this);

        if (m_size == 0)
        {
            throw std::underflow_error("remove from empty BPlusTree");
        }

        m_size--;

        if (m_size == 0)
        {
            clear();
            return make_iterator();
        }
        else
        {
            auto to_delete_key = pos.record_iterator->first;

            node_type* node = pos.node;
            RecordIterator record_iterator = pos.record_iterator;

            while (erase_helper(node, record_iterator));

            while (!m_root->is_leaf && m_root->records.size() == 1)
            {
                auto tmp = m_root->records.begin()->second;
                delete m_root;
                m_root = tmp;
                tmp->parent = nullptr;
            }

            return lower_bound(to_delete_key);
        }

    }

    iterator erase(const_iterator pos)
    {
        return erase(make_iterator_uncheck(const_cast<Node*>(pos.node), const_cast<typename std::remove_cv<decltype(pos.node->records)>::type&>
            (pos.node->records).erase(pos.record_iterator, pos.record_iterator)));
    }

    iterator find(const key_type& key)
    {
        auto cur = m_root;
        while (cur != nullptr)
        {
            if (!cur->is_leaf)
            {
                auto find_result = cur->records.lower_bound(key);
                if (find_result == cur->records.end())
                {
                    return make_iterator();
                }
                cur = find_result->second;
            }
            else
            {
                auto find_result = cur->records.find(key);
                if (find_result != cur->records.end())
                {
                    return make_iterator_uncheck(cur, find_result);
                }
                else
                {
                    return make_iterator();
                }
            }
        }
        return make_iterator();
    }

    iterator lower_bound(const key_type& key)
    {
        node_type* last_split_point = nullptr;

        node_type* cur = m_root;
        while (cur != nullptr)
        {
            if (!cur->is_leaf)
            {
                auto find_result = cur->records.lower_bound(key);
                if (find_result == cur->records.end())
                {
                    return make_iterator();
                }
                cur = find_result->second;
            }
            else
            {
                auto find_result = cur->records.lower_bound(key);
                return make_iterator_uncheck(cur, find_result);
            }
        }

        return make_iterator();
    }

    iterator upper_bound(const key_type& key)
    {
        node_type* last_split_point = nullptr;

        node_type* cur = m_root;
        while (cur != nullptr)
        {
            if (!cur->is_leaf)
            {
                auto find_result = cur->records.upper_bound(key);
                if (find_result == cur->records.end())
                {
                    return make_iterator();
                }
                cur = find_result->second;
            }
            else
            {
                auto find_result = cur->records.upper_bound(key);
                return make_iterator(cur, find_result);
            }
        }

        return make_iterator();
    }

    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        auto lb = lower_bound(key);
        return std::make_pair(lower_bound(key), ++lb);
    }

    // --------------- iterator ---------------
    iterator begin()
    {
        return m_size == 0 ? end() : make_iterator_uncheck(m_header.next, m_header.next->records.begin());
    }

    iterator end()
    {
        return make_iterator();
    }

    const_iterator begin() const
    {
        return m_size == 0 ? end() : make_iterator_uncheck(m_header.next, m_header.next->records.begin());
    }

    const_iterator end() const
    {
        return make_iterator();
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    const_iterator cend() const
    {
        return end();
    }

    // --------------- const version ---------------

    const_iterator find(const key_type& key) const
    {
        return const_iterator(const_cast<BPlusTree*>(this)->find(key));
    }

    const_iterator lower_bound(const key_type& key) const
    {
        return const_iterator(const_cast<BPlusTree*>(this)->lower_bound(key));
    }

    const_iterator upper_bound(const key_type& key) const
    {
        return const_iterator(const_cast<BPlusTree*>(this)->upper_bound(key));
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
    {
        return const_iterator(const_cast<BPlusTree*>(this)->equal_range(key));
    }

    // ------------------------------------------------
    size_type size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return m_size == 0;
    }

    void clear()
    {
        clear_helper(m_root);
        m_root = nullptr;
        reset_header();
        m_size = 0u;
    }

    void print() const
    {
        if (m_root == nullptr)
        {
            return;
        }

        std::queue<node_type*> q;
        q.push(m_root);

        while (!q.empty())
        {
            auto cur = q.front();
            q.pop();

            std::cout << "[";
            auto iter = cur->records.begin(), end = cur->records.end();
            for (int i = 0; i < cur->records.size() - 1; i++)
            {
                std::cout << iter->first << ",";
                if (iter->second != nullptr)
                {
                    q.push(iter->second);
                }
                iter++;
            }
            std::cout << iter->first << "]";
            if (iter->second != nullptr)
            {
                q.push(iter->second);
            }
            if (cur->next == nullptr || cur->next == &m_header)
            {
                std::cout << "\n";
            }
        }
    }

private:
    void clear_helper(node_type* node)
    {
        if (node == nullptr)
        {
            return;
        }

        for (auto iter = node->records.begin(), end = node->records.end(); iter != end; iter++)
        {
            clear_helper(iter->second);
        }

        delete node;
    }

    void reset_header()
    {
        m_header.next = m_header.pre = &m_header;
    }

protected:
    node_type* make_node()
    {
        return new node_type(m_innercomp);;
    }

    iterator make_iterator(node_type* node, const RecordIterator& rit)
    {
        assert(node != nullptr);

        if (rit != node->records.end())
        {
            return make_iterator_uncheck(node, rit);
        }
        else // end
        {
            if (node->next != &m_header)
            {
                // next one
                return iterator{ this, node->next, node->next->records.begin() };
            }
            else
            {
                return iterator{ this, nullptr };
            }
        }
    }

    const_iterator make_iterator(const node_type* node, const RecordConstIterator& rit) const
    {
        assert(node != nullptr);

        if (rit != node->records.end())
        {
            return make_iterator_uncheck(node, rit);
        }
        else // end
        {
            if (node->next != &m_header)
            {
                return const_iterator{ this, node->next, node->next->records.begin() };
            }
            else
            {
                return const_iterator{ this, nullptr };
            }
        }
    }

    iterator make_iterator_uncheck(node_type* node, const RecordIterator& rit)
    {
        return iterator{ this, node, rit };
    }

    const_iterator make_iterator_uncheck(const node_type* node, const RecordConstIterator& rit) const
    {
        return const_iterator{ this, node, rit };
    }

    iterator make_iterator()
    {
        return iterator{ this };
    }

    const_iterator make_iterator() const
    {
        return const_iterator{ this };
    }

    // Return: inserted parent, new leaf node
    std::pair<node_type*, node_type*> split(node_type* leaf_node)
    {
        // split to left one 
        node_type* left = make_node();

        auto iter = leaf_node->records.begin(), end = leaf_node->records.end();
        for (size_type i = 0; i < half_order; i++)
        {
            if (iter->second != nullptr)
            {
                iter->second->parent = left;
            }
            left->records.insert(left->records.end(), std::move(*iter));
            iter = leaf_node->records.erase(iter);
        }
        key_type key = iter->first;

        left->is_leaf = leaf_node->is_leaf;

        left->next = leaf_node;
        if (leaf_node->pre != nullptr)
        {
            left->pre = leaf_node->pre;
            leaf_node->pre->next = left;
        }
        leaf_node->pre = left;

        node_type* parent = leaf_node->parent;
        if (parent == nullptr) // root
        {
            // root node has at least two key
            parent = make_node();
            parent->is_leaf = false;
            m_root = parent;

            parent->records.insert(parent->records.end(), std::make_pair((--leaf_node->records.end())->first, leaf_node));
        }

        parent->records.insert(std::make_pair((--left->records.end())->first, left));

        leaf_node->parent = left->parent = parent;

        return { parent, left };
    }

    std::pair<node_type*, node_type*> merge_leaf(node_type* leaf_node, bool& propagation)
    {
        auto left = leaf_node->pre, right = leaf_node->next;

        // check left first
        if (left->parent == leaf_node->parent)
        {
            propagation = false;

            // note that, the leaf could be empty
            auto splitter_iter = leaf_node->parent->records.upper_bound(left->records.begin()->first);

            for (auto iter = leaf_node->records.begin(), end = leaf_node->records.end(); iter != end; )
            {
                left->records.insert(left->records.end(), std::move(*iter));
                iter = leaf_node->records.erase(iter);
            }

            left->next = leaf_node->next;
            leaf_node->next->pre = left;

            delete leaf_node;

            left->parent->records.erase(splitter_iter);

            return std::make_pair(left->parent, left);
        }
        else
        {
            auto splitter_iter = right->parent->records.find(right->records.begin()->first);

            for (auto iter = right->records.begin(), end = right->records.end(); iter != end; )
            {
                leaf_node->records.insert(leaf_node->records.end(), std::move(*iter));
                iter = right->records.erase(iter);
            }

            leaf_node->next = right->next;
            right->next->pre = leaf_node;

            delete right;

            leaf_node->parent->records.erase(splitter_iter);

            return std::make_pair(leaf_node->parent, leaf_node);
        }
    }

    void fix_key_on_path(node_type* node, const key_type& old_key, const key_type& new_key)
    {
        if (node->next == nullptr || node->next == &m_header)
        {
            node = node->parent;
            while (node != nullptr)
            {
                const_cast<key_type&>((--node->records.end())->first) = new_key;
                node = node->parent;
            }
        }
        else
        {
            node_type* right = node->next->parent;
            node = node->parent;
            while (node != right)
            {
                const_cast<key_type&>((--node->records.end())->first) = new_key;
                node = node->parent;
                right = right->parent;
            }
            const_cast<key_type&>(node->records.find(old_key)->first) = new_key;
        }
    }

    enum class EraseStrategy
    {
        ROOT, REMOVE_DIRECTLY, MERGE_LEFT, MERGE_RIGHT, BORROW_LEFT, BORROW_RIGHT, SINGLE_CHILD
    };

    // strategy for order 2
    template <bool order_eq_2>
    typename std::enable_if<order_eq_2, EraseStrategy>::type erase_strategy(const node_type* node, const RecordIterator& record_iterator)
    {
        if (node == m_root)
        {
            return EraseStrategy::ROOT;   // remove root
        }

        auto left = node->pre;
        auto right = node->next;
        const bool is_left_end = left == nullptr || left == &m_header;
        const bool is_right_end = right == nullptr || right == &m_header;
        const bool has_left_slibing = (!is_left_end && left->parent == node->parent);
        const bool has_right_slibing = (!is_right_end && right->parent == node->parent);

        if (has_left_slibing && node->records.size() - 1 + left->records.size() <= order)
        {
            return EraseStrategy::MERGE_LEFT; // merge with left one
        }

        if (has_right_slibing && node->records.size() - 1 + right->records.size() <= order)
        {
            return EraseStrategy::MERGE_RIGHT; // merge with right one
        }

        if (node->records.size() > half_order)
        {
            return EraseStrategy::REMOVE_DIRECTLY; // remove directly
        }

        // borrow a element from right leaf, if it's possible
        if (!is_right_end && right->records.size() > half_order)
        {
            return EraseStrategy::BORROW_RIGHT;
        }

        // or borrow a element from left leaf, if it's possible
        if (!is_left_end && left->records.size() > half_order)
        {
            return EraseStrategy::BORROW_LEFT;
        }

        return EraseStrategy::SINGLE_CHILD;
    }

    // strategy for order greater than 2
    template <bool order_eq_2>
    typename std::enable_if<!order_eq_2, EraseStrategy>::type erase_strategy(const node_type* node, const RecordIterator& record_iterator)
    {
        if (node == m_root)
        {
            return EraseStrategy::ROOT;   // remove root
        }

        auto left = node->pre;
        auto right = node->next;
        const bool is_left_end = left == nullptr || left == &m_header;
        const bool is_right_end = right == nullptr || right == &m_header;
        const bool has_left_slibing = (!is_left_end && left->parent == node->parent);
        const bool has_right_slibing = (!is_right_end && right->parent == node->parent);

        if (node->records.size() > half_order)
        {
            return EraseStrategy::REMOVE_DIRECTLY; // remove directly
        }

        // borrow a element from right leaf, if it's possible
        if (!is_right_end && right->records.size() > half_order)
        {
            return EraseStrategy::BORROW_RIGHT;
        }

        // or borrow a element from left leaf, if it's possible
        if (!is_left_end && left->records.size() > half_order)
        {
            return EraseStrategy::BORROW_LEFT;
        }

        if (has_left_slibing && node->records.size() - 1 + left->records.size() <= order)
        {
            return EraseStrategy::MERGE_LEFT; // merge with left one
        }

        if (has_right_slibing && node->records.size() - 1 + right->records.size() <= order)
        {
            return EraseStrategy::MERGE_RIGHT; // merge with right one
        }

        return EraseStrategy::SINGLE_CHILD;
    }

    // return if upper layer need modifying
    bool erase_helper(node_type*& node, RecordIterator& record_iterator)
    {
        EraseStrategy strategy = erase_strategy<order == 2>(node, record_iterator);

        key_type to_delete_key = record_iterator->first;
        auto left = node->pre;
        auto right = node->next;

        if (strategy == EraseStrategy::ROOT)
        {
            node->records.erase(record_iterator);
            return false;
        }
        else if (strategy == EraseStrategy::MERGE_LEFT)
        {
            node_type* parent = node->parent;

            bool need_fix_pos_key_on_path = record_iterator == (--node->records.end());

            node->records.erase(record_iterator);

            key_type left_key = (--left->records.end())->first;
            auto left_in_parent = parent->records.find(left_key);

            for (auto iter = left->records.rbegin(), end = left->records.rend(); iter != end; iter++)
            {
                if (iter->second != nullptr)
                {
                    iter->second->parent = node;
                }
                node->records.insert(node->records.begin(), std::move(*iter));
            }

            if (need_fix_pos_key_on_path)
            {
                fix_key_on_path(node, to_delete_key, (--node->records.end())->first);
            }

            if (left->pre != nullptr)
            {
                left->pre->next = node;
            }
            node->pre = left->pre;

            delete left;

            const_cast<node_type*&>(left_in_parent->second) = nullptr;

            node = parent;
            record_iterator = left_in_parent;
            return true;
        }
        else if (strategy == EraseStrategy::MERGE_RIGHT)
        {
            node_type* parent = node->parent;

            key_type left_key = (--node->records.end())->first;

            node->records.erase(record_iterator);

            auto left_in_parent = parent->records.find(left_key);

            for (auto iter = node->records.begin(), end = node->records.end(); iter != end; )
            {
                if (iter->second != nullptr)
                {
                    iter->second->parent = right;
                }
                right->records.insert(right->records.begin(), std::move(*iter));
                iter = node->records.erase(iter);
            }

            if (node->pre != nullptr)
            {
                node->pre->next = right;
            }
            right->pre = node->pre;

            delete node;

            const_cast<node_type*&>(left_in_parent->second) = nullptr;


            node = parent;
            record_iterator = left_in_parent;
            return true;
        }
        else if (strategy == EraseStrategy::REMOVE_DIRECTLY)
        {
            bool need_fix_pos_key_on_path = record_iterator == (--node->records.end());

            auto after_erase = node->records.erase(record_iterator);

            if (need_fix_pos_key_on_path)
            {
                fix_key_on_path(node, to_delete_key, (--node->records.end())->first);
            }
            return false;
        }
        else if (strategy == EraseStrategy::BORROW_RIGHT)
        {
            key_type old_key = (--node->records.end())->first;
            key_type new_key = right->records.begin()->first;
            fix_key_on_path(node, old_key, new_key);

            node->records.erase(record_iterator);
            auto right_first_iter = right->records.begin();

            if (right_first_iter->second != nullptr)
            {
                right_first_iter->second->parent = node;
            }
            node->records.insert(node->records.end(), *right_first_iter);
            right->records.erase(right_first_iter);

            return false;
        }
        else if (strategy == EraseStrategy::BORROW_LEFT)
        {
            bool need_fix_pos_key_on_path = record_iterator == (--node->records.end());

            node->records.erase(record_iterator);
            auto left_last_iter = --left->records.end();

            key_type left_old_key = left_last_iter->first;

            if (left_last_iter->second != nullptr)
            {
                left_last_iter->second->parent = node;
            }

            node->records.insert(node->records.begin(), std::move(*left_last_iter));
            left_last_iter = left->records.erase(left_last_iter);
            key_type left_new_key = (--left_last_iter)->first;

            fix_key_on_path(left, left_old_key, left_new_key);

            if (need_fix_pos_key_on_path)
            {
                key_type new_key = (--node->records.end())->first;
                fix_key_on_path(node, to_delete_key, new_key);
            }

            return false;
        }
        // single child
        else
        {
            auto parent = node->parent;
            const_cast<node_type*&>(parent->records.begin()->second) = nullptr;

            if (node->pre != nullptr)
            {
                node->pre->next = right;
                if (right != nullptr)
                {
                    right->pre = node->pre;
                }
            }
            if (node->next != nullptr)
            {
                node->next->pre = left;
                if (left != nullptr)
                {
                    left->next = node->next;
                }
            }

            delete node;

            node = parent;
            record_iterator = parent->records.begin();
            return true;
        }
    }

private:
    node_type* m_root = nullptr;
    InnerCompare m_innercomp;
    node_type m_header;
    size_type m_size = 0u;
};

