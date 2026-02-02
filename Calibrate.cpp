// calibrate_and_write_camera_yaml.cpp
// Calibrage caméra + génération automatique de camera.yaml (OpenCV C++)
// ------------------------------------------------------------------
// Usage :
//   ./calibrate --cam 0 --board 9x6 --square 25 --frames 25 --out camera.yaml
//   ./calibrate --video input.mp4 --board 9x6 --square 25 --frames 30 --out camera.yaml
//
// iPhone / URL (DroidCam, Iriun, etc.) :
//   ./calibrate --source "http://192.168.1.136:4747/video" --board 9x6 --square 25 --frames 30 --out camera.yaml
//   ./calibrate --source "http://192.168.1.136:4747/video" --rotate 90 --width 1280 --height 720 --fps 30 --board 9x6 --square 25 --frames 30 --out camera.yaml
//
// Build :
//   g++ -std=c++17 calibrate_and_write_camera_yaml.cpp -o calibrate `pkg-config --cflags --libs opencv4`

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>     // sscanf
#include <algorithm>  // std::all_of

struct Args {
    int camIndex = 0;

    // Ancien mode
    std::string videoPath;       // fichier ou URL

    // Nouveau mode unifié
    std::string source;          // "0" (cam index) ou URL ou fichier

    // Best-effort (dépend backend/flux)
    int reqWidth = 0;
    int reqHeight = 0;
    int reqFps = 0;
    int rotate = 0;              // 0, 90, 180, 270

    cv::Size board{9,6};         // inner corners (cols x rows)
    float squareSize = 25.0f;    // mm (informative)
    int targetFrames = 25;
    std::string outPath = "camera.yaml";
};

static bool parseBoard(const std::string& s, cv::Size& out){
    // Extrait tous les nombres trouvés dans la string
    std::vector<int> nums;
    int cur = -1;

    for (unsigned char c : s) {
        if (std::isdigit(c)) {
            if (cur < 0) cur = (c - '0');
            else cur = cur * 10 + (c - '0');
        } else {
            if (cur >= 0) { nums.push_back(cur); cur = -1; }
        }
    }
    if (cur >= 0) nums.push_back(cur);

    if (nums.size() >= 2) {
        int w = nums[0];
        int h = nums[1];
        if (w >= 2 && h >= 2) {
            out = cv::Size(w, h);
            return true;
        }
    }

    std::cerr << "parseBoard failed for raw: [" << s << "]\n";
    return false;
}



static Args parse(int argc, char** argv){
    Args a;

    auto requireValue = [&](const std::string& opt, int& i) -> std::string {
        if (i + 1 >= argc) {
            std::cerr << "Missing value after " << opt << "\n";
            std::exit(1);
        }
        return std::string(argv[++i]); // <-- le seul incrément autorisé
    };

    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];

        if (k == "--cam") {
            a.camIndex = std::stoi(requireValue(k, i));
        }
        else if (k == "--video") {
            a.videoPath = requireValue(k, i);
        }
        else if (k == "--source") {
            a.source = requireValue(k, i);
        }
        else if (k == "--board") {
            std::string v = requireValue(k, i);
            if (!parseBoard(v, a.board)) {
                std::cerr << "--board format attendu WxH, ex: 9x6\n";
                std::exit(1);
            }
        }
        else if (k == "--square") {
            a.squareSize = std::stof(requireValue(k, i));
        }
        else if (k == "--frames") {
            a.targetFrames = std::max(10, std::stoi(requireValue(k, i)));
        }
        else if (k == "--out") {
            a.outPath = requireValue(k, i);
        }
        else if (k == "--width") {
            a.reqWidth = std::stoi(requireValue(k, i));
        }
        else if (k == "--height") {
            a.reqHeight = std::stoi(requireValue(k, i));
        }
        else if (k == "--fps") {
            a.reqFps = std::stoi(requireValue(k, i));
        }
        else if (k == "--rotate") {
            a.rotate = std::stoi(requireValue(k, i));
        }
        else {
            std::cerr << "Argument inconnu: " << k << "\n";
            std::exit(1);
        }
    }

    if (!(a.rotate==0 || a.rotate==90 || a.rotate==180 || a.rotate==270)){
        std::cerr << "--rotate doit être 0, 90, 180 ou 270\n";
        std::exit(1);
    }

    return a;
}


static std::vector<cv::Point3f> makeObjectCorners(cv::Size board, float square){
    std::vector<cv::Point3f> obj; obj.reserve(board.width*board.height);
    for(int j=0;j<board.height;++j)
        for(int i=0;i<board.width;++i)
            obj.emplace_back(i*square, j*square, 0.0f);
    return obj;
}

