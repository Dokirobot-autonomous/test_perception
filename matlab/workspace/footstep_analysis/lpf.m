ALPHA=0.95;

s=length(time_imu);
accel_g_lpf=zeros(s,3);
accel_g_lpf(1,:)=accel_g(1,:);

for i=2:s(1)
    
    accel_g_lpf(i,:)=ALPHA*accel_g_lpf(i-1,:)+(1-ALPHA)*accel_g(i,:);

end

figure(2)
plot(time_imu,accel_g_lpf(:,3));