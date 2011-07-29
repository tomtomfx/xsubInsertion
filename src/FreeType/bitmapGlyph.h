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
# ifndef BITMAP_GLYPH
# define BITMAP_GLYPH

/** This class stores all informations needed to represent a glyph :
  * - a pixmap
  * - a size
  */
class BitmapGlyph {
private:
    /** the place font data representation is stored. One unsigned char is storing one glyph pixel*/
    unsigned char * m_pixmap;
    /* the width of the pixmap */
    int m_width;
    /** the height of the pixmap */
    int m_height;
    /** the offset to add to m_pixmap to go to the next line */
    int m_pitch;
    /** the horizontal increment to display the next char */
    int m_advanceX;
    /** the verical increment to display the next char */
    int m_advanceY;
    /** the distance between the base line and the top of the char */
    int m_bearingY;
    int m_offsetX;
    int m_offsetY;

public:
    /** Set the infos of the GlyphMap*/
    inline void setInfos (unsigned char * pixmap, int width, int height, int pitch, 
                          int offsetX, int offsetY, int bearingY, int advanceX, int advanceY) {
        m_pixmap = pixmap;
        m_width = width;
        m_height = height;
        m_pitch = pitch;
        m_advanceX = advanceX;
        m_advanceY = advanceY;
        m_bearingY = bearingY;
        m_offsetX= offsetX;
        m_offsetY = offsetY;
    }
    /** return a pointer to the pixmap */
    unsigned char * getPixmap () { return (m_pixmap); }
    /** return the pitch value */
    int getPitch() { return (m_pitch); }
    /** return the width value */
    int getWidth() { return (m_width); }
    /** return the height value */
    int getHeight() { return (m_height); }
    /** return the horizontal advance value */
    int getAdvanceX() { return (m_advanceX); }
    /** return the horizontal advance value */
    int getAdvanceY() { return (m_advanceY); }
    /** return the vertial bearing */
    int getBearingY() { return (m_bearingY); }
    int getOffsetX() { return (m_offsetX); }
    int getOffsetY() { return (m_offsetY); }
};
# endif