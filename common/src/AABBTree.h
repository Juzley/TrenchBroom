/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRENCHBROOM_AABBTREE_H
#define TRENCHBROOM_AABBTREE_H

#include <BBox.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>

template <typename T, size_t S, typename U, typename EQ = std::equal_to<U>>
class AABBTree {
private:
    using Box = BBox<T,S>;

    class Leaf;

    class Node {
    private:
        Box m_bounds;
    protected:
        explicit Node(const Box& bounds) : m_bounds(bounds) {}
    public:
        virtual ~Node() = default;

        /**
         * Return the bounds of this node.
         *
         * @return the bounds
         */
        const Box& bounds() const {
            return m_bounds;
        }

        /**
         * Return the height of this node. A leaf always has a height of 1, and an inner node has a height equal to the
         * maximum of the heights of its children plus one.
         *
         * @return the height
         */
        virtual size_t height() const = 0;

        /**
         * Return the balance of this node. A leaf always has a balance of 0, and for an inner node, the balance is the
         * difference between the heights of its left and its right subtrees.
         *
         * @return the balance
         */
        virtual int balance() const = 0;

        /**
         * Inserts a new node with the given parameters into the subtree of which this node is the root. Returns the new
         * root of the subtree after insertion.
         *
         * @param bounds the bounds of the data to be inserted
         * @param data the data to be inserted
         * @return the new root
         */
        virtual Node* insert(const Box& bounds, U& data) = 0;

        /**
         * Removes the node with the given parameters from the subtree of which this node is the root. Returns the new
         * root of the subtree after removal.
         *
         * @param bounds the bounds of the node to be removed
         * @param data the data associated with the node to be removed
         * @return the new root
         */
        virtual Node* remove(const Box& bounds, U& data) = 0;

        /**
         * Finds the leaf of this node's subtree such that it increases the given bounds the least.
         *
         * @param bounds the bounds to test
         * @return the leaf of this node's subtree that increases the given bounds the least
         */
        virtual Leaf* findRebalanceCandidate(const Box& bounds) = 0;

        /**
         * Appends a textual representation of this node to the given output stream.
         *
         * @param str the stream to append to
         */
        void appendTo(std::ostream& str) const {
            appendTo(str, "  ", 0);
        }

        /**
         * Appends a textual representation of this node to the given output stream using the given indent string and
         * the given level of indentation.
         *
         * @param str the stream to append to
         * @param indent the indent string
         * @param level the level of indentation
         */
        virtual void appendTo(std::ostream& str, const std::string& indent, size_t level) const = 0;
    protected:
        /**
         * Updates the bounds of this node.
         *
         * @param bounds the new bounds
         */
        void setBounds(const Box& bounds) {
            m_bounds = bounds;
        }

        /**
         * Appends a textual representation of this node's bounds to the given output stream.
         *
         * @param str the stream to append to
         */
        void appendBounds(std::ostream& str) const {
            str << "[ (";
            m_bounds.min.write(str);
            str << ") (";
            m_bounds.max.write(str);
            str << ") ]";
        }
    };

    /**
     * An inner node of an AABB tree does not carry data. It's only purpose is to structure the tree. Its bounds is
     * the smallest bounding box that contains the bounds of its children.
     */
    class InnerNode : public Node {
    private:
        Node* m_left;
        Node* m_right;
        size_t m_height;
    public:
        InnerNode(Node* left, Node* right) : Node(left->bounds().mergedWith(right->bounds())), m_left(left), m_right(right), m_height(0) {
            assert(m_left != nullptr);
            assert(m_right != nullptr);
            updateHeight();
        }

        ~InnerNode() {
            delete m_left;
            delete m_right;
        }

        size_t height() const override {
            return m_height;
        }

        int balance() const override {
            const auto l = static_cast<int>(m_left->height());
            const auto r = static_cast<int>(m_right->height());
            return r - l;
        }

        Node* insert(const Box& bounds, U& data) override {
            // Select the subtree which is increased the least by inserting a node with the given bounds.
            // Then insert the node into that subtree and update our reference to it.
            auto*& subtree = selectLeastIncreaser(m_left, m_right, bounds);
            subtree = subtree->insert(bounds, data);

            // Update our data and rebalance if necessary.
            updateBounds();
            updateHeight();
            rebalance();

            return this;
        }

