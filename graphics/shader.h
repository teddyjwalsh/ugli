#ifndef GRAPHICS_SHADER_H_
#define GRAPHICS_SHADER_H_

#include <map>
#include <string>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <GLFW/glfw3.h>

#include "graphics/gl_includes.h"
#include "graphics/buffer.h"
#include "graphics/definitions.h"
#include "graphics/texture.h"

namespace graphics
{

const std::string DEFAULT_VERSION_STRING = "#version 460 core";
const std::string DEFAULT_VERTEX_POSITION_VARIABLE = "v_pos";
const std::string DEFAULT_VERTEX_NORMAL_VARIABLE = "v_norm";
const std::string DEFAULT_INSTANCE_VARIABLE = "instance_var";

class ShaderBase
{
  public:
    ShaderBase(GLenum type):
        _shader_name(-1),
        _compiled(false)
    {
        _type = type; 
        _shader_name = glCreateShader(type);
    }
    
    void set_source_file(std::string file_path)
    {
        std::ifstream t(file_path);
        if (t)
        {
            _source = std::string((std::istreambuf_iterator<char>(t)),
                std::istreambuf_iterator<char>());
        }
        else
        {
            std::cout << "Couldn't find specified raytracing shader\n" << "\n";
        }
    }

    void compile()
    {
        const char* c_str = _source.c_str();
        glShaderSource(_shader_name, 1, &c_str, NULL);
        glCompileShader(_shader_name);

        GLint success;
        glGetShaderiv(_shader_name, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::cout << _source << "\n";
            std::cout << "Shader compilation failed: " << "\n";
            GLint log_size = 0;
            glGetShaderiv(_shader_name, GL_INFO_LOG_LENGTH, &log_size);
            std::vector<GLchar> log_output(log_size);
            glGetShaderInfoLog(_shader_name, log_size, NULL, log_output.data());
            std::string log_string(log_output.begin(), log_output.end());
            std::cout << log_string << "\n";
            _compiled = false;
        }
        else
        {
            _compiled = true;
        }
    }

    GLuint get_shader_id() const
    {
        return _shader_name;
    }

    GLenum get_type() const
    {
        return _type;
    }

    bool compiled() const
    {
        return _compiled;
    }


  private:
    GLuint _shader_name;
    GLenum _type;
    bool _compiled;
  protected:
    std::string _source;
};


class FileShader : public ShaderBase
{
  public:
    FileShader(GLuint type, std::string filepath) :
        ShaderBase(type),
        _filepath(filepath)
    {

    }

    virtual void build_source()
    {
        std::ifstream t(_filepath);
        _source = std::string((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());
    }

  private:
    std::string _filepath;
};

class Shader : public ShaderBase
{
  public:
    Shader(GLuint type, std::string file_path="") :
        ShaderBase(type)
    {
        _file = file_path;
    }


    void add_attrib(std::string name, std::string type)
    {
        _attribs[name] = type;
    }

    void add_output(std::string name, std::string type)
    {
        _outputs[name] = type;
    }

    void add_uniform(std::string name, std::string type)
    {
        _uniforms[name] = type;
    }

    virtual std::string build_head_specific()
    {
        std::string out_string;
        return out_string;
    }

    virtual std::string build_source_specific()
    {
        std::string out_string;
        return out_string;
    }

    virtual void build_source()
    {
        if (_file != "")
        {
            set_source_file(_file);
            return;
        }
        std::string out_string = DEFAULT_VERSION_STRING;

        out_string += "\n\n";

        int count = 0;

        out_string += build_head_specific();

        for (auto attrib : _attribs)
        {
            //out_string += "layout(location = " + std::to_string(count) + ") ";
            out_string += "in ";
            out_string += attrib.second;
            out_string += " ";
            out_string += attrib.first;
            out_string += ";\n";
            count += 1;
        }
        out_string += "\n";

        for (auto output : _outputs)
        {
            out_string += "out ";
            out_string += output.second;
            out_string += " ";
            out_string += output.first;
            out_string += ";\n";
        }
        out_string += "\n";

        for (auto uniform : _uniforms)
        {
            out_string += "uniform ";
            out_string += uniform.second;
            out_string += " ";
            out_string += uniform.first;
            out_string += ";\n";
        }
        out_string += "\n";
        out_string += "void main()\n{\n";
        out_string += build_source_specific();
        out_string += "}\n";
        std::cout << "SOURCE: " << out_string << "\n";
        _source = out_string;
    }

