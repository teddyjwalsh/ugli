#ifndef TEXT_H_
#define TEXT_H_

#include <map>

#include "ft2build.h"
#include FT_FREETYPE_H

#include "general.h"
#include "texture.h"
#include "object.h"


namespace graphics
{

class TextEngine
{
public:
    TextEngine():
        _size(256)
    {
        auto error = FT_Init_FreeType(&_library);
        
        if (error)
        {
            std::cout << "Freetype init error" << "\n";
        }
        load_font_face("arial", "../fonts/arial.ttf");
        load_font_face("helvetica", "../fonts/Helvetica 400.ttf");
        load_font_face("roboto", "../fonts/Roboto-Regular.ttf");
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
                    16*_size,
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
                                   FT_LOAD_RENDER);
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
    
    int _size;

private:
    FT_Library _library;
    std::map<std::string, FT_Face> _faces;
};

extern TextEngine g_engine;

class TextObject : public Object
{
public:
    TextObject(std::string text, int height, std::string font_name) :
        _width(10 * height),
        _height(height),
        _texture(std::make_shared<Texture2D>(10 * g_engine._size, g_engine._size, GL_RED, GL_UNSIGNED_BYTE)),
        _font_name(font_name)
    {
        std::vector<Vertex> vxs = {
                                   Vertex(0,0,0),
                                   Vertex(_width,0,0),
                                   Vertex(0,_height,0),
                                   Vertex(_width,_height,0),
                                   Vertex(0,_height,0),
                                   Vertex(_width,0,0),                        
        };
        std::vector<glm::vec2> uvs = {
                           glm::vec2(0,g_engine._size),
                           glm::vec2(10*g_engine._size,g_engine._size),
                           glm::vec2(0,0),
                           glm::vec2(10*g_engine._size,0),
                           glm::vec2(0,0),
                           glm::vec2(10*g_engine._size,g_engine._size),
        };
        std::vector<unsigned char> fake_data(_width * height, 128);
        load_uvs(uvs);
        load_vertices(vxs);
        //_texture->set_data(0, 0, _width, _height, (unsigned char*)fake_data.data());
        set_text(text);
        std::shared_ptr<Shader> v_shader = std::make_shared<Shader>(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
        std::shared_ptr<Shader> f_shader = std::make_shared<Shader>(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
        _shader = std::make_shared<Program>();
        _shader->add_shader(v_shader);
        _shader->add_shader(f_shader);
        _shader->compile_and_link();
        set_shader(_shader);
        set_pos(glm::vec3(100, 100, 0));
        set_rot(glm::mat4(1));
        int x_res, y_res;
        get_window_size(x_res, y_res);
        //set_scale(glm::vec3(2 * 1.0f / x_res, 2 * 1.0f / x_res, 0));
        _shader->bind_image_texture(_texture, 0);
    }

    void set_text(std::string text)
    {
        int slide = 0;
        for (int i = 0; i < text.length(); ++i)
        {
            auto bm = g_engine.render_glyph(text[i], _font_name);
            std::vector<unsigned char> fake_data(bm->width * _height, 128);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            //_texture->set_data(i * bm->width, 0, bm->width, _height, (unsigned char*)fake_data.data());
            _texture->set_data(slide, g_engine._size - bm->rows, bm->width, bm->rows, (unsigned char*)bm->buffer);
            slide += bm->width + _height*0.2;
        }
    }

private:
    std::shared_ptr<Texture2D> _texture;
    std::shared_ptr<Program> _shader;
    int _width;
    int _height;
    std::string _font_name;
};

}  // namespace graphics

#endif  // TEXT_H_