        Node* remove(const Box& bounds, U& data) override {
            auto* result = doRemove(bounds, data, m_left, m_right);
            if (result != nullptr) {
                return result;
            } else {
                return doRemove(bounds, data, m_right, m_left);
            }
        }
    private:
        /**
         * Attempt to remove the node with the given bounds and data from the given child.
         *
         * @param bounds the bounds of the node to remove
         * @param data the data of the node to remove
         * @param child the child to remove the node from
         * @param sibling the sibling of the given child in this inner node
         * @return the node that should replace the given child in this inner node, or nullptr if the node to remove is
         * not a descendant of the given child
         */
        Node* doRemove(const Box& bounds, U& data, Node*& child, Node*& sibling) {
            Node* result = nullptr;
            if (child->bounds().contains(bounds)) {
                auto* newChild = child->remove(bounds, data);
                if (newChild == nullptr) {
                    // child is a leaf, and it represents the node to remove; return sibling to the caller
                    result = sibling;
                    // prevent the sibling to get deleted when this node gets deleted by the parent
                    sibling = nullptr;
                    // child will be deleted when this node gets deleted by the caller
                } else if (newChild != child) {
                    // the node to be removed was deleted from child's subtree, and we need to update our pointer
                    // with the new root of that subtree
                    delete child;
                    child = newChild;

                    // Update our data and rebalance if necessary.
                    updateBounds();
                    updateHeight();
                    rebalance();

                    result = this;
                }
            }

            return result;
        }

        /**
         * If this node is out of balance, we rebalance it. A node is out of balance if and only if the height of its
         * left and the height of its right subtrees differ by more than 1.
         */
        void rebalance() {
            if (m_left->height() > m_right->height() && m_left->height() - m_right->height() > 1) {
                rebalance(m_left, m_right);
            } else if (m_right->height() > m_left->height() && m_right->height() - m_left->height() > 1) {
                rebalance(m_right, m_left);
            }
        }

        /**
         * Rebalance this subtree by removing a leaf from the higher subtree and inserting that leaf (or rather, the
         * data associated with it) into the lower subtree. Thereby, we select the leaf which would increase the bounds
         * of the lower subtree the least.
         *
         * Note that we pass the pointers to the higher and lower nodes by reference so that we can update them.
         *
         * @param higher the higher subtree of this node
         * @param lower the lower subtree of this node
         */
        void rebalance(Node*& higher, Node*& lower) {
            auto* toRemove = higher->findRebalanceCandidate(lower->bounds());
            const auto bounds = toRemove->bounds();
            const auto data = toRemove->data();

            higher = higher->remove(bounds, data);
            lower = lower->insert(bounds, data);
        }

    public:
        Leaf* findRebalanceCandidate(const Box& bounds) override {
            Leaf* leftCandidate = m_left->findRebalanceCandidate(bounds);
            Leaf* rightCandidate = m_right->findRebalanceCandidate(bounds);
            return selectLeastIncreaser(leftCandidate, rightCandidate, bounds);
        }
    private:
        /**
         * Selects one of the two given nodes such that it increases the given bounds the least.
         *
         * @tparam TT the type of the nodes
         * @param node1 the first node to test
         * @param node2 the second node to test
         * @param bounds the bounds to test against
         * @return node1 if it increases the given bounds volume by a smaller or equal amount than node2 would, and
         *     node2 otherwise
         */
        template <typename TT>
        static TT*& selectLeastIncreaser(TT*& node1, TT*& node2, const Box& bounds) {
            const auto new1 = node1->bounds().mergedWith(bounds);
            const auto new2 = node2->bounds().mergedWith(bounds);
            const auto vol1 = node1->bounds().volume();
            const auto vol2 = node2->bounds().volume();
            const auto diff1 = new1.volume() - vol1;
            const auto diff2 = new2.volume() - vol2;

            if (diff1 <= diff2) {
                return node1;
            } else {
                return node2;
            }
        }

        void updateBounds() {
            this->setBounds(m_left->bounds().mergedWith(m_right->bounds()));
        }

        void updateHeight() {
            m_height = std::max(m_left->height(), m_right->height()) + 1;
            assert(m_height > 0);
        }
    public:
        void appendTo(std::ostream& str, const std::string& indent, const size_t level) const override {
            for (size_t i = 0; i < level; ++i)
                str << indent;

            str << "O ";
            this->appendBounds(str);
            str << std::endl;

            m_left->appendTo(str, indent, level + 1);
            m_right->appendTo(str, indent, level + 1);
        }
    };

