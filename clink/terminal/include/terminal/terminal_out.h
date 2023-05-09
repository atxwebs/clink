// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "attributes.h"

//------------------------------------------------------------------------------
class TerminalOut
{
public:
    virtual                 ~TerminalOut() = default;
    virtual void            begin() = 0;
    virtual void            end() = 0;
    virtual void            write(const char* chars, int length) = 0;
    template <int S> void   write(const char (&chars)[S]);
    virtual void            flush() = 0;
    virtual int             get_columns() const = 0;
    virtual int             get_rows() const = 0;
};

//------------------------------------------------------------------------------
template <int S> void TerminalOut::write(const char (&chars)[S])
{
    write(chars, S);
}
