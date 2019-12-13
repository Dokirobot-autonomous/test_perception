
sbag_android_imu=select(bag_android,'Time',term1,'Topic','/android/galaxy1/imu');
msgs_android_imu=readMessages(sbag_android_imu);
s=size(msgs_android_imu);
accel=zeros(s(1),3);
accel_g=zeros(s(1),3);
for i=1:s(1)
    
    disp(i);
    
    msg=msgs_android_imu{i};
    
    quat=[msg.Orientation.W msg.Orientation.X msg.Orientation.Y msg.Orientation.Z];
    rotm=quat2rotm(quat);
    tmp=[msg.LinearAcceleration.X msg.LinearAcceleration.Y msg.LinearAcceleration.Z];

    accel(i,:)=tmp;
    accel_g(i,:)=rotm*tmp';
    
end
time_imu=sbag_android_imu.MessageList{:,1};

% boolen_imu_term1=time_imu>term1(1) & time_imu<term1(2);
% time_imu_term1=time_imu(boolen_imu_term1,:);
% accel_g_term1=accel_g(boolen_imu_term1,:);

figure(1)
plot(time_imu,accel_g(:,3));
