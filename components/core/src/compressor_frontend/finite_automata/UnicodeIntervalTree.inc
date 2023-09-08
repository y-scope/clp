#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_UNICODE_INTERVAL_TREE_TPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_UNICODE_INTERVAL_TREE_TPP

#include "UnicodeIntervalTree.hpp"

// C++ standard libraries
#include <cassert>

using std::max;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend::finite_automata {

    template<class T>
    void UnicodeIntervalTree<T>::insert (Interval interval, T value) {
        m_root = Node::insert(std::move(m_root), interval, value);
    }

    template<class T>
    unique_ptr<class UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::insert (unique_ptr<Node> node, Interval interval, T value) {
        if (node == nullptr) {
            unique_ptr<Node> n(new Node(interval, value));
            n->update();
            return n;
        }
        if (interval < node->m_interval) {
            node->m_left = Node::insert(std::move(node->m_left), interval, value);
        } else if (interval > node->m_interval) {
            node->m_right = Node::insert(std::move(node->m_right), interval, value);
        } else {
            node->m_value = value;
        }
        node->update();
        return Node::balance(std::move(node));
    }

    template<typename T>
    vector<typename UnicodeIntervalTree<T>::Data> UnicodeIntervalTree<T>::all () const {
        vector<Data> results;
        if (m_root != nullptr) {
            m_root->all(&results);
        }
        return results;
    }

    template<typename T>
    void UnicodeIntervalTree<T>::Node::all (vector<Data>* results) {
        if (m_left != nullptr) {
            m_left->all(results);
        }
        results->push_back(Data(m_interval, m_value));
        if (m_right != nullptr) {
            m_right->all(results);
        }
    }

    template<typename T>
    unique_ptr<vector<typename UnicodeIntervalTree<T>::Data>> UnicodeIntervalTree<T>::find (Interval interval) {
        unique_ptr<vector<Data>> results(new vector<Data>);
        m_root->find(interval, results.get());
        return results;
    }

    template<class T>
    void UnicodeIntervalTree<T>::Node::find (Interval interval, vector<Data>* results) {
        if (!overlaps_recursive(interval)) {
            return;
        }
        if (m_left != nullptr) {
            m_left->find(interval, results);
        }
        if (overlaps(interval)) {
            results->push_back(Data(m_interval, m_value));
        }
        if (m_right != nullptr) {
            m_right->find(interval, results);
        }
    }

    template<class T>
    unique_ptr<vector<typename UnicodeIntervalTree<T>::Data>> UnicodeIntervalTree<T>::pop (Interval interval) {
        unique_ptr<vector<Data>> results(new vector<Data>);
        while (true) {
            unique_ptr<Node> n;
            m_root = Node::pop(std::move(m_root), interval, &n);
            if (n == nullptr) {
                break;
            }
            results->push_back(Data(n->get_interval(), n->get_value()));
        }
        return results;
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::pop (unique_ptr<Node> node, Interval interval,
                                                                                         unique_ptr<Node>* ret) {
        if (node == nullptr) {
            return nullptr;
        }
        if (!node->overlaps_recursive(interval)) {
            return node;
        }
        node->m_left = Node::pop(std::move(node->m_left), interval, ret);
        if (ret->get() != nullptr) {
            node->update();
            return Node::balance(std::move(node));
        }
        assert(node->overlaps(interval));
        ret->reset(node.release());
        if (((*ret)->m_left == nullptr) && ((*ret)->m_right == nullptr)) {
            return nullptr;
        } else if ((*ret)->m_left == nullptr) {
            return std::move((*ret)->m_right);
        } else if ((*ret)->m_right == nullptr) {
            return std::move((*ret)->m_left);
        } else {
            unique_ptr<Node> replacement;
            unique_ptr<Node> sub_tree = Node::pop_min(std::move((*ret)->m_right), &replacement);
            replacement->m_left = std::move((*ret)->m_left);
            replacement->m_right = std::move(sub_tree);
            replacement->update();
            return Node::balance(std::move(replacement));
        }
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::pop_min (unique_ptr<Node> node, unique_ptr<Node>* ret) {
        assert(node != nullptr);
        if (node->m_left == nullptr) {
            assert(node->m_right != nullptr);
            unique_ptr<Node> right(std::move(node->m_right));
            ret->reset(node.release());
            return right;
        }
        node->m_left = Node::pop_min(std::move(node->m_left), ret);
        node->update();
        return Node::balance(std::move(node));
    }

    template<class T>
    void UnicodeIntervalTree<T>::Node::update () {
        if ((m_left == nullptr) && (m_right == nullptr)) {
            m_height = 1;
            m_lower = m_interval.first;
            m_upper = m_interval.second;
        } else if (m_left == nullptr) {
            m_height = 2;
            m_lower = m_interval.first;
            m_upper = max(m_interval.second, m_right->m_upper);
        } else if (m_right == nullptr) {
            m_height = 2;
            m_lower = m_left->m_lower;
            m_upper = max(m_interval.second, m_left->m_upper);
        } else {
            m_height = max(m_left->m_height, m_right->m_height) + 1;
            m_lower = m_left->m_lower;
            m_upper = max({m_interval.second, m_left->m_upper, m_right->m_upper});
        }
    }

    template<class T>
    int UnicodeIntervalTree<T>::Node::balance_factor () {
        return (m_right != nullptr ? m_right.get() : 0) -
               (m_left != nullptr ? m_left.get() : 0);
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::balance (unique_ptr<Node> node) {
        int factor = node->balance_factor();
        if (factor * factor <= 1) {
            return node;
        }
        int sub_factor = (factor < 0) ? node->m_left->balance_factor() : node->m_right->balance_factor();
        if (factor * sub_factor > 0) {
            return Node::rotate(std::move(node), factor);
        }
        if (factor == 2) {
            node->m_right = Node::rotate(std::move(node->m_right), sub_factor);
        } else {
            node->m_left = Node::rotate(std::move(node->m_left), sub_factor);
        }
        return Node::rotate(std::move(node), factor);
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::rotate (unique_ptr<Node> node, int factor) {
        if (factor < 0) {
            return Node::rotate_cw(std::move(node));
        } else if (factor > 0) {
            return Node::rotate_ccw(std::move(node));
        }
        return node;
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::rotate_cw (unique_ptr<Node> node) {
        unique_ptr<Node> n(std::move(node->m_left));
        node->m_left.reset(n->m_right.release());
        n->m_right.reset(node.release());
        n->m_right->update();
        n->update();
        return n;
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::rotate_ccw (unique_ptr<Node> node) {
        unique_ptr<Node> n(std::move(node->m_right));
        node->m_right.reset(n->m_left.release());
        n->m_left.reset(node.release());
        n->m_left->update();
        n->update();
        return n;
    }

    template<class T>
    bool UnicodeIntervalTree<T>::Node::overlaps_recursive (Interval i) {
        return ((m_lower <= i.first) && (i.first <= m_upper)) ||
               ((m_lower <= i.second) && (i.second <= m_upper)) ||
               ((i.first <= m_lower) && (m_lower <= i.second));
    }

    template<class T>
    bool UnicodeIntervalTree<T>::Node::overlaps (Interval i) {
        return ((m_interval.first <= i.first) && (i.first <= m_interval.second)) ||
               ((m_interval.first <= i.second) && (i.second <= m_interval.second)) ||
               ((i.first <= m_interval.first) && (m_interval.first <= i.second));
    }
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_UNICODE_INTERVAL_TREE_TPP