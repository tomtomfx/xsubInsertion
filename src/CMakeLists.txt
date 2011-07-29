cmake_minimum_required(VERSION 2.8)

include_directories(/usr/include/ /usr/include/freetype2/ ../build ${xsubInsertion_SOURCE_DIR}/src)
link_directories(/usr/lib/)

ADD_EXECUTABLE(xsubInsertion logger.cpp FreeType/glyph.cpp FreeType/font.cpp avi.cpp SubtitleEncoder.cpp subtitle.cpp xsubInsertion.cpp)

target_link_libraries(
  xsubInsertion
  libfreetype.so
)

install (TARGETS xsubInsertion DESTINATION bin)
