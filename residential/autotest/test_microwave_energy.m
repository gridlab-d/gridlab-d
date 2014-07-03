%this program calculates the energy of a microwave based of of this
%equation:
%E = [(Poff)*(1-D)+(Pon)*(D)]*(dt)
%    ----------------------------- @ inverse cos(.pf)
%                pf

clear;
Poff = 10;%standby power in W
Pon  = 1000;%installed power in W
pf   = 0.95;%power factor
D    = 0.1;%demand
dt   = 600;%time cycle for demand in sec
toff = (1-D)*dt;%amount of time the microwave is off per cycle
ton  = D*dt;%amount of time the microwave is on per cycle



E(1)=0+0*j;%Energy is initialized at 0 kWh
for n=2:86400
   E(n)=E(n-1)+Pon/(3600*1000)+j*Pon*sin(acos(pf))/(3600*pf*1000);
end

f=fopen('microwave_energy.player','w');
fprintf(f,'2000-01-01 0:00:01,%6.12f+%6.12fj\n',real(E(1)),imag(E(1)));
for i=2:length(E)
    fprintf(f,'+1s,%6.12f+%6.12fj\n',real(E(i)),imag(E(i)));
end
fclose(f);