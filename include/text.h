#ifndef TEXT_H_
#define TEXT_H_

#include <map>

#include "ft2build.h"
#include FT_FREETYPE_H

#include "texture.h"
#include "object.h"


namespace graphics
{

class TextEngine
{
public:
    TextEngine()
    {
        auto error = FT_Init_FreeType(&_library);
        if (error)
        {
            std::cout << "Freetype init error" << "\n";
        }
    }

    void load_font_face(std::string name, std::string ttf_path)
    {
        _faces[name] = FT_Face();
        auto error = FT_New_Face(_library,
                                 ttf_path.c_str(),
                                 0,
                                 &_faces[name]);

        if (error == FT_Err_Unknown_File_Format)
        {
            std::cout << "Freetype unkown file format loading " << 
                      ttf_path << "\n";
        }
        else if (error)
        {
            std::cout << "Freetype error loading font face " << 
                         ttf_path << "\n";
        }

        error = FT_Set_Char_Size(
                    _faces[name],
                    0,
                    16*64,
                    300,
                    300);
    }

    FT_Bitmap * render_glyph(char in_char, std::string font_name)
    {
        FT_UInt glyph_index;
        glyph_index = FT_Get_Char_Index(_faces[font_name],
                                        in_char);
        auto error = FT_Load_Glyph(_faces[font_name],
                                   glyph_index,
                                   FT_LOAD_DEFAULT);
        if (error)
        {
            std::cout << "Error loading glyph" << "\n";
        }

        error = FT_Render_Glyph(_faces[font_name]->glyph,
                                FT_RENDER_MODE_NORMAL);
        if (error)
        {
            std::cout << "Error rendering glyph" << "\n";
        }

        return &_faces[font_name]->glyph->bitmap;
    }

private:
    FT_Library _library;
    std::map<std::string, FT_Face> _faces;
};

extern TextEngine g_engine;

class TextObject : public Object
{
public:
    TextObject(std::string text, int height, std::string font_name):
        _width(32*height),
        _height(height),
        _texture(_height, _width),
        _font_name(font_name)
    {
        std::vector<Vertex> vxs = {
                                   Vertex(0,0,0),
                                   Vertex(_width,0,0),
                                   Vertex(0,_height,0),
                                   Vertex(_width,0,0),
                                   Vertex(0,_height,0),
                                   Vertex(_width,_height,0)
                                  };
        load_vertices(vxs);
        set_text(text);
    }

    void set_text(std::string text)
    {
        for (int i = 0; i < text.length(); ++i)
        {
            auto bm = g_engine.render_glyph(text[0], _font_name);
            _texture.set_data(i*_height, 0, bm->width, bm->rows, reinterpret_cast<char*>(bm->buffer)); 
        }
    }

private:
    Texture2D _texture;
    int _width;
    int _height;
    std::string _font_name;
};

}  // namespace graphics

#endif  // TEXT_H_
