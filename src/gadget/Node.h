/*
    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef __TINYEMBER_GADGET_NODE_H
#define __TINYEMBER_GADGET_NODE_H

#include <list>
#include "../Types.h"
#include "Collection.h"
#include "DirtyStateListener.h"
#include "NodeField.h"
#include "Parameter.h"

namespace gadget
{
    /** Forward declarations */
    class NodeFactory;
    class Parameter;
    class ParameterFactory;

    /**
     * Represents a node object within the gadget tree. A node may contain child nodes and parameters.
     */
    class Node
    {
        friend class NodeFactory;
        friend class ParameterFactory;
        public:
            typedef Collection<Node*> NodeCollection;
            typedef Collection<gadget::Parameter*> ParameterCollection;
            typedef DirtyStateListener<NodeFieldState::flag_type, Node const*> DirtyStateListenerT;

            /** Destructor */
            virtual ~Node();

            /**
             * Returns the node's number.
             * @return The node's number.
             */
            int number() const;

            /**
             * Returns the node's parent node. This value may be null if this node is the root.
             * @return The node's parent node.
             */
            Node const* parent() const;

            /**
             * Returns the string identifier of this node.
             * @return The string identifier of this node.
             */
            String const& identifier() const;

            /**
             * Returns the node's description string.
             * @return The description string.
             */
            String const& description() const;

            /**
             * Returns the current schema identifier. If no schema is specified, an empty
             *      string is being returned.
             * @return The current schema identifier.
             */
            String const& schema() const;

            /**
             * When a description is set, it is being returned. Otherwise, the identifier string
             * will be returned.
             * @return The description if visible, otherwise the identifier string.
             */
            String const& displayName() const;

            /**
             * Returns a const reference to the collection of child nodes.
             * @return A const reference to the collection of child nodes.
             */
            NodeCollection const& nodes() const;

            /**
             * Returns a const reference to the collection of parameters.
             * @return A const reference to the collection of parameters.
             */
            ParameterCollection const& parameters() const;

            /**
             * Returns true if at least one node property is marked dirty.
             * @return true if at least one node property is marked dirty.
             */
            bool isDirty() const;

            /**
             * Returns true if this node is online, otherwise false.
             * @return true if this node is online, otherwise false.
             */
            bool isOnline() const;

            /**
             * Returns true if this node is mounted. An unmounted node
             * will not be reported in GetDirectory requests.
             * @return true if the node is mounted, otherwise false.
             */
            bool isMounted() const;

            /**
             * Updates the description string of this node.
             * @param value The new description string to take.
             */
            void setDescription(String const& value);

            /**
             * Sets a new schema identifier. To remove the identifier,
             * simply pass an empty string.
             * @param value The new schema identifier.
             */
            void setSchema(String const& value);

            /**
             * Changes the online state of this node.
             * @param isOnline The new online state.
             */
            void setIsOnline(bool isOnline);

            /**
             * Resets the dirty state if this node and optionally all children's states
             * as well.
             * @param recursive If set to true, the dirty states of all children will be 
             *      cleared as well.
             */
            void clearDirtyState(bool recursive) const;

            /**
             * Remounts the node. Marks the node dirty and notifies its current state.
             */
            void mount();

            /**
             * Unmounts the node. Marks the node offline, dirty and notifies its current state.
             */
            void unmount();

            /**
             * Adds a new dirty state listener which receives a notification when
             * a property of the node changes.
             * @param listener The listener to add.
             */
            void registerListener(DirtyStateListenerT* listener);

            /**
             * Removes a dirty state listener from this node.
             * @param listener The listener to remove.
             */
            void unregisterListener(DirtyStateListenerT* listener);

            /**
             * Marks the node dirty by setting the DirtyChildEntity flag.
             */
            void markDirty();

            /**
             * Removes the passed node from the collection of child nodes. The parent of the
             * removed node will be set to null.
             * @param node The node to remove.
             * @return true if the node has been found and removed, otherwise false.
             * @note The removed node must be deleted manually.
             */
            bool remove(Node *const node);

            /**
             * Removes the passed parameter from this node. The parent of the removed
             * parameter will be set to null.
             * @param parameter The parameter to remove.
             * @return true if the parameter has been removed.
             * @note The removed parameter must be deleted manually.
             */
            bool remove(Parameter *const parameter);

            /**
             * Returns the current dirty state of this node.
             * @return The current dirty state of this node.
             */
            NodeFieldState const& dirtyState() const;

        private:
            /**
             * Initializes a new Node.
             * @param parent The parent node.
             * @param identifier The identifier string of this node.
             * @param number of this node.
             */
            Node(Node* parent, String const& identifier, int number);

            /**
             * Informs all registered state listeners about the dirty state of this node.
             */
            void notify() const;

        private:
            int const m_number;
            String m_description;
            String const m_identifier;
            String m_schema;
            Node* m_parent;
            NodeCollection m_children;
            ParameterCollection m_parameters;
            std::list<DirtyStateListenerT*> m_listeners;
            bool m_isOnline;
            bool m_isMounted;
            mutable NodeFieldState m_state;
    };

    /**************************************************************************
     * Inline implementation                                                  *
     **************************************************************************/

    inline String const& Node::displayName() const
    {
        if (m_description.empty())
            return m_identifier;
        else
            return m_description;
    }
}

#endif//__TINYEMBER_GADGET_NODE_H
