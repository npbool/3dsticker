#ifndef __HEAD_POSE_ESTIMATION
#define __HEAD_POSE_ESTIMATION

#include <opencv2/core/core.hpp>
#include <dlib/opencv.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>

#include <vector>
#include <array>
#include <string>

// Anthropometric for male adult
// Relative position of various facial feature relative to sellion
// Values taken from https://en.wikipedia.org/wiki/Human_head
// X points forward
//const static cv::Point3f P3D_SELLION(0., 0.,0.);
//const static cv::Point3f P3D_RIGHT_EYE(-20., -65.5,-5.);
//const static cv::Point3f P3D_LEFT_EYE(-20., 65.5,-5.);
//const static cv::Point3f P3D_RIGHT_EAR(-100., -77.5,-6.);
//const static cv::Point3f P3D_LEFT_EAR(-100., 77.5,-6.);
//const static cv::Point3f P3D_NOSE(21.0, 0., -48.0);
//const static cv::Point3f P3D_STOMMION(10.0, 0., -75.0);
//const static cv::Point3f P3D_MENTON(0., 0.,-133.0);
//z, x, y


const static cv::Point3f P3D_SELLION(0.0f, 0.0f,0.0f);
const static cv::Point3f P3D_RIGHT_EYE(65.5f, -5.0f, -20.0f);
const static cv::Point3f P3D_LEFT_EYE(-65.5f, -5.0f, -20.0f);
const static cv::Point3f P3D_RIGHT_EAR(77.5f, -6.0f, -100.0f);
const static cv::Point3f P3D_LEFT_EAR(-77.5f, -6.0f, -100.0f);
const static cv::Point3f P3D_NOSE(0.0f, -48.0f, 21.0f);
const static cv::Point3f P3D_STOMMION(0.0f, -75.0f, 10.0f);
const static cv::Point3f P3D_MENTON(0.0f, -133.0f, 0.0f);


static const int MAX_FEATURES_TO_TRACK=100;

// Interesting facial features with their landmark index
enum FACIAL_FEATURE {
    NOSE=30,
    RIGHT_EYE=45,
    LEFT_EYE=36,
    RIGHT_SIDE=16,
    LEFT_SIDE=0,
    EYEBROW_RIGHT=22,
    EYEBROW_LEFT=21,
    MOUTH_UP=51,
    MOUTH_DOWN=57,
    MOUTH_RIGHT=48,
    MOUTH_LEFT=54,
    SELLION=27,
    MOUTH_CENTER_TOP=62,
    MOUTH_CENTER_BOTTOM=66,
    MENTON=8
};


typedef cv::Matx44d head_pose;

class HeadPoseEstimation {

public:

    HeadPoseEstimation(float focalLength, const std::string& face_detection_model = "shape_predictor_68_face_landmarks.dat");

    void update(cv::InputArray image);

    head_pose pose(size_t face_idx) const;

    std::vector<head_pose> poses() const;

    float focalLength;
    float opticalCenterX;
    float opticalCenterY;

    mutable cv::Mat _debug;

    dlib::cv_image<dlib::bgr_pixel> current_image;

    dlib::frontal_face_detector detector;
    dlib::shape_predictor pose_model;

    std::vector<dlib::rectangle> faces;

    std::vector<dlib::full_object_detection> shapes;

    void my_project_points(std::vector<cv::Point3f> points, head_pose pose, std::vector<cv::Point2f>& projected_points) const;


    /** Return the point corresponding to the dictionary marker.
    */
    cv::Point2f coordsOf(size_t face_idx, FACIAL_FEATURE feature) const;

    /** Returns true if the lines intersect (and set r to the intersection
     *  coordinates), false otherwise.
     */
    bool intersection(cv::Point2f o1, cv::Point2f p1,
                      cv::Point2f o2, cv::Point2f p2,
                      cv::Point2f &r) const;

};

#endif // __HEAD_POSE_ESTIMATION
