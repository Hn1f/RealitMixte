/**
 * @file charuco_calibration.cpp
 * @brief Version corrigée pour OpenCV 4.12 (Windows/MinGW64)
 */

 #include <opencv2/opencv.hpp>
 #include <opencv2/aruco/charuco.hpp>
 #include <iostream>
 #include <string>
 #include <vector>
 
 namespace {
 const char* about =
     "Charuco: create/detect/calibrate\n"
     "Usage:\n"
     "  ./charuco -c=1                         # create board image\n"
     "  ./charuco -c=2 [-cam=0 | -video=URL]   # detect without calibration\n"
     "  ./charuco -c=3 -calib=camera.yaml [-cam=0 | -video=URL]\n"
     "  ./charuco -c=0 -video=vid.mov -out=camera.yaml # calibrate from video file\n"
     "  ./charuco -c=0 -video=http://192.168.1.157:4747/video  -out=camera.yaml # calibrate from DroidCam\n";
 
 const char* keys =
     "{c       |      | mode: 0=calibrate, 1=create board, 2=detect w/o calib, 3=detect with calib}"
     "{calib   |      | path to camera.yaml (for c=3)}"
     "{video   |      | video path or URL (for c=0,2,3)}"
     "{cam     |0     | camera index if no video (for c=0,2,3)}"
     "{out     |camera.yaml| output yaml (for c=0)}"
     "{board   |5x7   | ChArUco intersections colsxrows (e.g. 5x7)}"
     "{square  |0.04  | square length in meters (e.g. 0.04)}"
     "{marker  |0.02  | marker length in meters (e.g. 0.02)}";
 }
 
 static bool readCameraParameters(const std::string& filename, cv::Mat& K, cv::Mat& D)
 {
     cv::FileStorage fs(filename, cv::FileStorage::READ);
     if (!fs.isOpened()) return false;
     fs["camera_matrix"] >> K;
     fs["distortion_coefficients"] >> D;
     return !K.empty() && !D.empty();
 }
 
 static bool parseBoard(const std::string& s, cv::Size& out){
     int w=0,h=0;
     if (std::sscanf(s.c_str(), "%dx%d", &w, &h)==2 && w>2 && h>2){ out=cv::Size(w,h); return true; }
     return false;
 }
 
 // Helper pour récupérer le dictionnaire (sous forme de Ptr)
 static cv::Ptr<cv::aruco::Dictionary> getDict() {
     return cv::makePtr<cv::aruco::Dictionary>(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250));
 }
 
 static void createBoardImage(cv::Size boardSize= {5,7}, float square=0.04f, float marker=0.02f) {
     auto dict = getDict();
     
     // CORRECTION 1: On passe *dict (la valeur) et non le pointeur
     cv::Ptr<cv::aruco::CharucoBoard> boardCh = cv::makePtr<cv::aruco::CharucoBoard>(boardSize, square, marker, *dict);
     
     cv::Mat img;
     // CORRECTION 2: 'draw' est devenu 'generateImage' dans OpenCV 4.x récent
     boardCh->generateImage(cv::Size(1000, 800), img, 20, 1);
     
     cv::imwrite("CharucoBoard.png", img);
     std::cout << "Board saved to CharucoBoard.png\n";
 }
 
 static bool openVideoOrCamera(cv::VideoCapture& cap, const std::string& video, int camIndex)
 {
     if (!video.empty()) {
         if (!cap.open(video, cv::CAP_FFMPEG)) {
             std::cerr << "Cannot open video/URL: " << video << "\n";
             return false;
         }
     } else {
         if (!cap.open(camIndex)) {
             std::cerr << "Cannot open camera index: " << camIndex << "\n";
             return false;
         }
     }
     return true;
 }
 
 static void detectWithoutCalibration(const std::string& video, int camIndex,
                                      cv::Size boardSize={5,7}, float square=0.04f, float marker=0.02f)
 {
     cv::VideoCapture cap;
     if (!openVideoOrCamera(cap, video, camIndex)) return;
 
     auto dict = getDict();
     // CORRECTION: *dict
     cv::Ptr<cv::aruco::CharucoBoard> boardCh = cv::makePtr<cv::aruco::CharucoBoard>(boardSize, square, marker, *dict);
     cv::Ptr<cv::aruco::DetectorParameters> params = cv::makePtr<cv::aruco::DetectorParameters>();
 
     for(;;){
         cv::Mat frame; if(!cap.read(frame) || frame.empty()) break;
         std::vector<int> ids; std::vector<std::vector<cv::Point2f>> corners;
         
         cv::aruco::detectMarkers(frame, dict, corners, ids, params);
         
         cv::Mat out = frame.clone();
         if(!ids.empty()){
             cv::aruco::drawDetectedMarkers(out, corners, ids);
             cv::Mat chC, chIds;
             cv::aruco::interpolateCornersCharuco(corners, ids, frame, boardCh, chC, chIds);
             if(!chIds.empty()) cv::aruco::drawDetectedCornersCharuco(out, chC, chIds, cv::Scalar(255,0,0));
         }
         cv::imshow("Charuco (no calib)", out);
         if((char)cv::waitKey(1)==27) break;
     }
 }
 
 static void detectWithCalibrationPose(const std::string& calibPath, const std::string& video,
                                       int camIndex, cv::Size boardSize={5,7}, float square=0.04f, float marker=0.02f)
 {
     cv::Mat K, D;
     if(!readCameraParameters(calibPath, K, D)){
         std::cerr<<"Invalid or missing calibration file: "<<calibPath<<"\n";
         return;
     }
 
     cv::VideoCapture cap;
     if (!openVideoOrCamera(cap, video, camIndex)) return;
 
     auto dict = getDict();
     // CORRECTION: *dict
     cv::Ptr<cv::aruco::CharucoBoard> boardCh = cv::makePtr<cv::aruco::CharucoBoard>(boardSize, square, marker, *dict);
     cv::Ptr<cv::aruco::DetectorParameters> params = cv::makePtr<cv::aruco::DetectorParameters>();
 
     for(;;){
         cv::Mat frame; if(!cap.read(frame) || frame.empty()) break;
 
         std::vector<int> ids; std::vector<std::vector<cv::Point2f>> corners;
         cv::aruco::detectMarkers(frame, dict, corners, ids, params);
 
         cv::Mat out = frame.clone();
         if(!ids.empty()){
             cv::aruco::drawDetectedMarkers(out, corners, ids);
             cv::Mat chC, chIds;
             cv::aruco::interpolateCornersCharuco(corners, ids, frame, boardCh, chC, chIds, K, D);
 
             if(!chIds.empty()){
                 cv::aruco::drawDetectedCornersCharuco(out, chC, chIds, cv::Scalar(255,0,0));
                 
                 cv::Vec3d rvec, tvec;
                 bool ok = cv::aruco::estimatePoseCharucoBoard(chC, chIds, boardCh, K, D, rvec, tvec);
                 if(ok) cv::drawFrameAxes(out, K, D, rvec, tvec, 0.1f);
             }
         }
 
         cv::imshow("Charuco (calibrated + pose)", out);
         if((char)cv::waitKey(1)==27) break;
     }
 }
 
 static void calibrateFromVideoWriteYaml(const std::string& video, int camIndex, const std::string& outYaml,
                                        cv::Size boardSize, float square, float marker)
 {
     cv::VideoCapture cap;
     if (!openVideoOrCamera(cap, video, camIndex)) return;
 
     auto dict = getDict();
     // CORRECTION: *dict
     cv::Ptr<cv::aruco::CharucoBoard> boardCh = cv::makePtr<cv::aruco::CharucoBoard>(boardSize, square, marker, *dict);
     cv::Ptr<cv::aruco::DetectorParameters> params = cv::makePtr<cv::aruco::DetectorParameters>();
 
     std::vector<std::vector<cv::Point2f>> allCharucoCorners;
     std::vector<std::vector<int>>         allCharucoIds;
 
     int kept=0;
     cv::Mat frame;
     while (cap.read(frame)) {
         if (frame.empty()) break;
 
         std::vector<int> ids;
         std::vector<std::vector<cv::Point2f>> corners;
         cv::aruco::detectMarkers(frame, dict, corners, ids, params);
 
         if (!ids.empty()) {
             cv::Mat chCorners, chIds;
             cv::aruco::interpolateCornersCharuco(corners, ids, frame, boardCh, chCorners, chIds);
             if (chIds.total() >= 10) {
                 std::vector<cv::Point2f> vC((cv::Point2f*)chCorners.datastart, (cv::Point2f*)chCorners.dataend);
                 std::vector<int> vI((int*)chIds.datastart, (int*)chIds.dataend);
                 allCharucoCorners.push_back(std::move(vC));
                 allCharucoIds.push_back(std::move(vI));
                 kept++;
             }
         }
 
         cv::Mat vis = frame.clone();
         if(!ids.empty()) cv::aruco::drawDetectedMarkers(vis, corners, ids);
         cv::putText(vis, cv::format("kept views: %d", kept), {20,40},
                     cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,255,255}, 2);
         cv::imshow("Calibrating...", vis);
         if((char)cv::waitKey(1)==27) break;
         if(kept>=60) break;
     }
 
     if (allCharucoCorners.size() < 8) {
         std::cerr << "Not enough valid views ("<<allCharucoCorners.size()<<").\n";
         return;
     }
 
     cv::Mat K, D;
     std::vector<cv::Mat> rvecs, tvecs;
     cv::Size imageSize(frame.cols, frame.rows);
 
     double rms = cv::aruco::calibrateCameraCharuco(
         allCharucoCorners, allCharucoIds, boardCh, imageSize, K, D, rvecs, tvecs
     );
 
     std::cout << "Calibration OK. Reprojection RMSE = " << rms << " px\n";
     std::cout << "K =\n" << K << "\nD = " << D.t() << "\n";
 
     cv::FileStorage fs(outYaml, cv::FileStorage::WRITE);
     fs << "image_width"  << imageSize.width;
     fs << "image_height" << imageSize.height;
     fs << "camera_matrix" << K;
     fs << "distortion_coefficients" << D;
     fs.release();
     std::cout << "Saved: " << outYaml << " \n";
 }
 
 int main(int argc, char** argv)
 {
     cv::CommandLineParser parser(argc, argv, keys);
     parser.about(about);
     if(!parser.has("c")){ parser.printMessage(); return 0; }
 
     int mode = parser.get<int>("c");
     std::string calib = parser.get<std::string>("calib");
     std::string video = parser.get<std::string>("video");
     int camIndex      = parser.get<int>("cam");
     std::string out   = parser.get<std::string>("out");
 
     cv::Size boardSize; parseBoard(parser.get<std::string>("board"), boardSize);
     float square = parser.get<float>("square");
     float marker = parser.get<float>("marker");
 
     switch(mode){
         case 1:
             createBoardImage(boardSize, square, marker);
             break;
         case 2:
             detectWithoutCalibration(video, camIndex, boardSize, square, marker);
             break;
         case 3:
             detectWithCalibrationPose(calib, video, camIndex, boardSize, square, marker);
             break;
         case 0:
             calibrateFromVideoWriteYaml(video, camIndex, out, boardSize, square, marker);
             break;
         default:
             parser.printMessage();
             break;
     }
     return 0;
 }