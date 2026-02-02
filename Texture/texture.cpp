/**
 * @file texture.cpp
 * @brief Implémentation : création/mise à jour d'une texture OpenGL depuis un cv::Mat.
 */

 #include "texture.hpp"

 /**
  * @brief Crée ou met à jour une texture 2D OpenGL depuis une image OpenCV.
  * @see createOrUpdateTexture(GLuint, const cv::Mat&)
  */
 GLuint createOrUpdateTexture(GLuint tex, const cv::Mat& imgBGR){
     // --- Conversion espace couleur (CPU) vers un layout compatible OpenGL ---
     cv::Mat rgb;
     if (imgBGR.channels()==3) {
         // BGR -> RGB
         cv::cvtColor(imgBGR, rgb, cv::COLOR_BGR2RGB);
     } else if (imgBGR.channels()==4) {
         // BGRA -> RGBA
         cv::cvtColor(imgBGR, rgb, cv::COLOR_BGRA2RGBA);
     } else {
         // GRAY -> RGB (réplication)
         cv::cvtColor(imgBGR, rgb, cv::COLOR_GRAY2RGB);
     }
 
     // --- Allocation de la texture si nécessaire ---
     if (tex==0) glGenTextures(1, &tex);
 
     glBindTexture(GL_TEXTURE_2D, tex);
     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
 
     // Upload des pixels
     // Variante si tu veux supporter RGBA en interne :
     //   GLint internalFmt = (rgb.channels()==4) ? GL_RGBA8 : GL_RGB8;
     //   GLenum srcFormat  = (rgb.channels()==4) ? GL_RGBA  : GL_RGB;
     //   glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, rgb.cols, rgb.rows, 0, srcFormat, GL_UNSIGNED_BYTE, rgb.data);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, rgb.cols, rgb.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);
 
     // Paramétrage filtres & wrap
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 
     glBindTexture(GL_TEXTURE_2D, 0);
     return tex;
 }

 static cv::Mat flipIfNeeded(const cv::Mat& img, bool flipY){
    if(!flipY) return img;
    cv::Mat out; cv::flip(img, out, 0);
    return out;
}

GLuint loadTextureFromFile(const std::string& path, bool flipY)
{
    cv::Mat bgr = cv::imread(path, cv::IMREAD_COLOR);
    if (bgr.empty()) {
        std::cerr << "loadTextureFromFile: impossible de lire " << path << "\n";
        return 0;
    }

    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    rgb = flipIfNeeded(rgb, flipY);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 rgb.cols, rgb.rows,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 rgb.data);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
 