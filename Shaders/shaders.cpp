/**
 * @file shaders.cpp
 * @brief Implémentation des sources GLSL embarquées (fond, lignes, faces).
 */

 #include "shaders.hpp"

 /**
  * @brief Vertex shader pour le fond vidéo texturé (quad NDC).
  * @inputs
  *  - location=0 : vec2 aPos   (NDC)
  *  - location=1 : vec2 aUV    (coordonnées texture)
  * @outputs
  *  - vUV : coordonnées UV vers le fragment shader
  */
 const char* BG_VS = R"(#version 330 core
 layout (location=0) in vec2 aPos;
 layout (location=1) in vec2 aUV;
 out vec2 vUV;
 void main(){ vUV=aUV; gl_Position=vec4(aPos,0.0,1.0); }
 )";
 
 /**
  * @brief Fragment shader pour le fond vidéo : échantillonne `uTex`.
  * @uniforms
  *  - sampler2D uTex : texture de la frame vidéo
  */
 const char* BG_FS = R"(#version 330 core
 in vec2 vUV; out vec4 FragColor;
 uniform sampler2D uTex;
 void main(){ FragColor = texture(uTex, vUV); }
 )";
 
 /**
  * @brief Vertex shader des lignes : projection via `uMVP`.
  * @uniforms
  *  - mat4 uMVP
  * @inputs
  *  - location=0 : vec3 aPos
  */
 const char* LINE_VS = R"(#version 330 core
 layout (location=0) in vec3 aPos;
 uniform mat4 uMVP;
 void main(){ gl_Position = uMVP*vec4(aPos,1.0); }
 )";
 
 /**
  * @brief Geometry shader : transforme un segment en quad épais (en pixels).
  * @uniforms
  *  - float uThicknessPx : épaisseur voulue en pixels
  *  - vec2  uViewport    : taille viewport (width,height)
  * @notes
  *  - Convertit la direction du segment en NDC, calcule une normale
  *    écran, puis émet 4 sommets `triangle_strip`.
  *  - Préserve la profondeur (z/w) de chaque extrémité.
  */
 const char* LINE_GS = R"(#version 330 core
 layout(lines) in; layout(triangle_strip, max_vertices=4) out;
 uniform float uThicknessPx;
 uniform vec2  uViewport;
 void main(){
   vec4 p0=gl_in[0].gl_Position, p1=gl_in[1].gl_Position;
   vec2 ndc0=p0.xy/p0.w, ndc1=p1.xy/p1.w;
   vec2 dir=ndc1-ndc0; float len=length(dir);
   vec2 n=(len>1e-6)? normalize(vec2(-dir.y,dir.x)) : vec2(0.0,1.0);
   vec2 px2ndc = 2.0/uViewport;
   vec2 off = n*uThicknessPx*px2ndc;
   float z0=p0.z/p0.w, z1=p1.z/p1.w;
   vec4 v0=vec4(ndc0-off, z0,1), v1=vec4(ndc0+off, z0,1);
   vec4 v2=vec4(ndc1-off, z1,1), v3=vec4(ndc1+off, z1,1);
   gl_Position=v0; EmitVertex();
   gl_Position=v1; EmitVertex();
   gl_Position=v2; EmitVertex();
   gl_Position=v3; EmitVertex();
   EndPrimitive();
 }
 )";
 
 /**
  * @brief Fragment shader des lignes : couleur uniforme `uColor`.
  * @uniforms
  *  - vec3 uColor
  */
 const char* LINE_FS = R"(#version 330 core
 out vec4 FragColor; uniform vec3 uColor;
 void main(){ FragColor = vec4(uColor,1.0); }
 )";
 
 /**
  * @brief Vertex shader des faces pleines (uMVP * aPos).
  * @uniforms
  *  - mat4 uMVP
  * @inputs
  *  - location=0 : vec3 aPos
  */
 const char* FACE_VS = R"(#version 330 core
 layout (location=0) in vec3 aPos;
 uniform mat4 uMVP;
 void main(){ gl_Position = uMVP*vec4(aPos,1.0); }
 )";
 
 /**
  * @brief Fragment shader des faces pleines : couleur uniforme `uFaceColor`.
  * @uniforms
  *  - vec4 uFaceColor
  */
 const char* FACE_FS = R"(#version 330 core
 out vec4 FragColor; uniform vec4 uFaceColor;
 void main(){ FragColor = uFaceColor; }
 )";
 