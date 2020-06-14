/*
    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef __LIBFORMULA_SYMBOLTYPE_HPP
#define __LIBFORMULA_SYMBOLTYPE_HPP

namespace libformula
{
    /**
     * Structure representing all tokens supported by the termparser.
     */
    struct SymbolType
    {
        public:
            enum _Domain
            {
                LParen = '(',
                RParen = ')',
                Assign = '=',
                Plus = '+',
                Minus = '-',
                Multiply = '*',
                Divide = '/',
                LongDivide = '\\',
                Modulo = '%',
                Pow = '^',
                Comma = ',',

                It = '$',

                Pi              = 0x0201,
                E,

                Sin             = 0x0401,
                Cos,
                Tan,
                Sinh,
                Cosh,
                Tanh,
                Asin,
                Acos,
                Atan,
                Sqrt,
                Log,
                Ln,
                Round,
                Ceil,
                Int,
                Float,
                Exp,
                Powf,
                Abs,
                Sgn,
            
                IntegerValue    = 0x8001,
                RealValue,

                EndOfFile = 0xFFFF
            };

            typedef unsigned int value_type;

            /**
             * Initializes a new SymbolType instance with the provided token.
             */
            SymbolType(_Domain value)
                : m_value(value)
            {}

            /**
             * Returns the symbol type.
             * @return The symbol type.
             */
            value_type value() const
            {
                return m_value;
            }

        private:
            value_type m_value;
    };
}

#endif  // __LIBFORMULA_SYMBOLTYPE_HPP
