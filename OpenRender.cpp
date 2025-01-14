#define _CRT_SECURE_NO_WARNINGS

#include "OpenRender.h"
#include "Quad.h"
#include "Sphere.h"
#include "Bvh.h"
#include "windows.h"
#include<windef.h>

Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
GLboolean hdr = true; // Change with 'Space'
GLfloat exposure = 1.0f; // Change with Q and E
bool firstMouse = true;
bool blinn = false;
bool blinnKeyPressed = false;
// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
int init();
GLFWwindow* window;
bool movingMouse = false;
bool pressingKey = false;


//将颜色数据写到一个BMP文件中
//FileName:文件名
//ColorBuffer:颜色数据
//ImageWidth:图像宽度
//ImageHeight:图像长度
bool WriteBitmapFile(const char* filename, int wid, int hei, unsigned char* bitmapData)
{
    int width = wid;
    int height = hei;

    BITMAPFILEHEADER bitmapFileHeader;
    memset(&bitmapFileHeader, 0, sizeof(BITMAPFILEHEADER));
    bitmapFileHeader.bfSize = sizeof(BITMAPFILEHEADER);
    bitmapFileHeader.bfType = 0x4d42;	//BM
    bitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    //填充BITMAPINFOHEADER
    BITMAPINFOHEADER bitmapInfoHeader;
    memset(&bitmapInfoHeader, 0, sizeof(BITMAPINFOHEADER));
    bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfoHeader.biWidth = width;
    bitmapInfoHeader.biHeight = height;
    bitmapInfoHeader.biPlanes = 1;
    bitmapInfoHeader.biBitCount = 24;
    bitmapInfoHeader.biCompression = BI_RGB;
    bitmapInfoHeader.biSizeImage = width * abs(height) * 3;

    //
    FILE* filePtr;
    unsigned char tempRGB;
    int imageIdx;

    //swap R B
    for (imageIdx = 0; imageIdx < bitmapInfoHeader.biSizeImage; imageIdx += 3)
    {
        tempRGB = bitmapData[imageIdx];
        bitmapData[imageIdx] = bitmapData[imageIdx + 2];
        bitmapData[imageIdx + 2] = tempRGB;
    }

    filePtr = fopen(filename, "wb");
    if (NULL == filePtr)
    {
        return false;
    }

    fwrite(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

    fwrite(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

    fwrite(bitmapData, bitmapInfoHeader.biSizeImage, 1, filePtr);

    fclose(filePtr);
    return true;
}


void snap_shot(int img_w, int img_h)
{
    GLubyte* pPixelData;
    GLint line_width;
    GLint PixelDataLength;

    line_width = img_w * 3; // 得到每一行的像素数据长度 
    line_width = (line_width + 3) / 4 * 4;

    PixelDataLength = line_width * img_h;

    // 分配内存和打开文件 
    pPixelData = (GLubyte*)malloc(PixelDataLength);
    if (pPixelData == 0)
        exit(0);


    // 读取像素 
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    //glReadPixels(0, 0, img_w, img_h, GL_BGR_EXT, GL_UNSIGNED_BYTE, pPixelData);
    glReadPixels(0, 0, img_w, img_h, GL_RGB, GL_UNSIGNED_BYTE, pPixelData);


    WriteBitmapFile("snapshot.bmp", img_w, img_h, pPixelData);
    free(pPixelData);
}




OpenRender::OpenRender(Camera camera1, glm::mat4 projection1) {
    init();//OpenGL的杂七杂八初始化

    camera = camera1, projection = projection1;
    
    //延迟渲染输出平面
    quad = new Quad();
    quad->Creat();
    

    //延迟渲染shader
    //Shader shader("shader/deffered.vs", "shader/RayTrace1.fs");
    Shader shader("shader/deffered.vs", "shader/RayTrace_Pdf.fs");
    defferedShader = shader;
    Shader shader1("shader/Draw.vs", "shader/Draw.fs");
    drawShader = shader1;

    //延迟渲染的Gbuffer 结构为 vec3 Pos;vec3 normal;vec3 color;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    glGenTextures(1, &gColorSpec);
    glBindTexture(GL_TEXTURE_2D, gColorSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);
    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // - Create and attach depth buffer (renderbuffer)
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    // - Finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //记录上一帧的渲染结果 lastColor
    glGenFramebuffers(1, &lastColorBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, lastColorBuffer);
    glGenTextures(1, &gLastColor);
    glBindTexture(GL_TEXTURE_2D, gLastColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gLastColor, 0);
    GLuint attachments1 = GL_COLOR_ATTACHMENT0;
    glDrawBuffer(attachments1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    defferedShader.use();
    defferedShader.setInt("position", 0);
    defferedShader.setInt("normal", 1);
    defferedShader.setInt("gColor", 2);
    defferedShader.setInt("TriangleList", 3);
    defferedShader.setInt("BVHList", 4);
    defferedShader.setInt("LastColor", 5);


    drawShader.use();
    drawShader.setInt("drawColor", 6);
    //glEnable(GL_CULL_FACE);//背面剔除debug
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//线框模式debug
}

//延迟渲染
void OpenRender::Draw(std::shared_ptr<BasePartList> partlist,Shader shader)
{
    //将渲染信息写入Gbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        partlist->Draw(shader, camera, projection);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    static int frameCounter = 1;
    //frameCounter++;
    //写入lastColor
    defferedShader.use();
    for (int i = 0; i < 1; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, lastColorBuffer);

        //用Gbuffer的信息进一步渲染

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gColorSpec);

        //glActiveTexture(GL_TEXTURE3);
        //glBindTexture(GL_TEXTURE_BUFFER, partlist->bvh->textureTri);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_BUFFER, partlist->bvh->textureBvh);


        defferedShader.setInt("width", SCR_WIDTH);
        defferedShader.setInt("height", SCR_HEIGHT);
        defferedShader.setInt("numTri", partlist->bvh->TriNum);
        defferedShader.setVec3("eye", camera.Position);
        partlist->LoadLight(defferedShader);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, gLastColor);
        //if(i==0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            frameCounter++;
            defferedShader.setInt("frameCounter", frameCounter);
            quad->CreatVAO();
            quad->Draw();
       
    } 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    drawShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE6);
    drawShader.setFloat("exposure", 1.f);
    glBindTexture(GL_TEXTURE_2D, gLastColor);
    quad->CreatVAO();
    quad->Draw();
    std::cout << frameCounter << '\n';
}

