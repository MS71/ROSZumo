#include <rclcpp/rclcpp.hpp>
#include <rclcpp/qos.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>

#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>

using namespace std::placeholders;

#define RAD2DEG 57.295779513

class MinimalPoseOdomSubscriber : public rclcpp::Node {
public:
    MinimalPoseOdomSubscriber()
        : Node("odom_publisher") {

        /* Note: it is very important to use a QOS profile for the subscriber that is compatible
         * with the QOS profile of the publisher.
         * The ZED component node uses a default QoS profile with reliability set as "RELIABLE"
         * and durability set as "VOLATILE".
         * To be able to receive the subscribed topic the subscriber must use compatible
         * parameters.
         */

        // https://github.com/ros2/ros2/wiki/About-Quality-of-Service-Settings

        rclcpp::QoS qos(10);
        qos.keep_last(10);
        qos.best_effort();
        qos.durability_volatile();

        // Create pose subscriber
        mPoseSub = create_subscription<geometry_msgs::msg::PoseStamped>(
                    "pose2d", qos,
                    std::bind(&MinimalPoseOdomSubscriber::poseCallback, this, _1) );

        // Create odom subscriber
        mOdomSub = create_subscription<nav_msgs::msg::Odometry>(
                    "odom_tf", qos,
                    std::bind(&MinimalPoseOdomSubscriber::odomCallback, this, _1) );
    }

protected:
    void poseCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        // Camera position in map frame
        double tx = msg->pose.position.x;
        double ty = msg->pose.position.y;
        double tz = msg->pose.position.z;

        // Orientation quaternion
        tf2::Quaternion q(
                    msg->pose.orientation.x,
                    msg->pose.orientation.y,
                    msg->pose.orientation.z,
                    msg->pose.orientation.w);

        // 3x3 Rotation matrix from quaternion
        tf2::Matrix3x3 m(q);

        // Roll Pitch and Yaw from rotation matrix
        double roll, pitch, yaw;
        m.getRPY(roll, pitch, yaw);

        // Output the measure
        RCLCPP_INFO(get_logger(), "Received pose in '%s' frame : X: %.2f Y: %.2f Z: %.2f - R: %.2f P: %.2f Y: %.2f - Timestamp: %u.%u sec ",
                 msg->header.frame_id.c_str(),
                 tx, ty, tz,
                 roll * RAD2DEG, pitch * RAD2DEG, yaw * RAD2DEG,
                    msg->header.stamp.sec,msg->header.stamp.nanosec);
    }

    void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        // Camera position in map frame
        double tx = msg->pose.pose.position.x;
        double ty = msg->pose.pose.position.y;
        double tz = msg->pose.pose.position.z;

        // Orientation quaternion
        tf2::Quaternion q(
                    msg->pose.pose.orientation.x,
                    msg->pose.pose.orientation.y,
                    msg->pose.pose.orientation.z,
                    msg->pose.pose.orientation.w);

        // 3x3 Rotation matrix from quaternion
        tf2::Matrix3x3 m(q);

        // Roll Pitch and Yaw from rotation matrix
        double roll, pitch, yaw;
        m.getRPY(roll, pitch, yaw);

        // Output the measure
        RCLCPP_INFO(get_logger(), "Received odometry in '%s' frame : X: %.2f Y: %.2f Z: %.2f - R: %.2f P: %.2f Y: %.2f - Timestamp: %u.%u sec ",
                 msg->header.frame_id.c_str(),
                 tx, ty, tz,
                 roll * RAD2DEG, pitch * RAD2DEG, yaw * RAD2DEG,
                    msg->header.stamp.sec,msg->header.stamp.nanosec);
    }

private:
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr mPoseSub;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr mOdomSub;
};



int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MinimalPoseOdomSubscriber>());
    rclcpp::shutdown();
    return 0;
}







#if 0

#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "geometry_msgs/msg/pose2_d.hpp"
using std::placeholders::_1;

class MinimalSubscriber : public rclcpp::Node
{
  public:
    MinimalSubscriber()
    : Node("minimal_subscriber")
    {
      subscription_ = this->create_subscription<geometry_msgs::msg::Pose2D>(
      "/ros2zumo/pose2d", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
    }

  private:
    void topic_callback(const geometry_msgs::msg::Pose2D::SharedPtr msg) const
    {
        printf("got geometry_msgs::msg::Pose2D\n");
      //RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg->data.c_str());
      (void)msg;
    }
    rclcpp::Subscription<geometry_msgs::msg::Pose2D>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MinimalSubscriber>());
  rclcpp::shutdown();
  return 0;
}
#endif


#if 0
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

/* This example creates a subclass of Node and uses std::bind() to register a
 * member function as a callback from the timer. */

class MinimalPublisher : public rclcpp::Node
{
public:
    MinimalPublisher()
        : Node("odom_publisher")
        , count_(0)
    {
        publisher_ = this->create_publisher<std_msgs::msg::String>("/ros2zumo/pose2d", 10);
        timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));
    }

private:
    void timer_callback()
    {
        auto message = std_msgs::msg::String();
        message.data = "Hello, world! " + std::to_string(count_++);
        RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
        publisher_->publish(message);
    }
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    size_t count_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MinimalPublisher>());
    rclcpp::shutdown();
    return 0;
}
#endif