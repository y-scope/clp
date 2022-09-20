#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_UNICODE_INTERVAL_TREE_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_UNICODE_INTERVAL_TREE_HPP

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

// Project headers
#include "../Constants.hpp"

namespace compressor_frontend::finite_automata {

    template<class T>
    class UnicodeIntervalTree {
    public:
        typedef std::pair<uint32_t, uint32_t> Interval;

        struct Data {
        public:
            Interval m_interval;
            T m_value;

            Data (Interval interval, T value) : m_interval(std::move(interval)), m_value(value) {}
        };

        void insert (Interval interval, T value);

        std::vector<Data> all () const;

        std::unique_ptr<std::vector<Data>> find (Interval interval);

        std::unique_ptr<std::vector<Data>> pop (Interval interval);

        void reset () {
            m_root.reset();
        }

    private:
        class Node {
        public:
            static std::unique_ptr<Node> balance (std::unique_ptr<Node> node);

            static std::unique_ptr<Node> insert (std::unique_ptr<Node> node, Interval interval, T value);

            static std::unique_ptr<Node> pop (std::unique_ptr<Node> node, Interval interval, std::unique_ptr<Node>* ret);

            static std::unique_ptr<Node> pop_min (std::unique_ptr<Node> node, std::unique_ptr<Node>* ret);

            static std::unique_ptr<Node> rotate (std::unique_ptr<Node> node, int factor);

            static std::unique_ptr<Node> rotate_cw (std::unique_ptr<Node> node);

            static std::unique_ptr<Node> rotate_ccw (std::unique_ptr<Node> node);

            Node () : m_lower(0), m_upper(0), m_height(0) {}

            Node (Interval i, T v) : m_interval(std::move(i)), m_value(v) {}

            void all (std::vector<Data>* results);

            void find (Interval interval, std::vector<Data>* results);

            void update ();

            int balance_factor ();

            bool overlaps_recursive (Interval i);

            bool overlaps (Interval i);

            Interval get_interval () {
                return m_interval;
            }

            T get_value () {
                return m_value;
            }

        private:

            Interval m_interval;
            T m_value;
            uint32_t m_lower{};
            uint32_t m_upper{};
            int m_height{};
            std::unique_ptr<Node> m_left;
            std::unique_ptr<Node> m_right;

        };

        std::unique_ptr<Node> m_root;
    };
}

// Implementation of template class must be included in anything wanting to use it
#include "UnicodeIntervalTree.tpp"

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_UNICODE_INTERVAL_TREE_HPP