void OpenRender::setRenderQueue(std::shared_ptr<BasePartList> basepart)
{
    std::shared_ptr < Bvh > bvh = make_shared<Bvh>(basepart);
    basepart->bvh = bvh;
    renderQueue.push_back(basepart);
}

int OpenRender::Loop()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    Shader rayTrac("shader/RayTrace1.vs", "shader/deffered.fs");
    while (!glfwWindowShouldClose(window)) {
        if (movingMouse || pressingKey)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, lastColorBuffer);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            movingMouse = false;
            pressingKey = false;
        }
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // Check and call events
        processInput(window);
        this->Draw(renderQueue[0], rayTrac);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
int init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
}
bool keys[1024];
bool keysPressed[1024];
void processInput(GLFWwindow* window)
{
    // Camera controls
    if (keys[GLFW_KEY_W])
    {
        pressingKey = true;
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }   
    if (keys[GLFW_KEY_S])
    {
        pressingKey = true;
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (keys[GLFW_KEY_A])
    {
        pressingKey = true;
        camera.ProcessKeyboard(LEFT, deltaTime);
    } 
    if (keys[GLFW_KEY_D])
    {
        pressingKey = true;
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    if (keys[GLFW_KEY_SPACE] && !keysPressed[GLFW_KEY_SPACE])
    {
        //pressingKey = true;
        //hdr = !hdr;
        //keysPressed[GLFW_KEY_SPACE] = true;
        snap_shot(800, 900);
    }

    // Change parallax height scale
    if (keys[GLFW_KEY_Q])
        exposure -= 0.5 * deltaTime;
    else if (keys[GLFW_KEY_E])
        exposure += 0.5 * deltaTime;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        //pressingKey = true;
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    
    if (key >= 0 && key <= 1024)
    {
        if (action == GLFW_PRESS)
        {
            //pressingKey = true;
            keys[key] = true;
        }
            
        else if (action == GLFW_RELEASE)
        {
            //pressingKey = true;
            keys[key] = false;
            keysPressed[key] = false;
        }
    }
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    movingMouse = true;

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    movingMouse = true;
    camera.ProcessMouseScroll(yoffset);
}
