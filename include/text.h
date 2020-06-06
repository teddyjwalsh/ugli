#ifndef TEXT_H_
#define TEXT_H_

#include <map>

#include "ft2build.h"
#include FT_FREETYPE_H


class TextEngine
{
public:
    TextEngine()
    {
        auto error = FT_Init_FreeType(&library);
        if (error)
        {
            std::cout << "Freetype init error" << "\n";
        }
    }

    void load_font_face(std::string name, std::string ttf_path)
    {
        _faces[name] = FT_Face();
        auto error = FT_New_Face(library,
                                 ttf_path,
                                 0,
                                 &face);

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

private:
    FT_Library _library;
    std::map<std::string, FT_Face> _faces;
};

#endif  // TEXT_H_
