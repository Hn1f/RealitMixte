// main.cpp (AR Charuco + labyrinthe SOLIDE calé sur FEUILLE A4 + fond JPG + BALL)
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <algorithm>

#include "Shaders/shaders.hpp"
#include "GLUtils/gl_utils.hpp"
#include "ARMatrices/ar_matrices.hpp"
#include "Geometries/geometries.hpp"
#include "Texture/texture.hpp"
#include "SceneObjects.hpp"
#include "Smoothing/smoothing.hpp"

#include "Ball.hpp"

struct Axes { Mesh x, y, z; };

static Axes createAxes(float L) {
    Axes A{};
    auto makeLine = [](float x1, float y1, float z1,
                       float x2, float y2, float z2) {
        Mesh m;
        m.count = 2;
        const float V[6] = { x1, y1, z1,  x2, y2, z2 };
        glGenVertexArrays(1, &m.vao);
        glGenBuffers(1, &m.vbo);
        glBindVertexArray(m.vao);
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(V), V, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
        return m;
    };

    A.x = makeLine(0.f, 0.f, 0.f,  L,  0.f, 0.f);
    A.y = makeLine(0.f, 0.f, 0.f,  0.f,  L,  0.f);
    A.z = makeLine(0.f, 0.f, 0.f,  0.f, 0.f, -L);
    return A;
}

static bool loadCalibration(const std::string& path, cv::Mat& K, cv::Mat& D, cv::Size& calibSz) {
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;

    fs["camera_matrix"] >> K;
    fs["distortion_coefficients"] >> D;
    if (K.empty() || D.empty()) return false;

    if (D.type() != CV_64F) D.convertTo(D, CV_64F);
    D = D.reshape(1, 1);

    int w = 0, h = 0;
    if (!fs["image_width"].empty() && !fs["image_height"].empty()) {
        w = (int)fs["image_width"];
        h = (int)fs["image_height"];
    } else {
        w = (int)std::round(K.at<double>(0,2) * 2.0);
        h = (int)std::round(K.at<double>(1,2) * 2.0);
    }
    calibSz = cv::Size(w, h);
    return true;
}

