%This program calculates the energy of a microwave with a demand of 0
clear;
Poff = 200;%standby power in W
pf   = 0.95;%power factor



E(1)=0+0*j;%Energy is initialized at 0 kWh
for n=2:11
   E(n)=E(n-1)+Poff/(3600*1000)+j*Poff*sin(acos(pf))/(3600*pf*1000);
end

f=fopen('test_microwave_energy_accumulation.player','w');
fprintf(f,'2000-01-01 0:00:01,%6.12f+%6.12fj\n',real(E(1)),imag(E(1)));
for i=2:length(E)
    fprintf(f,'+1s,%6.12f+%6.12fj\n',real(E(i)),imag(E(i)));
end
fclose(f);