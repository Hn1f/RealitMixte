/**
 * @file geometries.cpp
 * @brief Géométries OpenGL utilitaires : quad plein écran, cube filaire, cube solide, maze, sphere.
 */

 #include <random>
 #include <stack>
 #include <utility>
 #include <vector>
 #include <cstdint>
 #include <algorithm>
 #include <cmath>
 
 #include "geometries.hpp"
 
 // IMPORTANT : on inclut ball.h ici pour connaître la structure Maze
 // (Maze est défini chez toi dans ce header)
 #include "../Ball.hpp"
 
 // ------------------------------------------------------------
 // Background quad
 // ------------------------------------------------------------
 Mesh createBackgroundQuad(){
     const float data[] = {
         // x  y   u  v
         -1,-1,  0,1,
          1,-1,  1,1,
          1, 1,  1,0,
         -1,-1,  0,1,
          1, 1,  1,0,
         -1, 1,  0,0
     };
     Mesh m; m.count = 6;
     glGenVertexArrays(1,&m.vao);
     glGenBuffers(1,&m.vbo);
     glBindVertexArray(m.vao);
     glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
     glBufferData(GL_ARRAY_BUFFER,sizeof(data),data,GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0); // aPos (x,y)
     glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
 
     glEnableVertexAttribArray(1); // aUV (u,v)
     glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
 
     glBindVertexArray(0);
     return m;
 }
 
 // ------------------------------------------------------------
 // Cube wireframe
 // ------------------------------------------------------------
 Mesh createCubeWireframeUnit(float size) {
     const float s = size;
     const float V[] = {
         -s,-s,-s,  +s,-s,-s,  +s,+s,-s,  -s,+s,-s, // 0..3 z-
         -s,-s,+s,  +s,-s,+s,  +s,+s,+s,  -s,+s,+s  // 4..7 z+
     };
 
     const GLuint E[] = {
         0,1, 1,2, 2,3, 3,0,    // base z-
         4,5, 5,6, 6,7, 7,4,    // base z+
         0,4, 1,5, 2,6, 3,7     // verticales
     };
 
     Mesh m; m.count = static_cast<GLsizei>(sizeof(E)/sizeof(E[0]));
     glGenVertexArrays(1, &m.vao);
     glGenBuffers(1, &m.vbo);
     glGenBuffers(1, &m.ebo);
 
     glBindVertexArray(m.vao);
     glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
     glBufferData(GL_ARRAY_BUFFER, sizeof(V), V, GL_STATIC_DRAW);
 
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(E), E, GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0); // aPos
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
 
     glBindVertexArray(0);
     return m;
 }
 
 // ------------------------------------------------------------
 // Cube solid
 // ------------------------------------------------------------
 Mesh createCubeSolidUnit(){
     const float V[] = {
         -0.5f,-0.5f,-0.5f,  +0.5f,-0.5f,-0.5f,  +0.5f,+0.5f,-0.5f,  -0.5f,+0.5f,-0.5f,
         -0.5f,-0.5f,+0.5f,  +0.5f,-0.5f,+0.5f,  +0.5f,+0.5f,+0.5f,  -0.5f,+0.5f,+0.5f
     };
     const GLuint I[] = {
         0,1,2,  0,2,3,      // face -Z
         4,6,5,  4,7,6,      // face +Z
         0,3,7,  0,7,4,      // face -X
         1,5,6,  1,6,2,      // face +X
         0,4,5,  0,5,1,      // face -Y
         3,2,6,  3,6,7       // face +Y
     };
     Mesh m; m.count = (GLsizei)(sizeof(I)/sizeof(I[0]));
     glGenVertexArrays(1,&m.vao);
     glGenBuffers(1,&m.vbo);
     glGenBuffers(1,&m.ebo);
 
     glBindVertexArray(m.vao);
     glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
     glBufferData(GL_ARRAY_BUFFER,sizeof(V),V,GL_STATIC_DRAW);
 
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(I),I,GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0); // aPos
     glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
 
     glBindVertexArray(0);
     return m;
 }
 
 // ------------------------------------------------------------
 // Destroy mesh
 // ------------------------------------------------------------
 void destroyMesh(Mesh& m){
     if (m.vao) glDeleteVertexArrays(1,&m.vao);
     if (m.vbo) glDeleteBuffers(1,&m.vbo);
     if (m.ebo) glDeleteBuffers(1,&m.ebo);
     m = {};
 }
 
 // ------------------------------------------------------------
 // Helpers for maze
 // ------------------------------------------------------------
 struct Box { float x0,y0,z0, x1,y1,z1; };
 
 static void appendBoxWire(const Box& b,
                           std::vector<float>& V,
                           std::vector<uint32_t>& I)
 {
     uint32_t base = (uint32_t)(V.size()/3);
 
     const float x0=b.x0, y0=b.y0, z0=b.z0;
     const float x1=b.x1, y1=b.y1, z1=b.z1;
 
     const float verts[8][3] = {
         {x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0},
         {x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1}
     };
     for(int k=0;k<8;k++){
         V.push_back(verts[k][0]);
         V.push_back(verts[k][1]);
         V.push_back(verts[k][2]);
     }
 
     auto e = [&](uint32_t a, uint32_t c){ I.push_back(base+a); I.push_back(base+c); };
 
     e(0,1); e(1,2); e(2,3); e(3,0);
     e(4,5); e(5,6); e(6,7); e(7,4);
     e(0,4); e(1,5); e(2,6); e(3,7);
 }
 
 Mesh createMazeLines(float mazeW, float mazeH, float wallT, float wallH)
 {
     std::vector<Box> walls;
     float z0 = 0.0f, z1 = wallH;
 
     // Bordure extérieure
     walls.push_back({0,0,z0, mazeW, wallT, z1});
     walls.push_back({0,mazeH-wallT,z0, mazeW, mazeH, z1});
     walls.push_back({0,0,z0, wallT, mazeH, z1});
     walls.push_back({mazeW-wallT,0,z0, mazeW, mazeH, z1});
 
     // Exemple : murs internes FIXES (si tu utilises cette version)
     walls.push_back({0.10f*mazeW, 0.15f*mazeH, z0, 0.90f*mazeW, 0.15f*mazeH + wallT, z1});
     walls.push_back({0.10f*mazeW, 0.35f*mazeH, z0, 0.70f*mazeW, 0.35f*mazeH + wallT, z1});
     walls.push_back({0.30f*mazeW, 0.55f*mazeH, z0, 0.95f*mazeW, 0.55f*mazeH + wallT, z1});
 
     walls.push_back({0.20f*mazeW, 0.15f*mazeH, z0, 0.20f*mazeW + wallT, 0.60f*mazeH, z1});
     walls.push_back({0.45f*mazeW, 0.25f*mazeH, z0, 0.45f*mazeW + wallT, 0.85f*mazeH, z1});
     walls.push_back({0.70f*mazeW, 0.35f*mazeH, z0, 0.70f*mazeW + wallT, 0.95f*mazeH, z1});
 
     std::vector<float> V;
     std::vector<uint32_t> I;
     V.reserve(walls.size()*8*3);
     I.reserve(walls.size()*24);
 
     for (auto& b : walls) appendBoxWire(b, V, I);
 
     Mesh m{};
     m.count = (GLsizei)I.size();
 
     glGenVertexArrays(1, &m.vao);
     glGenBuffers(1, &m.vbo);
     glGenBuffers(1, &m.ebo);
 
     glBindVertexArray(m.vao);
 
     glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
     glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof(float), V.data(), GL_STATIC_DRAW);
 
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof(uint32_t), I.data(), GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0);
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
 
     glBindVertexArray(0);
     return m;
 }
 
 // ---- Solid box helper ----
 static void appendBoxSolid(float x0,float y0,float z0, float x1,float y1,float z1,
                            std::vector<float>& V, std::vector<uint32_t>& I)
 {
     uint32_t base = (uint32_t)(V.size()/3);
     const float v[8][3]={
         {x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0},
         {x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1}
     };
     for(int k=0;k<8;k++){ V.push_back(v[k][0]); V.push_back(v[k][1]); V.push_back(v[k][2]); }
 
     auto tri=[&](uint32_t a,uint32_t b,uint32_t c){
         I.push_back(base+a); I.push_back(base+b); I.push_back(base+c);
     };
 
     // faces (12 triangles)
     tri(0,1,2); tri(0,2,3); // bottom
     tri(4,6,5); tri(4,7,6); // top
     tri(0,3,7); tri(0,7,4); // -X
     tri(1,5,6); tri(1,6,2); // +X
     tri(0,4,5); tri(0,5,1); // -Y
     tri(3,2,6); tri(3,6,7); // +Y
 }
 
 // ------------------------------------------------------------
 // Ton createMazeWallsSolid existant (perfect maze internal)
 // (je le laisse tel quel, fonctionnel)
 // ------------------------------------------------------------
 struct Cell { bool v=false; bool wN=true,wE=true,wS=true,wW=true; }; // murs présents
 
 static void generatePerfectMaze(int cx,int cy, std::vector<Cell>& grid, uint32_t seed=1337)
 {
     auto idx=[&](int x,int y){ return y*cx+x; };
     std::mt19937 rng(seed);
 
     std::stack<std::pair<int,int>> st;
     st.push({0,0});
     grid[idx(0,0)].v=true;
 
     while(!st.empty()){
         auto [x,y]=st.top();
 
         struct N{int nx,ny; int dir;};
         std::vector<N> neigh;
         if(y>0     && !grid[idx(x,y-1)].v) neigh.push_back({x,y-1,0}); // N
         if(x<cx-1  && !grid[idx(x+1,y)].v) neigh.push_back({x+1,y,1}); // E
         if(y<cy-1  && !grid[idx(x,y+1)].v) neigh.push_back({x,y+1,2}); // S
         if(x>0     && !grid[idx(x-1,y)].v) neigh.push_back({x-1,y,3}); // W
 
         if(neigh.empty()){ st.pop(); continue; }
 
         std::uniform_int_distribution<int> dist(0,(int)neigh.size()-1);
         auto n = neigh[dist(rng)];
 
         Cell& c = grid[idx(x,y)];
         Cell& d = grid[idx(n.nx,n.ny)];
         if(n.dir==0){ c.wN=false; d.wS=false; }
         if(n.dir==1){ c.wE=false; d.wW=false; }
         if(n.dir==2){ c.wS=false; d.wN=false; }
         if(n.dir==3){ c.wW=false; d.wE=false; }
 
         d.v=true;
         st.push({n.nx,n.ny});
     }
 }
 
 Mesh createMazeWallsSolid(float mazeW,float mazeH,int cellsX,int cellsY,
                           float corridorW,float wallT,float wallH)
 {
     const float pitchX = mazeW / cellsX;
     const float pitchY = mazeH / cellsY;
 
     corridorW = std::min(corridorW, std::min(pitchX, pitchY) - wallT*2.0f);
 
     std::vector<Cell> grid(cellsX*cellsY);
     generatePerfectMaze(cellsX, cellsY, grid, 42);
 
     std::vector<float> V;
     std::vector<uint32_t> I;
     V.reserve(20000);
     I.reserve(20000);
 
     auto idx=[&](int x,int y){ return y*cellsX+x; };
 
     const float z0=0.0f, z1=wallH;
 
     // Bordure extérieure
     appendBoxSolid(0,0,z0, mazeW, wallT, z1, V,I);
     appendBoxSolid(0,mazeH-wallT,z0, mazeW, mazeH, z1, V,I);
     appendBoxSolid(0,0,z0, wallT, mazeH, z1, V,I);
     appendBoxSolid(mazeW-wallT,0,z0, mazeW, mazeH, z1, V,I);
 
     for(int y=0;y<cellsY;y++){
         for(int x=0;x<cellsX;x++){
             const float x0 = x*pitchX;
             const float y0 = y*pitchY;
             const float x1 = x0 + pitchX;
             const float y1 = y0 + pitchY;
 
             Cell& c = grid[idx(x,y)];
 
             if(c.wN){
                 appendBoxSolid(x0, y0, z0, x1, y0+wallT, z1, V,I);
             }
             if(c.wW){
                 appendBoxSolid(x0, y0, z0, x0+wallT, y1, z1, V,I);
             }
             if(x==cellsX-1 && c.wE){
                 appendBoxSolid(x1-wallT, y0, z0, x1, y1, z1, V,I);
             }
             if(y==cellsY-1 && c.wS){
                 appendBoxSolid(x0, y1-wallT, z0, x1, y1, z1, V,I);
             }
         }
     }
 
     Mesh m{};
     m.count = (GLsizei)I.size();
 
     glGenVertexArrays(1,&m.vao);
     glGenBuffers(1,&m.vbo);
     glGenBuffers(1,&m.ebo);
 
     glBindVertexArray(m.vao);
 
     glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
     glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof(float), V.data(), GL_STATIC_DRAW);
 
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof(uint32_t), I.data(), GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0);
     glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
 
     glBindVertexArray(0);
     return m;
 }
 
 // déclaration laissée, non utilisée ici (tu l’avais dans ton header)
 Mesh createMazeWallsSolid(float sheetW, float sheetH,
     int cellsX, int cellsY,
     float corridorW, float wallT, float wallH,
     std::vector<Wall2D>& outWalls)
 {
     // Version “outWalls” non fournie dans ton code source initial.
     // Pour rester fidèle à ta structure, on renvoie juste la version sans outWalls
     // et on vide outWalls.
     outWalls.clear();
     return createMazeWallsSolid(sheetW, sheetH, cellsX, cellsY, corridorW, wallT, wallH);
 }
 
 // ------------------------------------------------------------
 // NEW : maze mesh SOLIDE depuis TON Maze (rendu == collisions)
 // ------------------------------------------------------------
 Mesh createMazeWallsSolidFromMaze(const Maze& maze, float wallH)
 {
     std::vector<float> V;
     std::vector<uint32_t> I;
     V.reserve(20000);
     I.reserve(20000);
 
     const float z0 = 0.0f;
     const float z1 = wallH;
 
     const float wallT = maze.wallThick;
 
     const float mazeW = maze.w * maze.cellW;
     const float mazeH = maze.h * maze.cellH;
 
     // Bordure extérieure
     appendBoxSolid(0, 0, z0,           mazeW, wallT, z1,           V, I); // N
     appendBoxSolid(0, mazeH-wallT, z0, mazeW, mazeH, z1,           V, I); // S
     appendBoxSolid(0, 0, z0,           wallT, mazeH, z1,           V, I); // W
     appendBoxSolid(mazeW-wallT, 0, z0, mazeW, mazeH, z1,           V, I); // E
 
     // Murs internes : N & W (pas de doublons), + E dernier col, + S dernière ligne
     for (int gy = 0; gy < maze.h; ++gy) {
         for (int gx = 0; gx < maze.w; ++gx) {
             const auto& c = maze.at(gx, gy);
 
             const float x0 = gx * maze.cellW;
             const float y0 = gy * maze.cellH;
             const float x1 = x0 + maze.cellW;
             const float y1 = y0 + maze.cellH;
 
             if (c.wN) {
                 appendBoxSolid(x0, y0, z0, x1, y0 + wallT, z1, V, I);
             }
             if (c.wW) {
                 appendBoxSolid(x0, y0, z0, x0 + wallT, y1, z1, V, I);
             }
 
             if (gx == maze.w - 1 && c.wE) {
                 appendBoxSolid(x1 - wallT, y0, z0, x1, y1, z1, V, I);
             }
             if (gy == maze.h - 1 && c.wS) {
                 appendBoxSolid(x0, y1 - wallT, z0, x1, y1, z1, V, I);
             }
         }
     }
 
     Mesh m{};
     m.count = (GLsizei)I.size();
 
     glGenVertexArrays(1,&m.vao);
     glGenBuffers(1,&m.vbo);
     glGenBuffers(1,&m.ebo);
 
     glBindVertexArray(m.vao);
 
     glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
     glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof(float), V.data(), GL_STATIC_DRAW);
 
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof(uint32_t), I.data(), GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0);
     glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
 
     glBindVertexArray(0);
     return m;
 }
 
 // ------------------------------------------------------------
 // Sphere (positions only) : utilisé par Ball (createSphere)
 // ------------------------------------------------------------
 Mesh createSphere(float radius, int stacks, int slices)
 {
     stacks = std::max(2, stacks);
     slices = std::max(3, slices);
 
     std::vector<float> V;
     std::vector<uint32_t> I;
     V.reserve((stacks+1)*(slices+1)*3);
     I.reserve(stacks*slices*6);
 
     for (int i = 0; i <= stacks; ++i) {
         float v = (float)i / (float)stacks;
         float phi = v * (float)M_PI;         // 0..pi
 
         float y = std::cos(phi);
         float r = std::sin(phi);
 
         for (int j = 0; j <= slices; ++j) {
             float u = (float)j / (float)slices;
             float theta = u * 2.0f * (float)M_PI; // 0..2pi
 
             float x = r * std::cos(theta);
             float z = r * std::sin(theta);
 
             V.push_back(radius * x);
             V.push_back(radius * y);
             V.push_back(radius * z);
         }
     }
 
     auto idx = [&](int i, int j)->uint32_t {
         return (uint32_t)(i*(slices+1) + j);
     };
 
     for (int i = 0; i < stacks; ++i) {
         for (int j = 0; j < slices; ++j) {
             uint32_t a = idx(i, j);
             uint32_t b = idx(i+1, j);
             uint32_t c = idx(i+1, j+1);
             uint32_t d = idx(i, j+1);
 
             I.push_back(a); I.push_back(b); I.push_back(c);
             I.push_back(a); I.push_back(c); I.push_back(d);
         }
     }
 
     Mesh m{};
     m.count = (GLsizei)I.size();
 
     glGenVertexArrays(1,&m.vao);
     glGenBuffers(1,&m.vbo);
     glGenBuffers(1,&m.ebo);
 
     glBindVertexArray(m.vao);
 
     glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
     glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof(float), V.data(), GL_STATIC_DRAW);
 
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof(uint32_t), I.data(), GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0);
     glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
 
     glBindVertexArray(0);
     return m;
 }
 