    /**
     * A leaf node represents actual data. It does not have any children. Its bounds equals the bounds supplied when
     * the node was inserted into the tree. A leaf has a height of 1 and a balance of 0.
     */
    class Leaf : public Node {
    private:
        U m_data;
    public:
        Leaf(const Box& bounds, U& data) : Node(bounds), m_data(data) {}

        /**
         * Returns the data associated with this node.
         *
         * @return the data associated with this node
         */
        U& data() {
            return m_data;
        }

        /**
         * Returns the data associated with this node.
         *
         * @return the data associated with this node
         */
        const U& data() const {
            return m_data;
        }

        /**
         * Returns the height of this node, which is always 1.
         *
         * @return the height of this node
         */
        size_t height() const override {
            return 1;
        }

        /**
         * Returns the balance of this node, which is always 0.
         *
         * @return the balance of this node
         */
        int balance() const override {
            return 0;
        }

        /**
         * Returns a new inner node that has this leaf as its left child and a new leaf representing the given bounds
         * and data as its right child.
         *
         * @param bounds the bounds to insert
         * @param data the data to insert
         * @return the newly created inner node which should replace this leaf in the parent
         */
        Node* insert(const Box& bounds, U& data) override {
            return new InnerNode(this, new Leaf(bounds, data));
        }

        /**
         * Tests whether this node equals the given data.
         *
         * @param bounds the bounds to remove
         * @param data the data to remove
         * @return nullptr if this node matches the given data and a pointer to this node otherwise
         */
        Node* remove(const Box& bounds, U& data) override {
            static const EQ eq;
            if (eq(data, m_data)) {
                return nullptr;
            } else {
                return this;
            }
        }

        /**
         * Always returns this node.
         *
         * @param bounds ignored
         * @return a pointer to this node
         */
        Leaf* findRebalanceCandidate(const Box& bounds) override {
            return this;
        }

        void appendTo(std::ostream& str, const std::string& indent, const size_t level) const override {
            for (size_t i = 0; i < level; ++i)
                str << indent;

            str << "L ";
            this->appendBounds(str);
            str << ": " << m_data << std::endl;
        }
    };
private:
    Node* m_root;
public:
    AABBTree() : m_root(nullptr) {}

    ~AABBTree() {
        delete m_root;
    }

    /**
     * Insert a node with the given bounds and data into this tree.
     *
     * @param bounds the bounds to insert
     * @param data the data to insert
     */
    void insert(const Box& bounds, U& data) {
        if (empty()) {
            m_root = new Leaf(bounds, data);
        } else {
            m_root = m_root->insert(bounds, data);
        }
        assert(std::abs(m_root->balance()) < 2);
    }

    /**
     * Removes the node with the given bounds and data into this tree.
     *
     * @param bounds the bounds to remove
     * @param data the data to remove
     * @return true if a node with the given bounds and data was removed, and false otherwise
     */
    bool remove(const Box& bounds, U& data) {
        if (empty()) {
            return false;
        } else if (m_root->bounds().contains(bounds)) {
            auto* newRoot = m_root->remove(bounds, data);
            if (newRoot != m_root) {
                delete m_root;
                m_root = newRoot;
            }
            assert(empty() || std::abs(m_root->balance()) < 2);
            return true;
        } else {
            return false;
        }
    }

    /**
     * Indicates whether this tree is empty.
     *
     * @return true if this tree is empty and false otherwise
     */
    bool empty() const {
        return m_root == nullptr;
    }

    /**
     * Returns the height of this tree.
     *
     * The height of an AABB tree is the length of the longest path from the root to a leaf.
     *
     * @return the height of this tree
     */
    size_t height() const {
        if (empty()) {
            return 0;
        } else {
            return m_root->height();
        }
    }

    /**
     * Returns the bounds of all nodes in this tree.
     *
     * @return the bounds of all nodes in this tree, or a bounding box made up of NaN values if this tree is empty
     */
    const Box& bounds() const {
        static const auto EmptyBox = Box(Vec<T,S>::NaN, Vec<T,S>::NaN);

        assert(!empty());
        if (empty()) {
            return EmptyBox;
        } else {
            return m_root->bounds();
        }
    }

    /**
     * Prints a textual representation of this tree to the given output stream.
     *
     * @param str the output stream to print to
     */
    void print(std::ostream& str = std::cout) const {
        if (!empty()) {
            m_root->appendTo(str);
        }
    }
};

#endif //TRENCHBROOM_AABBTREE_H
