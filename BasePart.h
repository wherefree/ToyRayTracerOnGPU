/*
管理渲染场景中每一个图元的基本位置 法向量 uv，以及其对应的VAO
提供对基本图元的渲染操作
提供图元数据传输到GPU的操作
*/
#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include<vector>
#include<glm/glm.hpp>

enum MatrialType { Dielectric = 1, Lambertian = 2, Metal = 3, Light = 4, Isotropic = 5 };
class BasePart
{
public:
    BasePart():VAO(0),VBO(0),EBO(0), texture_id(0){}
    BasePart(MatrialType matrial, glm::vec3 color) :BasePart() {
        Matrial = matrial, Color = color;
    }
    void CreatVAO();//图元数据传输到GPU,数据需更新时VAO默认为0
    virtual void Draw();//简单的画个图
    void setVAO(unsigned int vao);
    unsigned int getTexture() { return texture_id; }
    virtual void Creat()=0;

public:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;
    glm::vec3 Color;
    int Matrial;
    unsigned int VAO;
private:
    unsigned int VBO;
    unsigned int EBO;
    int indexCount;
    unsigned int texture_id;
    
};