// ─────────── Helpers diversité (translation/échelle/rotation) ───────────
static cv::Point2f centroid(const std::vector<cv::Point2f>& pts){
    cv::Point2f c(0,0);
    for (auto&p:pts) c+=p;
    if(!pts.empty()) c *= 1.f/float(pts.size());
    return c;
}
static double meanEdge(const std::vector<cv::Point2f>& pts){
    if (pts.size()<2) return 0.0;
    double s=0;
    for(size_t i=1;i<pts.size();++i) s += cv::norm(pts[i]-pts[i-1]);
    return s / double(pts.size()-1);
}
static double principalAngleDeg(const std::vector<cv::Point2f>& pts){
    if (pts.size()<2) return 0.0;
    cv::PCA p(cv::Mat(pts).reshape(1), cv::Mat(), cv::PCA::DATA_AS_ROW);
    cv::Point2d v(p.eigenvectors.at<float>(0,0), p.eigenvectors.at<float>(0,1));
    double a = std::atan2(v.y, v.x) * 180.0 / CV_PI;
    if (a>90) a-=180; else if (a<-90) a+=180;
    return a;
}
static bool isDifferentEnough(const std::vector<cv::Point2f>& cur,
                              const std::vector<cv::Point2f>& last,
                              int W, int H)
{
    if (last.empty() || cur.empty()) return true;

    const double ref = std::min(W,H);
    const double TH_SHIFT = 0.06;   // 6% du min(W,H)
    const double TH_SCALE = 0.06;   // 6% variation relative
    const double TH_ROT   = 8.0;    // degrés

    bool shiftOK = (cv::norm(centroid(cur)-centroid(last)) > TH_SHIFT*ref);

    double s1 = meanEdge(cur), s2 = meanEdge(last);
    bool scaleOK = (s2>1e-6) ? (std::abs((s1-s2)/s2) > TH_SCALE) : true;

    bool rotOK = (std::abs(principalAngleDeg(cur) - principalAngleDeg(last)) > TH_ROT);

    return shiftOK || scaleOK || rotOK;
}

// ──────────────────────────────────────────────────────────────────────────

static void applyRotateIfNeeded(cv::Mat& frame, int rotateDeg){
    if (frame.empty() || rotateDeg==0) return;
    if (rotateDeg == 90) cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
    else if (rotateDeg == 180) cv::rotate(frame, frame, cv::ROTATE_180);
    else if (rotateDeg == 270) cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE);
}

