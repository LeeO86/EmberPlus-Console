/*
    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef __TINYEMBER_NODEFIELD_H
#define __TINYEMBER_NODEFIELD_H

#include "DirtyState.h"

namespace gadget
{
    /**
     * Scoped enumeration containing the symbolic names of all node properties.
     */
    struct NodeField
    {
        enum _Domain
        {
            None = 0U,
            Identifier = (1U << 0U),
            Description = (1U << 1U),
            IsRoot = (1U << 2U),
            IsOnline = (1U << 3U),
            Schema = (1U << 4U),

            DirtyChildEntity = (1U << 31U),

            All = (
                     Identifier | Description | IsRoot | IsOnline | Schema
                  )
        };

        typedef unsigned int value_type;

        /**
         * Initializes a new NodeField.
         * @param value The node fields to initialize this instance with.
         */
        NodeField(value_type value)
            : value(value)
        {
        }

        value_type const value;
    };

    typedef DirtyState<NodeField::_Domain> NodeFieldState;
}

#endif//__TINYEMBER_NODEFIELD_H