  private:
    std::map<std::string, std::string> _attribs;
    std::map<std::string, std::string> _outputs;
    std::map<std::string, std::string> _uniforms;
    std::string _file;
};


class FragmentShader : public Shader
{
public:
    FragmentShader() :
        Shader(GL_FRAGMENT_SHADER)
    {
        add_attrib("norm", "vec3");
        add_output("frag_color", "vec4");
    }

    virtual std::string build_source_specific()
    {
        std::string out_string;
        out_string += std::string("    vec3 sun_vec = -normalize(vec3(-1.0,-1.0,-1.0));\n");
        out_string += std::string("    frag_color = vec4(1.0, 1.0, 1.0, 1.0)*dot(norm,sun_vec);\n");

            
        return out_string;
    }
};

class VertexShader : public Shader
{
public:
    VertexShader() :
        Shader(GL_VERTEX_SHADER)
    {
        add_attrib(DEFAULT_VERTEX_POSITION_VARIABLE, "vec3");
        add_attrib(DEFAULT_VERTEX_NORMAL_VARIABLE, "vec3");
        add_attrib(DEFAULT_INSTANCE_VARIABLE, "vec3");
        add_uniform("model_mat", "mat4");
        add_uniform(VIEW_MAT_NAME, "mat4");
        add_uniform(PROJECTION_MAT_NAME, "mat4");
        add_output("norm", "vec3");
    }

    virtual std::string build_source_specific()
    {
        std::string out_string;
        out_string += std::string("    //float y_val = (floor(instance_var/(10*10)));\n");
        out_string += std::string("    //float z_val = (floor((instance_var - y_val*10*10)/10));\n");
        out_string += std::string("    //float x_val = (mod(instance_var, 10));\n");
        out_string += std::string("    norm = vec3(model_mat*vec4(v_norm, 1.0));\n");
        out_string += std::string("    gl_Position = projection_mat*view_mat*((model_mat*vec4(" +
            DEFAULT_VERTEX_POSITION_VARIABLE +
            ", 1.0)) + vec4(instance_var, 0.0));\n");
        return out_string;
    }
};

class ComputeShader : public Shader
{
public:
    ComputeShader(std::string file_path="") :
        Shader(GL_COMPUTE_SHADER, file_path)
    {
    }

    virtual std::string build_head_specific()
    {
        std::string out_string;
        out_string += "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
            "layout(binding = 0, rgba32f) uniform image1D cube_locs;\n"
            "layout(binding = 1, rgba32f) uniform image2D out_tex;\n"
            "// The camera specification\n"
            "uniform vec3 eye;\n"
            "uniform vec3 ray00;\n"
            "uniform vec3 ray10;\n"
            "uniform vec3 ray01;\n"
            "uniform vec3 ray11;\n";
            
        return out_string;
    }

    virtual std::string build_source_specific()
    {
        std::string out_string;
        //imageLoad(in_tex, ivec2(1,1))
        //out_string += "imageStore(out_tex, ivec2(0,0), vec4(0.6, 0.6, 0.6, 0.6));\n";
        //out_string += "imageStore(out_tex, ivec2(1,0), vec4(0.6, 0.6, 0.6, 0.6));\n";
        //out_string += "imageStore(in_tex, ivec2(0,0), vec4(0.7, 0.7, 0.7, 0.7));\n";
        //out_string += "imageStore(in_tex, ivec2(1,1), vec4(0.7, 0.7, 0.7, 0.7));\n";
        //i/10.0, j/10.0, k/10.0
        out_string += "ivec2 pix = ivec2(gl_GlobalInvocationID.xy);\
            ivec2 size = imageSize(out_tex);\
            int cube_tex_size = imageSize(cube_locs);\
            int count = 0;\
            for (int i = 0; i < cube_tex_size.x; ++i)\
            {\
                imageStore(cube_locs, i, vec4(i*1.0/100.0, 0.5, 0.5, cube_tex_size*1.0/100.0));\
                count += 1;\
            }\
            if (pix.x >= size.x || pix.y >= size.y) {\
                return;\
            }\
            vec2 pos = vec2(pix) / vec2(size.x, size.y);\
            vec4 color = vec4(0.6,0.6,0.6,0.6);\
            imageStore(out_tex, pix, color);";
        return out_string;
    }
};

class ScreenVertexShader : public VertexShader
{
public:
    virtual std::string build_source_specific()
    {
        std::string out_string;
        out_string += std::string("    gl_Position = vec4(" +
            DEFAULT_VERTEX_POSITION_VARIABLE +
            ", 1.0);\n");
        return out_string;
    }
};

class ScreenFragmentShader : public FragmentShader
{
public:
    ScreenFragmentShader()
    {
        add_uniform("raytraced", "sampler2D");
        add_uniform("screen_size", "vec3");
    }

