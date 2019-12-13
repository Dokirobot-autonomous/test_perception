sbag_detection_mks_detector=select(bag_detection,'Time',term1,'Topic','/mks/detection/lidar_detector/objects');
msgs_mks_detector=readMessages(sbag_detection_mks_detector);

time_mks_detector=sbag_detection_mks_detector.MessageList{:,1};
boolean_mks_detector_term1=time_mks_detector>term1(1) & time_mks_detector<term1(2);

s=length(msgs_mks_detector);
position_pedestrian=zeros(s,2);
msgs_mks_pedestrian=[];
for i=1:s
    msgs_mks_pedestrian=[msgs_mks_pedestrian;rosmessage('autoware_msgs/DetectedObject')];
    objects=msgs_mks_detector{i}.Objects;
    for j=1:length(objects)
        if strcmp(objects(j).Label,'pedestrian')
            position_pedestrian(i,1)=objects(j).Pose.Position.X;
            position_pedestrian(i,2)=objects(j).Pose.Position.Y;
            msgs_mks_pedestrian(end)=objects(j);
        end
    end  
end

time_mks_detector_term1=time_mks_detector(boolean_mks_detector_term1);
position_pedestrian_term1=position_pedestrian(boolean_mks_detector_term1,:);

figure(4);
scatter3(position_pedestrian_term1(:,1),position_pedestrian_term1(:,2),time_mks_detector_term1);