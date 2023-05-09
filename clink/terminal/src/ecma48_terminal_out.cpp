// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "ecma48_terminal_out.h"
#include "ecma48_iter.h"
#include "screen_buffer.h"

//------------------------------------------------------------------------------
Ecma48TerminalOut::Ecma48TerminalOut(ScreenBuffer& screen)
: m_screen(screen)
{
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::begin()
{
    m_screen.begin();
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::end()
{
    m_screen.end();
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::flush()
{
    m_screen.flush();
}

//------------------------------------------------------------------------------
int Ecma48TerminalOut::get_columns() const
{
    return m_screen.get_columns();
}

//------------------------------------------------------------------------------
int Ecma48TerminalOut::get_rows() const
{
    return m_screen.get_rows();
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::write_c1(const Ecma48Code& code)
{
    if (code.get_code() != Ecma48Code::c1_csi)
        return;

    Ecma48Code::Csi<32> csi;
    code.decode_csi(csi);

    if (csi.private_use)
    {
        switch (csi.final)
        {
        case 'h': set_private_mode(csi);    break;
        case 'l': reset_private_mode(csi);  break;
        }
    }
    else
    {
        switch (csi.final)
        {
        case '@': insert_chars(csi);        break;
        case 'H': set_cursor(csi);          break;
        case 'J': erase_in_display(csi);    break;
        case 'K': erase_in_line(csi);       break;
        case 'P': delete_chars(csi);        break;
        case 'm': set_attributes(csi);      break;

        case 'A': m_screen.move_cursor(0, -csi.get_param(0, 1)); break;
        case 'B': m_screen.move_cursor(0,  csi.get_param(0, 1)); break;
        case 'C': m_screen.move_cursor( csi.get_param(0, 1), 0); break;
        case 'D': m_screen.move_cursor(-csi.get_param(0, 1), 0); break;
        }
    }
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::write_c0(int c0)
{
    switch (c0)
    {
    case Ecma48Code::c0_bel:
        // TODO: flash cursor.
        break;

    case Ecma48Code::c0_bs:
        m_screen.move_cursor(-1, 0);
        break;

    case Ecma48Code::c0_cr:
        m_screen.move_cursor(INT_MIN, 0);
        break;

    case Ecma48Code::c0_ht: // TODO: perhaps there should be a next_tab_stop() method?
    case Ecma48Code::c0_lf: // TODO: shouldn't expect ScreenBuffer impl to react to '\n' characters.
        {
            char c = char(c0);
            m_screen.write(&c, 1);
            break;
        }
    }
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::write(const char* chars, int length)
{
    Ecma48Iter iter(chars, m_state, length);
    while (const Ecma48Code& code = iter.next())
    {
        switch (code.get_type())
        {
        case Ecma48Code::type_chars:
            m_screen.write(code.get_pointer(), code.get_length());
            break;

        case Ecma48Code::type_c0:
            write_c0(code.get_code());
            break;

        case Ecma48Code::type_c1:
            write_c1(code);
            break;
        }
    }
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::set_attributes(const Ecma48Code::CsiBase& csi)
{
    // Empty parameters to 'CSI SGR' implies 0 (reset).
    if (csi.param_count == 0)
        return m_screen.set_attributes(Attributes::defaults);

    // Process each code that is supported.
    Attributes attr;
    for (int i = 0; i < csi.param_count; ++i)
    {
        unsigned int param = csi.params[i];

        // Resets
        if (param == 0)  { attr = Attributes::defaults; continue; }
        if (param == 49) { attr.reset_bg();             continue; }
        if (param == 39) { attr.reset_fg();             continue; }

        // Bold/Underline
        if ((param == 1) | (param == 2) | (param == 22))
        {
            attr.set_bold(param == 1);
            continue;
        }

        if ((param == 4) | (param == 24))
        {
            attr.set_underline(param == 4);
            continue;
        }

        // Fore/background colours.
        if ((param - 30 < 8) | (param - 90 < 8))
        {
            param += (param >= 90) ? 14 : 2;
            attr.set_fg(param & 0x0f);
            continue;
        }

        if ((param - 40 < 8) | (param - 100 < 8))
        {
            param += (param >= 100) ? 4 : 8;
            attr.set_bg(param & 0x0f);
            continue;
        }

        // TODO: Rgb/xterm256 support for terminals that support it.
    }

    m_screen.set_attributes(attr);
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::erase_in_display(const Ecma48Code::CsiBase& csi)
{
    /* CSI ? Ps J : Erase in Display (DECSED).
            Ps = 0  -> Selective Erase Below (default).
            Ps = 1  -> Selective Erase Above.
            Ps = 2  -> Selective Erase All.
            Ps = 3  -> Selective Erase Saved Lines (xterm). */

    switch (csi.get_param(0))
    {
    case 0: m_screen.clear(ScreenBuffer::clear_type_after);     break;
    case 1: m_screen.clear(ScreenBuffer::clear_type_before);    break;
    case 2: m_screen.clear(ScreenBuffer::clear_type_all);       break;
    }
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::erase_in_line(const Ecma48Code::CsiBase& csi)
{
    /* CSI Ps K : Erase in Line (EL).
            Ps = 0  -> Erase to Right (default).
            Ps = 1  -> Erase to Left.
            Ps = 2  -> Erase All. */

    switch (csi.get_param(0))
    {
    case 0: m_screen.clear_line(ScreenBuffer::clear_type_after);    break;
    case 1: m_screen.clear_line(ScreenBuffer::clear_type_before);   break;
    case 2: m_screen.clear_line(ScreenBuffer::clear_type_all);      break;
    }
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::set_cursor(const Ecma48Code::CsiBase& csi)
{
    /* CSI Ps ; Ps H : Cursor Position [row;column] (default = [1,1]) (CUP). */
    int row = csi.get_param(0, 1);
    int column = csi.get_param(1, 1);
    m_screen.set_cursor(column - 1, row - 1);
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::insert_chars(const Ecma48Code::CsiBase& csi)
{
    /* CSI Ps @  Insert Ps (Blank) Character(s) (default = 1) (ICH). */
    int count = csi.get_param(0, 1);
    m_screen.insert_chars(count);
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::delete_chars(const Ecma48Code::CsiBase& csi)
{
    /* CSI Ps P : Delete Ps Character(s) (default = 1) (DCH). */
    int count = csi.get_param(0, 1);
    m_screen.delete_chars(count);
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::set_private_mode(const Ecma48Code::CsiBase& csi)
{
    /* CSI ? Pm h : DEC Private Mode Set (DECSET).
            Ps = 5  -> Reverse Video (DECSCNM).
            Ps = 12 -> Start Blinking Cursor (att610).
            Ps = 25 -> Show Cursor (DECTCEM). */
    for (int i = 0; i < csi.param_count; ++i)
    {
        switch (csi.params[i])
        {
        case 12: /* TODO */ break;
        case 25: /* TODO */ break;
        }
    }
}

//------------------------------------------------------------------------------
void Ecma48TerminalOut::reset_private_mode(const Ecma48Code::CsiBase& csi)
{
    /* CSI ? Pm l : DEC Private Mode Reset (DECRST).
            Ps = 5  -> Normal Video (DECSCNM).
            Ps = 12 -> Stop Blinking Cursor (att610).
            Ps = 25 -> Hide Cursor (DECTCEM). */
    for (int i = 0; i < csi.param_count; ++i)
    {
        switch (csi.params[i])
        {
        case 12: /* TODO */ break;
        case 25: /* TODO */ break;
        }
    }
}
