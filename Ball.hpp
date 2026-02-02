#pragma once
#include <vector>
#include <cmath>
#include <stack>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>

// On inclut geometries pour connaitre "Mesh"
#include "Geometries/geometries.hpp" 

// --- CLASSE MAZE ---
class Maze {
public:
    int w, h;
    float cellW, cellH;
    float wallThick;
    
    struct Cell {
        bool wN=true, wS=true, wE=true, wW=true; 
        bool visited=false;
    };
    std::vector<Cell> grid;

    Maze(int width, int height, float sheetWidth, float sheetHeight, float wallThickness) 
        : w(width), h(height), wallThick(wallThickness) {
        grid.resize(w * h);
        cellW = sheetWidth / (float)w;
        cellH = sheetHeight / (float)h;
    }

    Cell& at(int x, int y) { 
        // Sécurité pour éviter les crashs si on demande hors limites
        if (x < 0) x = 0; if (x >= w) x = w - 1;
        if (y < 0) y = 0; if (y >= h) y = h - 1;
        return grid[y * w + x]; 
    }
    const Cell& at(int x, int y) const { 
        if (x < 0) x = 0; if (x >= w) x = w - 1;
        if (y < 0) y = 0; if (y >= h) y = h - 1;
        return grid[y * w + x]; 
    }

    void generate() {
        std::srand(static_cast<unsigned int>(std::time(0)));
        std::stack<std::pair<int,int>> stack;
        int startX = 0, startY = 0;
        at(startX, startY).visited = true;
        stack.push({startX, startY});

        while(!stack.empty()) {
            auto [cx, cy] = stack.top();
            std::vector<std::pair<int,int>> neighbors;
            const int dirs[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}}; 
            
            for(auto& d : dirs) {
                int nx = cx + d[0], ny = cy + d[1];
                if(nx >=0 && nx < w && ny >=0 && ny < h && !at(nx,ny).visited)
                    neighbors.push_back({nx, ny});
            }

            if(!neighbors.empty()) {
                auto next = neighbors[std::rand() % neighbors.size()];
                int nx = next.first, ny = next.second;
                if(nx > cx) { at(cx,cy).wE = false; at(nx,ny).wW = false; }
                else if(nx < cx) { at(cx,cy).wW = false; at(nx,ny).wE = false; }
                else if(ny > cy) { at(cx,cy).wS = false; at(nx,ny).wN = false; }
                else if(ny < cy) { at(cx,cy).wN = false; at(nx,ny).wS = false; }
                at(nx, ny).visited = true;
                stack.push(next);
            } else {
                stack.pop();
            }
        }
    }
};

// --- CLASSE BALL ---
class Ball {
    public:
        glm::vec2 pos;
        glm::vec2 vel;
        float radius;
        Mesh mesh;
    
        // Référence "plateau à plat"
        bool hasFlatRef = false;
        cv::Mat R0; // 3x3 CV_64F
    
        // Réglages
        float g = 9.81f;         // intensité
        float gain = 1.0f;       // multiplicateur global (0.5..2)
        float deadzone = 0.03f;  // petite zone morte (0.01..0.08)
    
        Ball(float r) : radius(r), pos(0,0), vel(0,0) {
            mesh = createSphere(radius, 16, 16);
            R0 = cv::Mat::eye(3,3,CV_64F);
        }
    
        void reset(const Maze& m) {
            pos = glm::vec2(m.cellW * 0.5f, m.cellH * 0.5f);
            vel = glm::vec2(0,0);
        }
    
        // Appelle ça une fois quand tu veux définir "planche à plat"
        // (par ex. au premier poseOk, ou quand tu appuies sur une touche)
        void setFlatReference(const cv::Mat& rvec) {
            cv::Mat R;
            cv::Rodrigues(rvec, R);
            if (R.type() != CV_64F) R.convertTo(R, CV_64F);
            R0 = R.clone();
            hasFlatRef = true;
        }
    
