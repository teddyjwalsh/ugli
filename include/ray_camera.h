#ifndef RAY_CAMERA_H_
#define RAY_CAMERA_H_

#include "glm/glm.hpp"

class RayCamera
{
public:
    RayCamera(glm::vec3 pos, glm::vec3 lookat):
        _pos(pos),
        _theta(60*3.14159/180),
        _phi(60*3.14159/180),
        _f(0.1),
        _look_at(lookat)
    {
        _forward = glm::normalize(lookat - pos);
        _up = glm::vec3(0,1,0);
        _right = glm::cross(_forward, _up);
        _up = glm::cross(_right, _forward);
    }

    // Constant distance between pixel
    glm::vec3 get_ray(float x, float y)
    {
        glm::vec3 center = _pos + _forward*_f;
        float ud = _f*tan(_phi/2.0);
        glm::vec3 vec_up = _up*ud;
        float ld = _f*tan(_theta/2.0);
        glm::vec3 vec_left = (-_right)*ld;
        glm::vec3 full_right = vec_left*(-2.0f);
        glm::vec3 full_down = vec_up*(-2.0f);
        glm::vec3 top_left = center + vec_left + vec_up;
        glm::vec3 m_right = center - vec_left;
        glm::vec3 m_left = center + vec_left;
        glm::vec3 out_vec = glm::normalize(top_left + full_right*x + full_down*y - _pos);
        glm::vec3 out_vec_test = glm::normalize(top_left + full_right * 0.5f + full_down *0.5f - _pos);
        float dp = glm::dot(m_right - _pos, m_left - _pos)/(glm::length(m_left - _pos)*glm::length(m_right - _pos));
        float angle = acos(dp) * 180 / 3.14159;
        return out_vec;
    }

    // Constant angle between pixel
    glm::vec3 get_ray_angle(float theta, float phi)
    {
        _forward = glm::normalize(_look_at - _pos);
        _up = glm::vec3(0, 1, 0);
        _right = glm::cross(_forward, _up);
        _up = glm::cross(_right, _forward);
        glm::vec3 center = _pos + _forward * _f;
        float ud = _f * tan(_phi / 2.0);
        glm::vec3 vec_up = _up * ud;
        float ld = _f * tan(_theta / 2.0);
        glm::vec3 vec_left = (-_right) * ld;
        glm::vec3 full_right = vec_left * (-2.0f);
        glm::vec3 full_down = vec_up * (-2.0f);
        glm::vec3 top_left = center + vec_left + vec_up;
        float x = _f* tan(_theta / 2.0 - theta);
        float y = _f * tan(_phi / 2.0 - phi);
        glm::vec3 out_vec = glm::normalize(top_left + full_right * x + full_down * y - _pos);
        return out_vec;
    }

    glm::vec3 get_pos() const
    {
        return _pos;
    }

    float _f;
    float _theta;
    float _phi;

private:
    glm::vec3 _pos;
    glm::vec3 _forward;
    glm::vec3 _up;
    glm::vec3 _right;
    glm::vec3 _look_at;

};

#endif  // RAY_CAMERA_H_
