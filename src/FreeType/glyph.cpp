/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2010  Thomas Fayoux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "glyph.h"


Glyph::Glyph(FT_Face face, unsigned int charCode)
{
    m_face = face;
    m_glyph = NULL;
    m_bitmap = NULL;
    m_hSize = m_vSize = -1;
    m_charCode = charCode;

    // find the index
    m_index = FT_Get_Char_Index (m_face, charCode);
    // load the glyph
    FT_Load_Glyph (m_face, m_index,FT_LOAD_NO_BITMAP /*| FT_LOAD_NO_HINTING |FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH*/ );;
    // retrieve the glyph
    FT_Get_Glyph (m_face->glyph, &m_glyph);
    m_bearingY = m_face->glyph->metrics.horiBearingY >> 6;
}

Glyph::~Glyph()
{
    FT_Done_Glyph (m_glyph);
}

void Glyph::render()
{
    // render it
    FT_Glyph_To_Bitmap (&m_glyph, ft_render_mode_normal, 0, 1);
    m_bitmap = (FT_BitmapGlyph)m_glyph;
}

bool Glyph::getInfo (BitmapGlyph* bitmapGlyph, int vSize, int hSize)
{
    if (vSize != m_vSize || hSize != m_hSize)
    {
        m_vSize = vSize;
        m_hSize = hSize;
        FT_Done_Glyph (m_glyph);
        // load the glyph
        FT_Error error = FT_Load_Glyph (m_face, m_index,FT_LOAD_NO_BITMAP /*| FT_LOAD_NO_HINTING |FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH*/ );
        /* convert to an anti-aliased bitmap */
        error = FT_Render_Glyph( m_face->glyph, FT_RENDER_MODE_NORMAL );
        // retrieve the glyph
        FT_Get_Glyph (m_face->glyph, &m_glyph);
        m_bearingY = m_face->glyph->metrics.horiBearingY >> 6;
    }
    render();
    int xAdvance = m_glyph->advance.x >> 16;
    bitmapGlyph->setInfos (m_bitmap->bitmap.buffer, m_bitmap->bitmap.width, m_bitmap->bitmap.rows, m_bitmap->bitmap.pitch, m_bitmap->left, m_bitmap->top, m_bearingY, xAdvance, -m_glyph->advance.y >> 16);
    return (true);
}
