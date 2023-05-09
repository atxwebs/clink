// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <lib/editor_module.h>

//------------------------------------------------------------------------------
class HostModule
    : public EditorModule
{
public:
                    HostModule(const char* host_name);
    virtual void    bind_input(Binder& binder) override;
    virtual void    on_begin_line(const Context& context) override;
    virtual void    on_end_line() override;
    virtual void    on_matches_changed(const Context& context) override;
    virtual void    on_input(const Input& Input, Result& result, const Context& context) override;
    virtual void    on_terminal_resize(int columns, int rows, const Context& context) override;

private:
    const char*     m_host_name;
};
