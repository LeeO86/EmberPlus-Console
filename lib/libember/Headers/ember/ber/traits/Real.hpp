/*
    libember -- C++ 03 implementation of the Ember+ Protocol

    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef __LIBEMBER_BER_TRAITS_REAL_HPP
#define __LIBEMBER_BER_TRAITS_REAL_HPP

#include <limits>
#include "CodecTraits.hpp"
#include "RegisterDecoder.hpp"
#include "Integral.hpp"
#include "../../meta/FunctionTraits.hpp"
#include "../../util/TypePun.hpp"

//SimianIgnore

namespace libember { namespace ber
{
    namespace detail
    {
        /**
         * Common helper implementation for floating point types of all
         * bit-widths.
         */
        struct RealUniversalTagTraits
        {
            static Tag universalTag()
            {
                return make_tag(Class::Universal, Type::Real);
            }
        };

        /**
         * Common helper implementation for floating point types of all
         * bit-widths.
         */
        template<typename RealType>
        struct RealEncodingTraits
        {
            typedef RealType value_type;

            static void encode(util::OctetStream& output, value_type value)
            {
                if (value == +std::numeric_limits<value_type>::infinity())
                {
                    // 0x40 Indicates positive infinity
                    output.append(0x40);
                }
                else if (value == -std::numeric_limits<value_type>::infinity())
                {
                    // 0x41 Indicates positive infinity
                    output.append(0x41);
                }
                else if (
                    value == std::numeric_limits<value_type>::quiet_NaN() ||
                    value == std::numeric_limits<value_type>::signaling_NaN())
                {
                    // 0x42 Indicates NaN
                    output.append(0x42);
                }
                else
                {
                    double const real = value;
                    unsigned long long const bits = util::type_pun<unsigned long long>(real);

                    if (bits != 0)
                    {
                        long long const exponent = ((0x7FF0000000000000LL & bits) >> 52) - 1023;
                        unsigned long long mantissa = 0x000FFFFFFFFFFFFFULL & bits;
                        mantissa |= 0x0010000000000000ULL;

                        while((mantissa & 0xFF) == 0x00)
                            mantissa >>= 8;

                        while((mantissa & 0x01) == 0x00)
                            mantissa >>= 1;

                        std::size_t const exponentLength = ber::encodedLength<long long>(exponent);
                        unsigned char preamble = static_cast<unsigned char>(0x80 | (exponentLength - 1));

                        if((bits & 0x8000000000000000ULL) != 0)
                            preamble |= 0x40;

                        output.append(preamble);
                        ber::encode<long long>(output, exponent);
                        ber::encode<unsigned long long>(output, mantissa);
                    }
                }
            }

            static std::size_t encodedLength(value_type value)
            {
                if (value == +std::numeric_limits<value_type>::infinity()
                ||  value == -std::numeric_limits<value_type>::infinity()
                ||  value == std::numeric_limits<value_type>::quiet_NaN()
                ||  value == std::numeric_limits<value_type>::signaling_NaN())
                {
                    return 1;
                }
                else
                {
                    std::size_t encodedLength = 0;
                    double const real = value;
                    unsigned long long const bits = util::type_pun<unsigned long long>(real);

                    if (bits != 0)
                    {
                        long long const exponent = ((0x7FF0000000000000LL & bits) >> 52) - 1023;
                        unsigned long long mantissa =   0x000FFFFFFFFFFFFFULL & bits;
                        mantissa |= 0x0010000000000000ULL;

                        while((mantissa & 0xFF) == 0x00)
                            mantissa >>= 8;

                        while((mantissa & 0x01) == 0x00)
                            mantissa >>= 1;

                        encodedLength += 1;     // preamble
                        encodedLength += ber::encodedLength<long long>(exponent);
                        encodedLength += ber::encodedLength<unsigned long long>(mantissa);
                    }

                    return encodedLength;
                }
            }
        };


        /**
         * Common helper implementation for floating point types of all
         * bit-widths.
         */
        template<typename RealType>
        struct RealDecodingTraits
        {
            typedef RealType value_type;
            /**
             * Traits type providing various infos on the decode functions
             * signature.
             * Unfortunately C++03 does not yet support a library independent
             * typeof/decltype operation, which is why we have to
             * 1. explicitly repeat the signature here and
             * 2. cannot defer the introspection work to the freestanding
             *    decode function.
             */
            typedef meta::FunctionTraits<value_type (*)(util::OctetStream&, std::size_t)> signature;

            static value_type decode(util::OctetStream& input, std::size_t encodedLength)
            {
                if (encodedLength == 0)
                    return static_cast<value_type>(0.0);

                unsigned char const preamble = input.front();
                input.consume();

                if (encodedLength == 1 && preamble == 0x40)
                {
                    return +std::numeric_limits<value_type>::infinity();
                }
                else if (encodedLength == 1 && preamble == 0x41)
                {
                    return -std::numeric_limits<value_type>::infinity();
                }
                else if (encodedLength == 1 && preamble == 0x42)
                {
                    return std::numeric_limits<value_type>::quiet_NaN();
                }
                else
                {
                    unsigned long long bits = 0;
                    unsigned int const sign = (preamble & 0x40);
                    unsigned int const exponentLength = 1 + (preamble & 3);
                    unsigned int const mantissaShift = ((preamble >> 2) & 3);

                    long long exponent = ber::decode<long long>(input, exponentLength);
                    unsigned long long mantissa = ber::decode<unsigned long long>(input, encodedLength - exponentLength - 1) << mantissaShift;

                    while((mantissa & 0x7FFFF00000000000ULL) == 0x00)
                        mantissa <<= 8;

                    while((mantissa & 0x7FF0000000000000ULL) == 0x00)
                        mantissa <<= 1;

                    mantissa &= 0x0FFFFFFFFFFFFFULL;
                    bits = (static_cast<unsigned long long>(exponent + 1023) << 52) | mantissa;

                    if (sign != 0)
                        bits |= (0x8000000000000000ULL);

                    double const real = util::type_pun<double>(bits);
                    return static_cast<value_type>(real);
                }
            }
        };
    }

    /** UniversalTagTraits specialization for the float type. */
    template<>
    struct UniversalTagTraits<float>
        : detail::RealUniversalTagTraits
    {};

    /** EncodingTraits specialization for the float type. */
    template<>
    struct EncodingTraits<float>
        : detail::RealEncodingTraits<float>
    {};

    /** DecodingTraits specialization for the float type. */
    template<>
    struct DecodingTraits<float>
        : detail::RealDecodingTraits<float>
    {};

    /** UniversalTagTraits specialization for the double type. */
    template<>
    struct UniversalTagTraits<double>
        : detail::RealUniversalTagTraits
    {};

    /** EncodingTraits specialization for the double type. */
    template<>
    struct EncodingTraits<double>
        : detail::RealEncodingTraits<double>
    {};

    /** DecodingTraits specialization for the double type. */
    template<>
    struct DecodingTraits<double>
        : detail::RealDecodingTraits<double>
    {};

    /**
     * Helper template instances that makes the decode function
     * for the various floating point types dynamically available.
     */
    RegisterDecoder<float > const _floatDecoderRegistrar;
    RegisterDecoder<double> const _doubleDecoderRegistrar;
}
}

//EndSimianIgnore

#endif  // __LIBEMBER_BER_TRAITS_REAL_HPP

