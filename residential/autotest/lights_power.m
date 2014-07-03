function power = lights_power(total,pf)
    demand = [0,.1,.25,.5,.75,1];
    power=0:5;
    for k=1:6,
        load = demand(k)*total;
        Re = load*pf;
        Im = (load^2 - Re^2)^.5;
        power(k) = Re + Im*1i;
    end
end