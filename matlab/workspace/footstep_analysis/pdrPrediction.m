function out = pdrPrediction(in,app,omega)

out=zeros(5,1);

out(1)=in(1)+(in(4)*nthroot(app,4)+in(5))*cos(in(3));
out(2)=in(2)+(in(4)*nthroot(app,4)+in(5))*sin(in(3));
out(3)=in(3)+omega;
out(4)=in(4);
out(5)=in(5);

end