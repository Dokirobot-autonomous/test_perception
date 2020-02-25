std_msgs/Header header
std_msgs/Float64 existence_probability # probability of object existence
std_msgs/String[] label_candidates # possible labels of the object
std_msgs/Float64MultiArray label_probabilities # probabilities of each label candidate
std_msgs/Float64MultiArray mean       # [x,y,v,yaw,yawd]
std_msgs/Float64MultiArray covariance # [xx,xy,xy,...,yawdv,yawdyaw,yawdaywd]
