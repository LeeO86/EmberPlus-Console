/*
    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef __LIBFORMULA_ERRORTYPE_HPP
#define __LIBFORMULA_ERRORTYPE_HPP

#include <string>

namespace libformula
{
    /**
     * Identifies an error type that occured during scanning or parsing of the term provided.
     */
    struct ErrorType
    {
        public:
            /**
             * Enumeration containing all error types.
             */
            enum _Domain
            {
                None,
                InvalidArgumentCount,
                TokenIsNotAValidNumber,
                UnknownToken,
                UnknownFunction,
                UnexpectedToken,
                UnexpectedEndOfFile
            };

            typedef _Domain value_type;

            /**
             * Initializes a new ErrorType with the specified type.
             */
            ErrorType(_Domain value)
                : m_value(value)
            {}

            /**
             * Returns the error type.
             * @return The error type.
             */
            value_type type() const
            {
                return m_value;
            }

        private:
            value_type m_value;
    };
}

#endif  // __LIBFORMULA_ERRORTYPE_HPP
