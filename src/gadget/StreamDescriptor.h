/*
    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef __TINYEMBER_GADGET_STREAMDESCRIPTOR_H
#define __TINYEMBER_GADGET_STREAMDESCRIPTOR_H

#include "StreamFormat.h"

namespace gadget
{
    /**
     * Class describing the format and offset of a stream.
     */
    class StreamDescriptor
    {
        public:
            typedef std::size_t offset_type;

            /**
             * Initializes a new StreamDescriptor.
             * @param format The format of the stream value.
             * @param offset The offset of the value within the stream.
             */
            StreamDescriptor(StreamFormat const& format, offset_type offset);

            /**
             * Returns the offset of the value within the stream.
             * @return The offset of the value within the stream.
             */
            offset_type offset() const;

            /**
             * Returns the format of the stream value.
             * @return The format of the stream value.
             */
            StreamFormat const& format() const;

        private:
            StreamFormat m_format;
            offset_type m_offset;
    };


    /**************************************************************************
     * Inline implementation                                                  *
     **************************************************************************/

    inline StreamDescriptor::StreamDescriptor(StreamFormat const& format, offset_type offset)
        : m_format(format)
        , m_offset(offset)
    {}

    inline StreamDescriptor::offset_type StreamDescriptor::offset() const
    {
        return m_offset;
    }

    inline StreamFormat const& StreamDescriptor::format() const
    {
        return m_format;
    }
}

#endif//__TINYEMBER_GADGET_STREAMDESCRIPTOR_H