int main() {
    const std::string droidcamUrl = "http://192.168.1.158:4747/video";

    SceneObjects scene;

    // ----------- 1) Capture -----------
    cv::VideoCapture video(droidcamUrl);
    if (!video.isOpened()) {
        std::cerr << "Impossible d'ouvrir DroidCam: " << droidcamUrl << "\n";
        return -1;
    }

    cv::Mat frame;
    if (!video.read(frame) || frame.empty()) {
        std::cerr << "Première frame vide.\n";
        return -1;
    }

    // ----------- 2) Calibration -----------
    cv::Mat K, D;
    cv::Size calibSz;
    if (!loadCalibration("camera.yaml", K, D, calibSz)) {
        std::cerr << "camera.yaml introuvable ou invalide.\n";
        return -1;
    }

    // Si taille différente, adapte les intrinsèques
    if (frame.size() != calibSz) {
        const double sx = (double)frame.cols / (double)calibSz.width;
        const double sy = (double)frame.rows / (double)calibSz.height;
        K.at<double>(0,0) *= sx; // fx
        K.at<double>(1,1) *= sy; // fy
        K.at<double>(0,2) *= sx; // cx
        K.at<double>(1,2) *= sy; // cy
    }

    // ----------- 3) Charuco board -----------
    const int squaresX = 5;
    const int squaresY = 7;

    const float squareLength = 0.026f; // m
    const float markerLength = 0.019f; // m

    auto dict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
    cv::Ptr<cv::aruco::CharucoBoard> board =
        cv::aruco::CharucoBoard::create(squaresX, squaresY, squareLength, markerLength, dict);

    auto params = cv::aruco::DetectorParameters::create();
    params->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
    params->cornerRefinementWinSize = 5;
    params->cornerRefinementMaxIterations = 30;
    params->cornerRefinementMinAccuracy = 0.01;

    // ----------- 4) OpenGL init -----------
    if (!glfwInit()) { std::cerr << "glfwInit failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(frame.cols, frame.rows, "AR Charuco + Maze + Ball", nullptr, nullptr);
    if (!win) { std::cerr << "glfwCreateWindow failed\n"; return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "glewInit failed\n"; return -1; }
    glGetError();

    // ----------- Shaders -----------
    GLuint progBG   = linkProgram({ compileShader(GL_VERTEX_SHADER, BG_VS),
                                    compileShader(GL_FRAGMENT_SHADER, BG_FS) });

    GLuint progLine = linkProgram({ compileShader(GL_VERTEX_SHADER,   LINE_VS),
                                    compileShader(GL_GEOMETRY_SHADER, LINE_GS),
                                    compileShader(GL_FRAGMENT_SHADER, LINE_FS) });

    GLuint progFace = linkProgram({ compileShader(GL_VERTEX_SHADER, FACE_VS),
                                    compileShader(GL_FRAGMENT_SHADER, FACE_FS) });

    GLint uBG_tex        = glGetUniformLocation(progBG,   "uTex");
    GLint uLine_MVP      = glGetUniformLocation(progLine, "uMVP");
    GLint uLine_Color    = glGetUniformLocation(progLine, "uColor");
    GLint uLine_ThickPx  = glGetUniformLocation(progLine, "uThicknessPx");
    GLint uLine_Viewport = glGetUniformLocation(progLine, "uViewport");

    GLint uFace_MVP   = glGetUniformLocation(progFace, "uMVP");
    GLint uFace_Color = glGetUniformLocation(progFace, "uFaceColor");

    Mesh bg = createBackgroundQuad();

    // ----------- Texture background -----------
    GLuint texBG = loadTextureFromFile("./assets/background.jpg", true);
    if (!texBG) {
        std::cerr << "Fond JPG introuvable. Vérifie ./assets/background.jpg\n";
        return -1;
    }

    // ----------- A4 sheet dims (m) -----------
    const float sheetW = 0.297f;
    const float sheetH = 0.210f;

    // ----------- Ball & walls params -----------
    const float ballR = 0.010f;  // 1cm
    const float wallT = 0.0035f; // 3.5mm
    const float wallH = 0.040f;  // 4cm

    int cellsX = 8;
    int cellsY = 6;

    // ✅ TON Maze (sert collisions)
    Maze maze(cellsX, cellsY, sheetW, sheetH, wallT);
    maze.generate();

    // ✅ Mesh murs basé SUR LE MEME Maze
    Mesh mazeSolid = createMazeWallsSolidFromMaze(maze, wallH);

    // ✅ TON Ball
    Ball ball(ballR);
    ball.reset(maze);

    scene.addOBJ("./assets/obj/SM/Meshy_AI_SM_0115202256_texture.obj",
        glm::vec3(-0.06f, sheetH*0.5f, 0.0f),
        glm::vec3(-90.f, 0.f, 0.f),      // ✅ redresse : rotation -90° X
        glm::vec3(0.10f, 0.10f, 0.10f), 
        glm::vec4(0.7f,0.7f,0.7f,1.0f));
    


    // debug axes
    Axes axes = createAxes(0.10f);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f,0.05f,0.06f,1.0f);

    PoseSmoother poseSmooth;
    poseSmooth.alphaPose = 0.25;

    cv::Mat rvec, tvec;
    bool hasPose = false;

    const float lineThicknessPx = 3.0f;

    // ---------------------- CALAGE FEUILLE vs BOARD ----------------------
    const float marginLeft   = 0.080f;
    const float marginBottom = 0.010f;

    double lastT = glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        // dt
        double nowT = glfwGetTime();
        float dt = (float)(nowT - lastT);
        lastT = nowT;
        dt = std::min(dt, 1.0f/20.0f);
        dt = std::max(dt, 1.0f/500.0f);

        // ----------- Read frame -----------
        if (!video.read(frame) || frame.empty()) break;

        // ----------- Detect Charuco -----------
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> markerCorners;
        cv::aruco::detectMarkers(gray, dict, markerCorners, markerIds, params);

        bool poseOk = false;

        if (!markerIds.empty()) {
            cv::Mat charucoCorners, charucoIds;
            cv::aruco::interpolateCornersCharuco(markerCorners, markerIds, gray, board,
                                                 charucoCorners, charucoIds, K, D);

            if (charucoIds.total() >= 6) {
                poseOk = cv::aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds,
                                                             board, K, D, rvec, tvec);
            }
        }

        if (poseOk) {
            if (rvec.type() != CV_64F) rvec.convertTo(rvec, CV_64F);
            if (tvec.type() != CV_64F) tvec.convertTo(tvec, CV_64F);
            
            poseSmooth.smooth(rvec, tvec);
            hasPose = true;
        }
        if (poseOk && !ball.hasFlatRef) {
            ball.setFlatReference(rvec);
        }
        

        // ----------- Render background JPG -----------
        int fbw, fbh;
        glfwGetFramebufferSize(win, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        glUseProgram(progBG);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texBG);
        glUniform1i(uBG_tex, 0);

        glBindVertexArray(bg.vao);
        glDrawArrays(GL_TRIANGLES, 0, bg.count);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        // ----------- Draw 3D (maze + ball + debug axes) -----------
