/*
    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef __TINYEMBER_GADGET_ACCESS_H
#define __TINYEMBER_GADGET_ACCESS_H

namespace gadget
{
    /**
     * Scoped enumeration containing the symbolic names for the various access types.
     */
    struct Access
    {
        public:
            typedef enum _Domain
            {
                None = 0,
                ReadOnly,
                WriteOnly,
                ReadWrite,
            } _Domain;

            typedef std::size_t value_type;

            /**
             * Constructor
             * @param access The access type to initialize this instance with.
             */
            Access(_Domain access)
                : m_value(access)
            {}

            /**
             * Returns the raw value currently represented by this instance.
             * @return The raw value currently represented by this instance.
             */
            value_type value() const
            {
                return m_value;
            }

        private:
            value_type m_value;
    };
}

#endif//__TINYEMBER_GADGET_ACCESS_H