        static float applyDeadzone(float v, float dz) {
            if (std::fabs(v) < dz) return 0.0f;
            // re-scale pour éviter le "jump" au bord de la deadzone
            float s = (v > 0.0f) ? 1.0f : -1.0f;
            float a = (std::fabs(v) - dz) / (1.0f - dz);
            return s * a;
        }
    
        void update(float dt, const cv::Mat& rvec, const Maze& maze) {
    
            // 1) Rotation board->camera
            cv::Mat R;
            cv::Rodrigues(rvec, R);
            if (R.type() != CV_64F) R.convertTo(R, CV_64F);
    
            // 2) Si on n'a pas encore de référence "plat", on la prend maintenant
            // (tu peux préférer le faire dans main quand poseOk devient vrai)
            if (!hasFlatRef) {
                R0 = R.clone();
                hasFlatRef = true;
            }
    
            // 3) Rotation relative par rapport à la pose "plat"
            // Rrel = R0^T * R  (board flat -> board current) dans le même repère caméra
            cv::Mat Rrel = R0.t() * R;
    
            // 4) Gravité "monde" dans le repère FLAT de la board :
            // plateau flat : normale ~ +Z (ou -Z selon ton repère de labyrinthe).
            // Ici on choisit g0 = (0,0,-1) : gravité vers -Z.
            cv::Mat g0 = (cv::Mat_<double>(3,1) << 0.0, 0.0, -1.0);
    
            // 5) Gravité exprimée dans le repère board courant : g_cur = Rrel^T * g0
            // (car Rrel mappe flat->cur, donc pour ramener un vecteur flat vers cur : Rrel^T)
            cv::Mat g_cur = Rrel.t() * g0;
    
            // 6) Acc sur le plan : on prend X,Y
            float ax = (float)g_cur.at<double>(0,0);
            float ay = (float)g_cur.at<double>(1,0);
    
            // 7) Deadzone + gain
            ax = applyDeadzone(ax, deadzone);
            ay = applyDeadzone(ay, deadzone);
    
            glm::vec2 acc(-ax * g * gain, -ay * g * gain);



    
            // 8) Intégration
            vel += acc * dt;
            vel *= 0.85f; // frottement (ajuste)
    
            glm::vec2 nextPos = pos + vel * dt;
    
            // -------- collisions (inchangé) --------
            int gx = (int)(pos.x / maze.cellW);
            int gy = (int)(pos.y / maze.cellH);
            const auto& cell = maze.at(gx, gy);
    
            float cellLeft   = gx * maze.cellW;
            float cellRight  = (gx + 1) * maze.cellW;
            float cellTop    = gy * maze.cellH;
            float cellBottom = (gy + 1) * maze.cellH;
    
            const float bounce = 0.4f;
            const float r = radius;
    
            if (cell.wW && (nextPos.x - r < cellLeft)) {
                nextPos.x = cellLeft + r;
                vel.x = -vel.x * bounce;
            } else if (cell.wE && (nextPos.x + r > cellRight)) {
                nextPos.x = cellRight - r;
                vel.x = -vel.x * bounce;
            }
    
            if (cell.wN && (nextPos.y - r < cellTop)) {
                nextPos.y = cellTop + r;
                vel.y = -vel.y * bounce;
            } else if (cell.wS && (nextPos.y + r > cellBottom)) {
                nextPos.y = cellBottom - r;
                vel.y = -vel.y * bounce;
            }
    
            float maxW = maze.w * maze.cellW - r;
            float maxH = maze.h * maze.cellH - r;
            if(nextPos.x < r) nextPos.x = r;
            if(nextPos.y < r) nextPos.y = r;
            if(nextPos.x > maxW) nextPos.x = maxW;
            if(nextPos.y > maxH) nextPos.y = maxH;
    
            pos = nextPos;
        }
    
        void draw(GLuint prog, GLint uMVP, const glm::mat4& VP_maze_local) {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, radius));
            glm::mat4 MVP = VP_maze_local * M;
    
            glUseProgram(prog);
            glUniformMatrix4fv(uMVP, 1, GL_FALSE, glm::value_ptr(MVP));
    
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    };
    