// --- Draw 3D (maze + ball + debug axes) ---
if (hasPose) {
    glm::mat4 P = projectionFromCV(K, (float)fbw, (float)fbh, 0.01f, 2000.0f);
    glm::mat4 M_board = modelFromRvecTvec_OpenCVtoGL(rvec, tvec);

    const float zLift = 0.005f;

    // ✅ Rotation VISUELLE du labyrinthe (et donc de la balle car elle est dessinée avec MVP_maze)
    // ✅ Physique/mouvements : AUCUN changement (ball.update reste identique)
    glm::mat4 modelMaze =
        glm::translate(glm::mat4(1.0f), glm::vec3(-marginLeft, -marginBottom, -zLift))
      * glm::translate(glm::mat4(1.0f), glm::vec3(sheetW * 0.5f, sheetH * 0.5f, 0.0f)) // pivot centre feuille
      * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1))            // rotation Z
      * glm::translate(glm::mat4(1.0f), glm::vec3(-sheetW * 0.5f, -sheetH * 0.5f, 0.0f));

    glm::mat4 MVP_maze = P * M_board * modelMaze;

    scene.drawAll(progFace, uFace_MVP, uFace_Color, MVP_maze);
    
    if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) {
        ball.setFlatReference(rvec);
        ball.vel = glm::vec2(0,0);
    }

    // ✅ update ball : NE CHANGE PAS
    ball.update(dt, rvec, maze);

    // --- Murs ---
    glUseProgram(progFace);
    glUniformMatrix4fv(uFace_MVP, 1, GL_FALSE, glm::value_ptr(MVP_maze));
    glUniform4f(uFace_Color, 0.85f, 0.85f, 0.85f, 1.0f);
    glBindVertexArray(mazeSolid.vao);
    glDrawElements(GL_TRIANGLES, mazeSolid.count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // --- Balle (dessinée avec MVP_maze donc elle tourne visuellement avec le laby) ---
    glUseProgram(progFace);
    glUniform4f(uFace_Color, 0.2f, 0.9f, 0.2f, 1.0f);
    ball.draw(progFace, uFace_MVP, MVP_maze);

    // --- Axes debug : NE TOUCHE PAS ---
    glUseProgram(progLine);
    glUniform2f(uLine_Viewport, (float)fbw, (float)fbh);
    glUniform1f(uLine_ThickPx, lineThicknessPx);

    glm::mat4 MVP_axes = P * M_board; // ✅ inchangé
    glUniformMatrix4fv(uLine_MVP, 1, GL_FALSE, glm::value_ptr(MVP_axes));

    glBindVertexArray(axes.x.vao); glUniform3f(uLine_Color, 1.f, 0.f, 0.f); glDrawArrays(GL_LINES, 0, axes.x.count);
    glBindVertexArray(axes.y.vao); glUniform3f(uLine_Color, 0.f, 1.f, 0.f); glDrawArrays(GL_LINES, 0, axes.y.count);
    glBindVertexArray(axes.z.vao); glUniform3f(uLine_Color, 0.f, 0.f, 1.f); glDrawArrays(GL_LINES, 0, axes.z.count);
    glBindVertexArray(0);
}

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteProgram(progBG);
    glDeleteProgram(progLine);
    glDeleteProgram(progFace);

    if (texBG) glDeleteTextures(1, &texBG);

    destroyMesh(bg);
    destroyMesh(mazeSolid);
    destroyMesh(ball.mesh);

    if (axes.x.vao) { glDeleteVertexArrays(1, &axes.x.vao); glDeleteBuffers(1, &axes.x.vbo); }
    if (axes.y.vao) { glDeleteVertexArrays(1, &axes.y.vao); glDeleteBuffers(1, &axes.y.vbo); }
    if (axes.z.vao) { glDeleteVertexArrays(1, &axes.z.vao); glDeleteBuffers(1, &axes.z.vbo); }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
