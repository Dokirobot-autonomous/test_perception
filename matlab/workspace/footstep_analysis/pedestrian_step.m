
boolean_mks_detector_step=false(length(time_mks_detector),1);

for i=1:length(time_imu_step)
    
    [tmp,idx]=min(abs(time_mks_detector-time_imu_step(i)));
    boolean_mks_detector_step(idx)=true;
    
end

boolean_mks_detector_step(end)=false;

time_mks_detector_step=time_mks_detector(boolean_mks_detector_step);
position_pedestrian_step=position_pedestrian(boolean_mks_detector_step,:);
msgs_mks_pedestrian_step=msgs_mks_pedestrian(boolean_mks_detector_step);


l=zeros(length(position_pedestrian_step)-1,1);
for i=1:(length(position_pedestrian_step))
    
    l(i)=msgs_mks_pedestrian_step(i).Dimensions.X;

    
end
l(l>1.5)=0;

figure(5)
scatter3(position_pedestrian_step(:,1),position_pedestrian_step(:,2),time_mks_detector_step(:,1));

figure(6)
scatter(time_mks_detector_step(1:end),l)