int main(int argc,char**argv){
    Args args = parse(argc,argv);

    // ─────────── Capture (cam index / fichier / URL iPhone) ───────────
    cv::VideoCapture cap;

    // Source unifiée : --source > --video > --cam
    std::string src = args.source;
    if (src.empty() && !args.videoPath.empty()) src = args.videoPath;

    if (!src.empty()) {
        // Si numérique => index caméra
        bool allDigits = std::all_of(src.begin(), src.end(),
                                     [](unsigned char c){ return std::isdigit(c); });
        if (allDigits) {
            args.camIndex = std::stoi(src);
            cap.open(args.camIndex, cv::CAP_ANY);
        } else {
            // URL (DroidCam/Iriun/RTSP/HTTP) ou fichier
            cap.open(src, cv::CAP_FFMPEG);
            if (!cap.isOpened()) cap.open(src, cv::CAP_ANY); // fallback
        }
    } else {
        cap.open(args.camIndex, cv::CAP_ANY);
    }

    if(!cap.isOpened()){
        std::cerr << "Impossible d'ouvrir la source vidéo.\n";
        std::cerr << "Exemples:\n";
        std::cerr << "  ./calibrate --cam 0 ...\n";
        std::cerr << "  ./calibrate --video input.mp4 ...\n";
        std::cerr << "  ./calibrate --source \"http://IP:PORT/video\" ...\n";
        return 1;
    }

    // Best-effort (peut ne pas être respecté par un flux réseau)
    if (args.reqWidth  > 0) cap.set(cv::CAP_PROP_FRAME_WIDTH,  args.reqWidth);
    if (args.reqHeight > 0) cap.set(cv::CAP_PROP_FRAME_HEIGHT, args.reqHeight);
    if (args.reqFps    > 0) cap.set(cv::CAP_PROP_FPS,          args.reqFps);

    cv::Mat frame, gray;
    std::vector<std::vector<cv::Point2f>> allCorners;
    std::vector<std::vector<cv::Point3f>> allObjects;

    const auto obj = makeObjectCorners(args.board, args.squareSize);

    int taken=0;
    cv::Size imageSize;

    // Mémoire pour diversité
    std::vector<cv::Point2f> lastKeptCorners;
    int framesSinceKeep = 0;
    const int MIN_GAP_FRAMES = 3; // pas plus d'une vue tous les 3 frames

    std::cout << "Calibrage : montrez le damier "<<args.board.width<<"x"<<args.board.height
              << " sous differents angles, rapprochez/eloignez...\n"
              << "Objectif: "<<args.targetFrames<<" vues valides. 'S' pour forcer la prise, 'q' pour terminer.\n";

    for(;;){
        if(!cap.read(frame)) break;

        applyRotateIfNeeded(frame, args.rotate);
        imageSize = frame.size();

        if (frame.channels()==3) cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        else if (frame.channels()==4) cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
        else gray = frame;

        // Détection
        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCorners(
            gray, args.board, corners,
            cv::CALIB_CB_ADAPTIVE_THRESH |
            cv::CALIB_CB_NORMALIZE_IMAGE |
            cv::CALIB_CB_FAST_CHECK
        );

        if(found){
            cv::cornerSubPix(
                gray, corners, cv::Size(11,11), cv::Size(-1,-1),
                cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 30, 0.01)
            );
            cv::drawChessboardCorners(frame, args.board, corners, true);
        }

        // HUD
        cv::putText(frame, cv::format("frames: %d/%d", taken, args.targetFrames),
                    {20,30}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {0,255,0}, 2);
        cv::putText(frame, found?"pattern: OK":"pattern: --",
                    {20,60}, cv::FONT_HERSHEY_SIMPLEX, 0.8,
                    found?cv::Scalar(0,255,0):cv::Scalar(0,0,255), 2);
        cv::putText(frame, "S: save, q/ESC: calibrate now",
                    {20,90}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {255,255,0}, 1);

        cv::imshow("Calibrage", frame);

        int key = cv::waitKey(1);
        bool pressedS = (key=='s' || key=='S');
        if(key=='q' || key==27) break;

        // Conservation (diversité + anti-spam)
        if (found) {
            bool different = isDifferentEnough(corners, lastKeptCorners, frame.cols, frame.rows);
            if (taken==0 || pressedS || (different && framesSinceKeep >= MIN_GAP_FRAMES)) {
                allCorners.push_back(corners);
                allObjects.push_back(obj);
                lastKeptCorners = corners;
                ++taken;
                framesSinceKeep = 0;
                std::cout << "[+] kept " << taken << (pressedS ? " (manual)\n" : " (auto)\n");
            } else {
                ++framesSinceKeep;
            }
        } else {
            ++framesSinceKeep;
        }

        if(taken >= args.targetFrames) break;
    }

    if(allCorners.size() < 8){
        std::cerr << "Pas assez de vues valides ("<<allCorners.size()<<"). Relance avec plus de prises variees.\n";
        return 2;
    }

    // Calibrage
    cv::Mat K, dist; std::vector<cv::Mat> rvecs, tvecs;
    int flags = 0; // modèle 5 coeffs par défaut
    cv::TermCriteria tc(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 100, 1e-6);
    double rms = cv::calibrateCamera(allObjects, allCorners, imageSize, K, dist, rvecs, tvecs, flags, tc);

    // Assurer 1x5 pour YAML
    cv::Mat dist5 = cv::Mat::zeros(1,5,CV_64F);
    for(int i=0;i<std::min(5, (int)dist.total()); ++i){
        // dist peut être 1xN ou Nx1
        dist5.at<double>(0,i) = dist.reshape(1,1).at<double>(0,i);
    }

    std::cout << "\nRMS reprojection error: "<< rms <<" px\n";
    std::cout << "Image size: "<< imageSize << "\n";
    std::cout << "K =\n" << K << "\n";
    std::cout << "dist(1x5) = " << dist5 << "\n";
    std::cout << cv::format("fx=%.3f, fy=%.3f, cx=%.3f, cy=%.3f\n",
                             K.at<double>(0,0), K.at<double>(1,1), K.at<double>(0,2), K.at<double>(1,2));

    // Écriture YAML (format OpenCV attendu)
    {
        cv::FileStorage fs(args.outPath, cv::FileStorage::WRITE);
        fs << "image_width"  << imageSize.width;
        fs << "image_height" << imageSize.height;
        fs << "camera_matrix" << K;
        fs << "distortion_coefficients" << dist5;
    }
    std::cout << "\nParametres ecrits dans: " << args.outPath << "\n";
    std::cout << "Astuce: si vous changez la resolution plus tard, re-scalez (fx,fy,cx,cy) proportionnellement.\n";

    // Aperçu undistort SANS ZOOM/CROP (alpha=1.0)
    {
        cv::Mat newK = cv::getOptimalNewCameraMatrix(K, dist5, imageSize, /*alpha=*/1.0, imageSize);
        cv::Mat map1, map2;
        cv::initUndistortRectifyMap(K, dist5, cv::Mat(), newK, imageSize, CV_16SC2, map1, map2);

        cv::Mat undist;
        if (!frame.empty()) {
            cv::remap(frame, undist, map1, map2, cv::INTER_LINEAR);
            cv::imshow("Undistort (aperçu, alpha=1.0)", undist);
            cv::waitKey(0);
        }
    }

    return 0;
}
