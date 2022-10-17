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
        /// TODO: probably use this Data type more often in this class???
        /**
         * Structure to represent utf8 data
         */
        struct Data {
        public:
            Data (Interval interval, T value) : m_interval(std::move(interval)), m_value(value) {}

            Interval m_interval;
            T m_value;
        };
        
        /**
         * Insert data into the tree
         * @param interval
         * @param value
         */
        void insert (Interval interval, T value);
        
        /**
         * Returns all utf8 in the tree
         * @return std::vector<Data>
         */
        std::vector<Data> all () const;

        /**
         * Return an interval in the tree
         * @param interval
         * @return std::unique_ptr<std::vector<Data>>
         */
        std::unique_ptr<std::vector<Data>> find (Interval interval);
        
        /**
         * Remove an interval from the tree
         * @param interval
         * @return std::unique_ptr<std::vector<Data>>
         */
        std::unique_ptr<std::vector<Data>> pop (Interval interval);

        void reset () {
            m_root.reset();
        }

    private:
        class Node {
        public:
            // Constructor
            Node () : m_lower(0), m_upper(0), m_height(0) {}

            // Constructor
            Node (Interval i, T v) : m_interval(std::move(i)), m_value(v) {}
            
            /**
             * Balance the subtree below a node
             * @param node
             * @return std::unique_ptr<Node>
             */
            static std::unique_ptr<Node> balance (std::unique_ptr<Node> node);

            /**
             * Insert a node
             * @param node
             * @param interval
             * @param value
             * @return std::unique_ptr<Node>
             */
            static std::unique_ptr<Node> insert (std::unique_ptr<Node> node, Interval interval, T value);
            
            /**
             * Remove a node
             * @param node
             * @param interval
             * @param ret
             * @return std::unique_ptr<Node>
             */
            static std::unique_ptr<Node> pop (std::unique_ptr<Node> node, Interval interval, std::unique_ptr<Node>* ret);

            /**
             * Remove a node
             * @param node
             * @param ret
             * @return std::unique_ptr<Node>
             */
            static std::unique_ptr<Node> pop_min (std::unique_ptr<Node> node, std::unique_ptr<Node>* ret);

            /**
             * Rotate a node by a factor
             * @param node
             * @param factor
             * @return std::unique_ptr<Node>
             */
            static std::unique_ptr<Node> rotate (std::unique_ptr<Node> node, int factor);

            /**
             * Rotate a node clockwise
             * @param node
             * @return std::unique_ptr<Node>
             */
            static std::unique_ptr<Node> rotate_cw (std::unique_ptr<Node> node);

            /**
             * Rotate a node counterclockwise
             * @param node
             * @return std::unique_ptr<Node>
             */
            static std::unique_ptr<Node> rotate_ccw (std::unique_ptr<Node> node);

            /**
             * add all utf8 in subtree to results
             * @param results
             */
            void all (std::vector<Data>* results);

            /**
             * add all utf8 in subtree that matches interval to results
             * @param interval
             * @param results
             */
            void find (Interval interval, std::vector<Data>* results);
            
            /**
             * update node
             */
            void update ();

            /**
             * get balance factor of node
             */
            int balance_factor ();

            /**
             * overlaps_recursive()
             * @param i
             */
            bool overlaps_recursive (Interval i);
            
            /**
             * overlaps()
             * @param i
             */
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