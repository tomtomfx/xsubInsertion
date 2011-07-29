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

#include "font.h"
#include <math.h>

Font::Font(FT_Library library, char * name)
{
    if (name != NULL)
    {
        m_name = strdup (name);
    }
    else
    {
        m_name = NULL;
    }
    m_library = library;
    m_face = NULL;
    m_useTransform = false;
    m_glyphsSize = MAX_UNICODE_GLYPHS;
    m_glyphs = new Glyph*[m_glyphsSize];
    for (unsigned int i = 0; i < m_glyphsSize; i++)
    {
        m_glyphs[i] = NULL;
    }
}

// clean up
Font::~Font()
{
    if (m_name != NULL)
    {
        free(m_name);
    }
    if (m_face != NULL)
    {
        FT_Done_Face (m_face);
    }
    if (m_glyphs != NULL)
    {
        for (unsigned int i = 0; i < m_glyphsSize; i++)
        {
            if (m_glyphs [i] != NULL)
            {
                delete m_glyphs [i];
            }
        }
        delete [] m_glyphs;
    }
}

// try to load the font
bool Font::load()
{
    if (m_name == NULL)
    {
        return (false);
    }
    m_resolution = 72;
    m_vPointSize = 20;
    m_hPointSize = 20;
    // let's try several fonts
    // first the name 'as is'
    if (FT_New_Face (m_library, m_name, 0, &m_face) == 0)
    {
        return (true);
    }
    return (false);
}

void Font::setSize (int hPointSize, int vPointSize )
{
    m_vPointSize = vPointSize;
    m_hPointSize = hPointSize;
    FT_Set_Pixel_Sizes (m_face, m_hPointSize, m_vPointSize);
}

void Font::setTransform (float scaleX, float scaleY, float angle)
{
    if (angle != 0)
    {
        m_useTransform = true;
        m_matrix.xx = (FT_Fixed)(cos(angle)*0x10000);
        m_matrix.xy = (FT_Fixed)(-sin(angle)*0x10000);
        m_matrix.yx = (FT_Fixed)(sin(angle)*0x10000);
        m_matrix.yy = (FT_Fixed)(cos(angle)*0x10000);
    }
    else
    {
        m_useTransform = false;
    }
}

bool Font::getGlyphInfo (unsigned long charCode, BitmapGlyph* bitmapGlyph)
{
    if (m_face == NULL || charCode >= MAX_UNICODE_GLYPHS)
    {
        return false;
    }

    /* retrieve glyph index from character code */
    //FT_UInt glyph_index = FT_Get_Char_Index( m_face, charCode );

    if (charCode >= m_glyphsSize && !resizeGlyphs(charCode))
    {
        return false;
    }

    Glyph* glyph = m_glyphs[charCode];
    // if the glyph is NOT already loaded, load it
    if (glyph == NULL)
    {
        glyph = m_glyphs[charCode] = new Glyph(m_face, charCode);
    }
    return (glyph->getInfo(bitmapGlyph, m_hPointSize, m_vPointSize));
}

bool Font::resizeGlyphs(int minSize)
{
    unsigned int i;
    Glyph**newGlyphs = new Glyph*[MAX_UNICODE_GLYPHS];
    if (newGlyphs == NULL)
    {
        return false;
    }
    for (i = 0; i < m_glyphsSize; i++)
    {
        newGlyphs[i] = m_glyphs[i];
    }
    for (i = m_glyphsSize; i < MAX_UNICODE_GLYPHS; i++)
    {
        newGlyphs[i] = NULL;
    }
    delete m_glyphs;
    m_glyphsSize = MAX_UNICODE_GLYPHS;
    m_glyphs = newGlyphs;
    return true;
}

