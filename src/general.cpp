
#include <stdio.h>

#include <glm/gtc/type_ptr.hpp>

#include "gl_includes.h"
#include "camera.h"
#include "object.h"
#include "shader.h"
#include "texture.h"
#include "definitions.h"

namespace graphics
{

    GLuint active_program;
    GLFWwindow* window;
    std::shared_ptr<Camera> active_cam;
    std::shared_ptr<Texture> draw_target;
    std::shared_ptr<Program> active_program_ptr;
    int x_res;
    int y_res;

    void set_program(GLuint in_program)
    {
        if (in_program != active_program)
        {
            active_program = in_program;
            glUseProgram(in_program);
        }
    }

    void set_program(std::shared_ptr<Program> in_program)
    {
        if (in_program != active_program_ptr)
        {
            active_program_ptr = in_program;
            glUseProgram(in_program->get_program_name());
        }
    }

    void set_active_camera(std::shared_ptr<Camera> in_cam)
    {
        active_cam = in_cam;
    }

    void set_camera_matrices(std::shared_ptr<Camera> in_camera)
    {
        active_program_ptr->set_uniform_mat4(VIEW_MAT_NAME, in_camera->get_view());
        active_program_ptr->set_uniform_mat4(PROJECTION_MAT_NAME, in_camera->get_projection());
        /*
        GLuint view_mat_loc = glGetUniformLocation(active_program, VIEW_MAT_NAME.c_str());
        GLuint projection_mat_loc = glGetUniformLocation(active_program, PROJECTION_MAT_NAME.c_str());
        glm::mat4 view_mat = in_camera->get_view();
        glm::mat4 projection_mat = in_camera->get_projection();
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, glm::value_ptr(view_mat));
        glUniformMatrix4fv(projection_mat_loc, 1, GL_FALSE, glm::value_ptr(projection_mat));
        */
    }

    void set_camera_matrices(const Camera* in_camera)
    {
        active_program_ptr->set_uniform_mat4(VIEW_MAT_NAME, in_camera->get_view());
        active_program_ptr->set_uniform_mat4(PROJECTION_MAT_NAME, in_camera->get_projection());
        /*
        GLuint view_mat_loc = glGetUniformLocation(active_program, VIEW_MAT_NAME.c_str());
        GLuint projection_mat_loc = glGetUniformLocation(active_program, PROJECTION_MAT_NAME.c_str());
        glm::mat4 view_mat = in_camera->get_view();
        glm::mat4 projection_mat = in_camera->get_projection();
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, glm::value_ptr(view_mat));
        glUniformMatrix4fv(projection_mat_loc, 1, GL_FALSE, glm::value_ptr(projection_mat));
        */
    }

    std::shared_ptr<Camera> get_active_camera()
    {
        return active_cam;
    }

    void error_callback(int error, const char* description)
    {
        fprintf(stderr, "Error: %s\n", description);
    }

    void start_loop()
    {
        int width, height;
        glfwPollEvents();
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void end_loop()
    {
        //if (!draw_target)
        //{
        glfwSwapBuffers(window);
        //}
    }

    void terminate()
    {
        glfwTerminate();
    }

    void resize_callback(GLFWwindow* in_window, int x, int y)
    {
        x_res = x;
        y_res = y;
    }

    void init(int in_x_res, int in_y_res)
    {
        // INITIALIZATION
        if (!glfwInit())
        {
            printf("Failed GLFW init!");
        }

        glfwSetErrorCallback(error_callback);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        x_res = in_x_res;
        y_res = in_y_res;
        window =
            glfwCreateWindow(in_x_res, in_y_res, "GL window", NULL, NULL);
        if (!window)
        {
            printf("Failed window creation!");
        }

        glfwMakeContextCurrent(window);
        gladLoadGL();
        glfwSetWindowSizeCallback(window, resize_callback);
        
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    }

    void draw_object(std::shared_ptr<Object> in_object)
    {
        set_program(in_object->get_program());
        in_object->bind();
        in_object->get_program()->set_uniform_mat4("model_mat", in_object->get_transform());
        in_object->get_program()->set_uniform_int("x_res", x_res);
        in_object->get_program()->set_uniform_int("y_res", y_res);
        //GLuint model_mat_loc = glGetUniformLocation(
        //    in_object->get_program()->get_program_name(), "model_mat");
        //glm::mat4 model_mat = in_object->get_transform();
        //glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, glm::value_ptr(model_mat));
        if (in_object->is_instanced())
        {
            glDrawArraysInstanced(GL_TRIANGLES,
                0,
                in_object->vertex_count(),
                in_object->get_instance_count());
        }
        else
        {
            glDrawArrays(GL_TRIANGLES,
                0,
                in_object->vertex_count());
        }
    }

    void draw_object(std::shared_ptr<Object> in_object,
        std::shared_ptr<Camera> in_camera)
    {
        set_program(in_object->get_program());
        set_camera_matrices(in_camera);
        in_object->bind();
        in_object->get_program()->set_uniform_mat4("model_mat", in_object->get_transform());
        //GLuint model_mat_loc = glGetUniformLocation(
        //    in_object->get_program()->get_program_name(), "model_mat");
        //glm::mat4 model_mat = in_object->get_transform();
        //glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, glm::value_ptr(model_mat));
        if (in_object->is_instanced())
        {
            glDrawArraysInstanced(GL_TRIANGLES,
                0,
                in_object->vertex_count(),
                in_object->get_instance_count());
        }
        else
        {
            glDrawArrays(GL_TRIANGLES,
                0,
                in_object->vertex_count());
        }
    }

    void draw_object(std::shared_ptr<Object> in_object,
        const Camera* in_camera)
    {
        set_program(in_object->get_program());
        set_camera_matrices(in_camera);
        in_object->bind();
        in_object->get_program()->set_uniform_mat4("model_mat", in_object->get_transform());
        //GLuint model_mat_loc = glGetUniformLocation(
        //    in_object->get_program()->get_program_name(), "model_mat");
        //glm::mat4 model_mat = in_object->get_transform();
        //glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, glm::value_ptr(model_mat));
        if (in_object->is_instanced())
        {
            glDrawArraysInstanced(GL_TRIANGLES,
                0,
                in_object->vertex_count(),
                in_object->get_instance_count());
        }
        else
        {
            glDrawArrays(GL_TRIANGLES,
                0,
                in_object->vertex_count());
        }
    }

    void set_draw_target(std::shared_ptr<Texture2D> in_tex)
    {
        if (in_tex)
        {
            if (!in_tex->rendering_enabled())
            {
                in_tex->enable_rendering(true);
            }
            in_tex->draw_to_texture();
            draw_target = in_tex;
        }
        else
        {
            draw_target = in_tex;
        }
    }

    bool window_should_close()
    {
        return glfwWindowShouldClose(window);
    }

    void get_window_size(int& x_size, int& y_size)
    {
        glfwGetWindowSize(window, &x_size, &y_size);
    }

}  // namespace graphics
