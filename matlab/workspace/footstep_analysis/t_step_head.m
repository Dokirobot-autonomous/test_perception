boolean_step=false(s,1);
boolean_head=false(s,1);

init=true;


for i=2:(length(accel_g_lpf)-1)
    
    if accel_g_lpf(i,3)>accel_g_lpf(i-1,3) && accel_g_lpf(i,3)>accel_g_lpf(i+1,3)
        boolean_step(i)=true;
        init=false;
    elseif accel_g_lpf(i,3)<accel_g_lpf(i-1,3) && accel_g_lpf(i,3)<accel_g_lpf(i+1,3)
        if init==false
            boolean_head(i)=true;
        end
    end
    
end

accel_g_lpf_step=accel_g_lpf(boolean_step,:);
accel_g_lpf_head=accel_g_lpf(boolean_head,:);
time_imu_step=time_imu(boolean_step);

accel_g_lpf_pp=accel_g_lpf_step-accel_g_lpf_head;

figure(3)
scatter(time_imu_step,accel_g_lpf_pp(:,3))

figure(7)
accel_g_lpf_pp_4sqrt=nthroot(abs(accel_g_lpf_pp),4);
scatter(time_imu_step,accel_g_lpf_pp_4sqrt(:,3))
