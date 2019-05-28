#include <iostream>
#include <ros/ros.h>
#include <tf/tf.h>
#include <nav_msgs/Odometry.h>
#include <grid_map/grid_map.h>
#include <grid_map/grid_mapper.h>

GridMap* g_map;
GridMapper* g_gmapper;
//tf::StampedTransform g_transform;
ros::Subscriber g_odom_suber;
ros::Publisher g_map_puber;
Pose2d g_robot_pose;

void odometryCallback ( const nav_msgs::OdometryConstPtr& odom );
void laserCallback ( const sensor_msgs::LaserScanConstPtr& scan );

int main ( int argc, char **argv )
{
    ros::init ( argc, argv, "OdometryMapping" );
    ros::NodeHandle nh;

    /***** parameters *****/
    int map_sizex, map_sizey, map_initx, map_inity;
    double map_cell_size;
    /* TODO 错误处理 */
    nh.getParam ( "/mapping/map/sizex", map_sizex );
    nh.getParam ( "/mapping/map/sizey", map_sizey );
    nh.getParam ( "/mapping/map/initx", map_initx );
    nh.getParam ( "/mapping/map/inity", map_inity );
    nh.getParam ( "/mapping/map/cell_size", map_cell_size );

    Pose2d T_r_l;
    double x, y, theta;
    double P_occ, P_free, P_prior;
    /* TODO 错误处理 */
    nh.getParam ( "/mapping/robot_laser/x", x );
    nh.getParam ( "/mapping/robot_laser/y", y );
    nh.getParam ( "/mapping/robot_laser/theta", theta );
    T_r_l = Pose2d ( x, y, theta );

    nh.getParam ( "/mapping/sensor_model/P_occ", P_occ );
    nh.getParam ( "/mapping/sensor_model/P_free", P_free );
    nh.getParam ( "/mapping/sensor_model/P_prior", P_prior );
    

    std::string map_image_save_dir, map_config_save_dir;
    nh.getParam ( "/mapping/map_image_save_dir", map_image_save_dir );
    nh.getParam ( "/mapping/map_config_save_dir", map_config_save_dir );

    /***** Initialization *****/
    g_map = new GridMap ( map_sizex, map_sizey,  map_initx, map_inity, map_cell_size );
    g_gmapper = new GridMapper ( g_map, T_r_l, P_occ, P_free, P_prior );

    /***** advertise and subscribe Topic *****/
    g_odom_suber = nh.subscribe ( "/mbot/odometry", 1, odometryCallback );
    ros::Subscriber laser_suber = nh.subscribe ( "/scan", 1, laserCallback );
    g_map_puber = nh.advertise<nav_msgs::OccupancyGrid> ( "mapping/grid_map", 1 );
    
    ros::spin();

    g_map->saveMap(map_image_save_dir, map_config_save_dir);
    
    std::cout << std::endl << "Map saved in " << map_image_save_dir << std::endl;
}

void odometryCallback ( const nav_msgs::OdometryConstPtr& odom )
{
    double x = odom->pose.pose.position.x;
    double y = odom->pose.pose.position.y;
    double theta = tf::getYaw ( odom->pose.pose.orientation );
    g_robot_pose = Pose2d ( x, y, theta );
}

void laserCallback ( const sensor_msgs::LaserScanConstPtr& scan )
{
    g_gmapper->updateMap ( scan, g_robot_pose );
    
    /* map shown with opencv */
    // cv::Mat map = g_map->toCvMat();
    // cv::imshow ( "map", map );
    // cv::waitKey ( 1 );
    
    nav_msgs::OccupancyGrid occ_map;
    g_map->toRosOccGridMap ( "odom", occ_map );
    g_map_puber.publish ( occ_map );
}

