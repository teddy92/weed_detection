#include "detection_manager.h"

//weed_detection::detected_balls front_detected_;

detectionManager::detectionManager() : arm_angle_(0)
{
  nearest_ball = NULL;
  ball_life = 2000;
  arm_lenght = 500;
  y_calibration_offset_ = 0;
  stop_threshold_ = 0;
  slow_threshold_ = 0;
  normal_vel_ = 0;
  front_cam_angle_thres_ = 5;
  spray_threshold_ = 0;

  front_detections_sub_ = node_.subscribe<weed_detection::detected_balls>("front_detections", 1, &detectionManager::front_detection_callback, this);
  arm_detections_sub_ = node_.subscribe<geometry_msgs::Pose2D>("front_detections", 1, &detectionManager::arm_detection_callback, this);
  move_to_upcoming_pub_ = node_.advertise<std_msgs::Float64>("move_to_upcoming", 1, true);
  move_precisely_ = node_.advertise<geometry_msgs::Pose2D>("move_precisely", 1, true);
  robot_vel_ = node_.advertise<std_msgs::Float64>("robot_velocity", 1, true);
  arm_state_sub_ = node_.subscribe<sensor_msgs::JointState>("arm_state", 1, &detectionManager::arm_state_callback, this);
  spray_ = node_.serviceClient<std_srvs::Trigger>("spray");
}

void detectionManager::arm_state_callback(const sensor_msgs::JointState::ConstPtr& msg)
{
  arm_angle_ = msg->position.at(0);
}

void detectionManager::arm_detection_callback(const geometry_msgs::Pose2D::ConstPtr& msg)
{
  nearest_ball_pose_ = msg;
}

float detectionManager::computeOrientation(geometry_msgs::Pose pose)
{
  //TODO
  return pose.position.x;
}

void detectionManager::reachBall()
{
  if (abs(arm_angle_ - nearest_ball->predicted_orientation) > front_cam_angle_thres_)
  {
  move_to_upcoming_pub_.publish(nearest_ball->predicted_orientation);
  }
  else
  {
    move_precisely_.publish(nearest_ball_pose_);
  }

  if(abs(nearest_ball_pose_.x - sprayer_pose_.x) < spray_threshold_ &&
      abs(nearest_ball_pose_.y - sprayer_pose_.y) < spray_threshold_)
  {
    if (spray_.call("spray"))
    {
      nearest_ball->sprayed = true;
    }
  }

}


void detectionManager::processNearestBall()
{

  // Slow down or stop the robot depending on distance_to_reach
  if (nearest_ball->distance_to_reach < stop_threshold_)
  {
    robot_vel_.publish(0.0);
  }
  else if (nearest_ball->distance_to_reach < slow_threshold_)
  {
    float vel_ratio = (float)nearest_ball->distance_to_reach / (slow_threshold_ - stop_threshold_);
    robot_vel_.publish(normal_vel_ * vel_ratio);
  }

  // If nearest ball is outside the working range of the arm
  if (nearest_ball->distance_to_reach > nearest_ball->extra_distance_by_offset)
  {
    move_to_upcoming_pub_.publish(nearest_ball->predicted_orientation);
  }
  else
  {
    reachBall();
  }
}

void detectionManager::processDatabase()
{
  for (int i = 0; i < ball_database_.size(); i++)
  {
    //Delete ball if outdated
    if (ball_database_.at(i).last_update - ros::Time::now().toSec() > ball_life)
      {
      delete &ball_database_.at(i);
      ball_database_.erase(ball_database_.begin() + i);
      i--;
      }
    else
    {
      //check if i is the first ball to be reached by the arm and hasn't been allready sprayed
      if ((ball_database_.at(i).distance_to_reach < nearest_ball->distance_to_reach) && !ball_database_.at(i).sprayed)
      {
        nearest_ball = &ball_database_.at(i);
      }
    }
  }

  processNearestBall();


}


//auto pred = [id](const registered_ball & item) {
//    return item.ID == id;
//};

void detectionManager::front_detection_callback(const weed_detection::detected_balls::ConstPtr& msg){
  //TODO Mutex
  for (int i = 0; i < msg->ids.size(); i++)
  {
    std::vector<registered_ball>::iterator it = std::find_if(ball_database_.begin(),
                                                             ball_database_.end(),
                                                             idMatcher(msg->ids.at(i)));
    if (it != ball_database_.end())
      {
      /*
       * Compute the distance in movement direction from the ball to the working range of the arm
       */
        it->extra_distance_by_offset = arm_lenght - sqrt(pow(arm_lenght, 2) -
                                                         pow(msg->predictions.at(i).position.x, 2));
        it->distance_to_reach = (msg->locations.at(i).position.y - y_calibration_offset_) + it->extra_distance_by_offset;
                                     ;
        it->predicted_orientation = computeOrientation(msg->predictions.at(i));
        it->last_update = ros::Time::now().toSec();
      }
    else
    {
      registered_ball new_entry;
      new_entry.ID = msg->ids.at(i);
      new_entry.distance_to_reach = (msg->locations.at(i).position.y - y_calibration_offset_) + (arm_lenght - sqrt(pow(arm_lenght, 2) -
                                                                                         pow(msg->predictions.at(i).position.x, 2)));
      new_entry.predicted_orientation = computeOrientation(msg->predictions.at(i));
      it->last_update = ros::Time::now().toSec();
      ball_database_.push_back(new_entry);
    }
  }
  processDatabase();
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "detection_manager");
  detectionManager dmNode;

//  ros::Rate loop_rate(30);
//
//  while (ros::ok())
//  {
//
//
//    ros::spinOnce();
//
//    loop_rate.sleep();
//  }
  ros::spin();

  return 0;
}