    virtual std::string build_source_specific()
    {
        std::string out_string;
        out_string += "frag_color = texture2D(raytraced, gl_FragCoord.xy*1.0/screen_size.xy);// /1024.0).rgba;\n";
        return out_string;
    }
};

class Program
{
  public:
    Program()
    {
        _program = glCreateProgram();
    }

    void add_shader(std::shared_ptr<Shader> in_shader)
    {
        _shaders[in_shader->get_type()] = in_shader;
    }

    void attach_shader(std::shared_ptr<ShaderBase> in_shader)
    {
        glAttachShader(_program, in_shader->get_shader_id());
    }

    bool compile_and_link()
    {
        for (auto& shdr : _shaders)
        {
            if (!shdr.second->compiled())
            {
                attach_shader(shdr.second);
                shdr.second->build_source();
                shdr.second->compile();
                if (!shdr.second->compiled())
                {
                    return false;
                }
            }
        }
        link();
        return true;
    }

    void link()
    {
        glLinkProgram(_program);
        GLint success;
        glGetProgramiv(_program, GL_LINK_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::cout << " shader program link failed: " << "\n";
            GLint log_size = 0;
            glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &log_size);
            std::vector<GLchar> log_output(log_size);
            glGetProgramInfoLog(_program, log_size, NULL, log_output.data());
            std::string log_string(log_output.begin(), log_output.end());
            std::cout << log_string << "\n";
        }
    }


    void use()
    {
        set_program(_program);
    }


    GLuint get_program_name()
    {
        return _program;
    }

    void bind_image_texture(std::shared_ptr<Texture> in_tex, int index=0)
    {
        use();
        GLuint tex_name = in_tex->get_texture_name();
        //glBindImageTextures(index, 1, &tex_name);
        glBindImageTexture(index, tex_name, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    }

    template <class T>
    void bind_storage_buffer(std::shared_ptr<graphics::Buffer<T>> in_tex, int index)
    {
        use();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, in_tex->get_buffer_name());
    }

    void set_uniform_vec3(std::string var_name, glm::vec3 in_vec)
    {
        GLuint loc;
        if (_uniform_locs.find(var_name) == _uniform_locs.end())
        {
            _uniform_locs[var_name] = glGetUniformLocation(_program, var_name.c_str());
            loc = _uniform_locs[var_name];
        }
        else
        {
            loc = _uniform_locs[var_name];
        }
        use();
        glUniform3fv(loc, 1, glm::value_ptr(in_vec));
    }

    void set_uniform_float(std::string var_name, GLfloat in_val)
    {
        GLuint loc;
        if (_uniform_locs.find(var_name) == _uniform_locs.end())
        {
            _uniform_locs[var_name] = glGetUniformLocation(_program, var_name.c_str());
            loc = _uniform_locs[var_name];
        }
        else
        {
            loc = _uniform_locs[var_name];
        }
        use();
        glUniform1fv(loc, 1, &in_val);
    }

    void set_uniform_double(std::string var_name, GLdouble in_val)
    {
        GLuint loc;
        if (_uniform_locs.find(var_name) == _uniform_locs.end())
        {
            _uniform_locs[var_name] = glGetUniformLocation(_program, var_name.c_str());
            loc = _uniform_locs[var_name];
        }
        else
        {
            loc = _uniform_locs[var_name];
        }
        use();
        glUniform1dv(loc, 1, &in_val);
    }

    void set_uniform_int(std::string var_name, GLint in_val)
    {
        GLuint loc;
        if (_uniform_locs.find(var_name) == _uniform_locs.end())
        {
            _uniform_locs[var_name] = glGetUniformLocation(_program, var_name.c_str());
            loc = _uniform_locs[var_name];
        }
        else
        {
            loc = _uniform_locs[var_name];
        }
        use();
        glUniform1iv(loc, 1, &in_val);
    }

    void set_uniform_mat4(std::string var_name, glm::mat4 in_mat)
    {
        GLuint loc;
        if (_uniform_locs.find(var_name) == _uniform_locs.end())
        {
            _uniform_locs[var_name] = glGetUniformLocation(_program, var_name.c_str());
            loc = _uniform_locs[var_name];
        }
        else
        {
            loc = _uniform_locs[var_name];
        }
        use();
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(in_mat));
    }

    void run_compute_program(int x_width=1024, int y_width=1024)
    {
        use();
        glDispatchCompute(x_width, y_width, 1);
    }

  private:
    GLuint _program;
    std::map<GLenum, std::shared_ptr<Shader>> _shaders;
    std::map<std::string, int> _uniform_locs;
};

}  // namespace graphics

#endif  // GRAPHICS_SHADER_H_

