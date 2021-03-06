#include "ball_detector.h"

using namespace cv;

/*
 * Constructor
 */
BallDetector::BallDetector(std::string name)
{
  // Initialize the ballDetector
  name_ = name;
  nh_ = ros::NodeHandle("~");
  loadParameters();
  // Open window for real time settings if param set accordingly
  if (show_gui_)
    settingsWindow();

//  tracker_ = new CTracker(0.2,0.5,60.0,10,10);
  // Initialize the tracker
  tracker_ = new CTracker(kalman_dt_, kalman_accel_noise_mag_, hungarian_dist_thres_, hungarian_max_skipped_frames_, 10);
}

/*
 * Initialize the camera device
 */
int BallDetector::initialize_camera()
{
  cam_ = new VideoCapture(camera_device_);
  // Check if camera works
  if (!cam_->isOpened())
  {
    ROS_FATAL_STREAM("[" << name_ << "] Camera device " << camera_device_ << " not found.");
    return -1;
  }
  ROS_INFO_STREAM("[" << name_ << "] Camera initialized. FPS: " << cam_->get(CV_CAP_PROP_FPS));
  return 0;
}

/*
 * Load parameters from ROS-Parameterserver
 */
void BallDetector::loadParameters()
{
  if (!nh_.hasParam("camera_device"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for camera_device [0]");
  nh_.param("camera_device", camera_device_, 0);

  //Setting HSV threshold values
  if (!nh_.hasParam("H_min"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for H_min [0]");
  nh_.param("H_min", H_min_, 0);

  if (!nh_.hasParam("H_max"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for H_max [0]");
  nh_.param("H_max", H_max_, 0);

  if (!nh_.hasParam("S_min"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for S_min [0]");
  nh_.param("S_min", S_min_, 0);

  if (!nh_.hasParam("S_max"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for S_max [0]");
  nh_.param("S_max", S_max_, 0);

  if (!nh_.hasParam("V_min"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for V_min [0]");
  nh_.param("V_min", V_min_, 0);

  if (!nh_.hasParam("V_max"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for V_max [0]");
  nh_.param("V_max", V_max_, 0);

  //Setting other parameters
  if (!nh_.hasParam("min_contour_area"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for min_contour_area [0]");
  nh_.param("min_contour_area", min_contour_area_, 0);

  if (!nh_.hasParam("min_contour_radius"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for min_contour_radius [0]");
  nh_.param("min_contour_radius", min_contour_circle_radius_, 0);

  if (!nh_.hasParam("max_contour_radius"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for max_contour_radius [0]");
  nh_.param("max_contour_radius", max_contour_circle_radius_, 0);

  if (!nh_.hasParam("min_area_ratio"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for min_area_ratio [0]");
  nh_.param("min_area_ratio", min_area_ratio_, 0);

  if (!nh_.hasParam("show_gui"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for show_gui [true]");
  nh_.param("show_gui", show_gui_, true);

  if (!nh_.hasParam("kalman_dt"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for kalman_dt [0]");
  nh_.param("kalman_dt", kalman_dt_, 0.2);

  if (!nh_.hasParam("kalman_accel_noise_mag"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for kalman_accel_noise_mag [0]");
  nh_.param("kalman_accel_noise_mag", kalman_accel_noise_mag_, 0.5);

  if (!nh_.hasParam("hungarian_dist_thres"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for hungarian_dist_thres [0]");
  nh_.param("hungarian_dist_thres", hungarian_dist_thres_, 60.0);

  if (!nh_.hasParam("hungarian_max_skipped_frames"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for hungarian_max_skipped_frames_ [0]");
  nh_.param("hungarian_max_skipped_frames", hungarian_max_skipped_frames_, 10);

  if (!nh_.hasParam("detect_manager_arm_lenght"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for detect_manager_arm_lenght [0]");
  nh_.param("detect_manager_arm_lenght", detect_manager_arm_lenght_, 0);

  if (!nh_.hasParam("detect_manager_yoffset"))
    ROS_WARN_STREAM("[" << name_ << "] Used default parameter for detect_manager_yoffset [0]");
  nh_.param("detect_manager_yoffset", detect_manager_yoffset_, 0);

  kalman_dt_int_ = kalman_dt_ * 100;
  kalman_accel_noise_mag_int_ = kalman_accel_noise_mag_ * 100;
  hungarian_dist_thres_int_ = hungarian_dist_thres_ * 100;
}

/*
 * Open a window where settings can be changed.
 * Most parameters can be adjusted in realtime but this was not tested for everything.
 */
void BallDetector::settingsWindow()
{
  std::string settings_name = "Settings "+ name_;
  namedWindow(settings_name, CV_WINDOW_AUTOSIZE);

  createTrackbar("H_min", settings_name, &H_min_, 179);
  createTrackbar("H_max", settings_name, &H_max_, 179);
  createTrackbar("S_min", settings_name, &S_min_, 255);
  createTrackbar("S_max", settings_name, &S_max_, 255);
  createTrackbar("V_min", settings_name, &V_min_, 255);
  createTrackbar("V_max", settings_name, &V_max_, 255);

  createTrackbar("min_contour_area", settings_name, &min_contour_area_, 5000);
  createTrackbar("min_contour_radius", settings_name, &min_contour_circle_radius_, 100);
  createTrackbar("max_contour_radius", settings_name, &max_contour_circle_radius_, 100);
  createTrackbar("min_area_ratio", settings_name, &min_area_ratio_, 100);

  createTrackbar("kalman_dt", settings_name, &kalman_dt_int_, 100);
  createTrackbar("kalman_accel_noise_mag", settings_name, &kalman_accel_noise_mag_int_, 100);
  createTrackbar("hungarian_dist_thres", settings_name, &hungarian_dist_thres_int_, 10000);
  createTrackbar("hungarian_max_skipped_frames", settings_name, &hungarian_max_skipped_frames_, 100);

  createTrackbar("detect_manager_arm_lenght_", settings_name, &detect_manager_arm_lenght_, 1000);
  createTrackbar("detect_manager_yoffset_", settings_name, &detect_manager_yoffset_, 1000);
}

/*
 * Compute the positions of the balls and feed them into the Kalman-Filter
 */
vector<tracked_ball>* BallDetector::processFrame()
{
  Mat orig_frame;
  // Get a new frame from camera
  cam_->read(orig_frame);
  if (show_gui_)
  {
    imshow("Original "+ name_, orig_frame);
    cv::waitKey(1);
  }
  Mat hsv_frame;
  // Convert frame to black and white
  cvtColor(orig_frame, hsv_frame, COLOR_BGR2HSV);

  // Extraction of areas of the right color
  Mat thres_frame;

  // Check if selected range crosses 0 and 179, as 0 and 179 are the same color. (i.e. to filter red objects)
  if (H_min_ > H_max_)
  {
    Mat thres_frame_1, thres_frame_2;
    inRange(hsv_frame, Scalar(0, S_min_, V_min_), Scalar(H_max_, S_max_, V_max_), thres_frame_1);
    inRange(hsv_frame, Scalar(H_min_, S_min_, V_min_), Scalar(179, S_max_, V_max_), thres_frame_2);
    thres_frame = thres_frame_1 + thres_frame_2;
  }
  else
  {
    inRange(hsv_frame, Scalar(H_min_, S_min_, V_min_), Scalar(H_max_, S_max_, V_max_), thres_frame);
  }

//  if (show_gui_)
//    imshow("Thresholded "+ name_, thres_frame);

  // Morphological opening
  Mat morph_frame;
  erode(thres_frame, morph_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
  dilate(morph_frame, morph_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

  // Morphological closing
//        dilate(morph_frame, morph_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
//        erode(morph_frame, morph_frame, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

  if (show_gui_)
    imshow("Morphed "+ name_, morph_frame);

  //TODO Maybe Canny

  // Vectors for storing the extracted contours
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;

  findContours(morph_frame, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

  ROS_DEBUG_STREAM_NAMED("processFrame", "[" << name_ << "] Extracted " << contours.size() << " contours.");

  std::vector<Point2f>* detected_balls = processContours(contours, hierarchy, &orig_frame);

  //TODO skip?
  std::vector<Point2d> detected_balls_d;
  for (int iter = 0 ; iter < (int)detected_balls->size() ; iter++)
    {
      detected_balls_d.push_back((cv::Point2d)detected_balls->at(iter));
    }

//  if (detected_balls->size() > 0)
//  {

  // Set the parameters for kalman and hungarian filters.
  tracker_->dt = kalman_dt_int_ / 100.0;
  tracker_->Accel_noise_mag = kalman_accel_noise_mag_int_ / 100.0;
  tracker_->dist_thres = hungarian_dist_thres_int_ / 100.0;
  tracker_->maximum_allowed_skipped_frames = hungarian_max_skipped_frames_;
  // Feed the detected balls to the tracker
  tracker_->Update(detected_balls_d);


  ROS_DEBUG_STREAM_NAMED("processFrame", "[" << name_ << "] Computed " << tracker_->tracks.size() << " tracks.");

  // Array of colors for easy
  Scalar Colors[]={Scalar(255,0,0),Scalar(0,255,0),Scalar(0,0,255),Scalar(255,255,0),Scalar(0,255,255),
                   Scalar(255,0,255),Scalar(255,127,255),Scalar(127,0,255),Scalar(127,0,127)};

  for(int i=0;i<tracker_->tracks.size();i++)
  {
          if(tracker_->tracks[i]->trace.size()>1)
          {
                  for(int j=0;j<tracker_->tracks[i]->trace.size()-1;j++)
                  {
                          line(orig_frame,tracker_->tracks[i]->trace[j],tracker_->tracks[i]->trace[j+1],Colors[tracker_->tracks[i]->track_id%9],2,CV_AA);
                  }
                  putText(orig_frame,
                          static_cast<ostringstream*>( &(ostringstream() << tracker_->tracks[i]->track_id) )->str(), (Point)*tracker_->tracks[i]->trace.begin(),
                          FONT_HERSHEY_SIMPLEX,
                          0.5,
                          Scalar(255, 255, 255),
                          2,
                          8);
          }
  }
  if (show_gui_){
        if (detect_manager_arm_lenght_ != 0)
              ellipse(orig_frame, Point(orig_frame.cols / 2, detect_manager_yoffset_ + detect_manager_arm_lenght_ ), Size(detect_manager_arm_lenght_, detect_manager_arm_lenght_), 0, 0, 360, Scalar(255, 0, 0), 2, 8, 0);

        imshow("Extracted balls "+ name_, orig_frame);
      }

  vector<tracked_ball>* tracked_balls = new vector<tracked_ball>;
  for(int i=0;i<tracker_->tracks.size();i++)
  {
    if (tracker_->tracks.at(i)->trace.size() < 5)
      continue;
    tracked_ball new_ball;
    new_ball.id = tracker_->tracks.at(i)->track_id;
    new_ball.location = tracker_->tracks.at(i)->trace.at(tracker_->tracks.at(i)->trace.size() - 1);
    new_ball.prediction = tracker_->tracks.at(i)->KF->LastResult;
    tracked_balls->push_back(new_ball);
  }
  return tracked_balls;
}

/*
 * Morphological filtering of the found hypotheses
 */
std::vector<Point2f>* BallDetector::processContours(std::vector<std::vector<Point> > contours,
                                                                 std::vector<cv::Vec4i> hierarchy,
                                                                 Mat* frame)
{
  std::vector<Point2f> circle_midpoint(contours.size());
  std::vector<float> circle_radius(contours.size());
  std::vector<float> contour_area(contours.size());
  float min_area_ratio_float = min_area_ratio_ / 100.0;
  std::vector<Point2f>* detected_balls = new std::vector<Point2f>;
  for (int i = 0; i < contours.size(); i++)
  {
    //compute the areas of the contours
    contour_area.at(i) = contourArea(contours.at(i));
    //discard the contour if it's smaller than min_contour_area_
    if (contour_area.at(i) < min_contour_area_)
      continue;
    //compute the circle of the minimum area enclosing the contour
    minEnclosingCircle(contours.at(i), circle_midpoint.at(i), circle_radius.at(i));
    //discard the contour if the enclosing circle is too small or too big
    if (circle_radius.at(i) < min_contour_circle_radius_ || circle_radius.at(i) > max_contour_circle_radius_)
      continue;
    /*
     * compute the ratio area_of_contour/area_of_sorrounding_circle
     * to know how circle-like the contour is then discard contour if bad
     */
    if (contour_area.at(i) / (pow(circle_radius.at(i), 2) * M_PI) < min_area_ratio_float)
      continue;
    detected_balls->push_back(circle_midpoint.at(i));

    if (show_gui_){
      Scalar color = Scalar(255, 255, 255);
      Scalar color2 = Scalar(255, 0, 0);
      drawContours(*frame, contours, i, color, 2, 8, hierarchy, 0, Point());
      circle(*frame, circle_midpoint.at(i), 4, color2, -1, 8, 0);
      circle(*frame, circle_midpoint.at(i), circle_radius.at(i), color2, 2, 8, 0);
       }
  }

  return detected_balls;
}
