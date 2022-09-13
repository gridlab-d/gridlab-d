object regulator_configuration {
  name "rcon_reg4";
  connect_type WYE_WYE;
	Control MANUAL; // LINE_DROP_COMP;
  // use these for OUTPUT_VOLTAGE mode
  // band_center 2480.000000;
  // band_width 40.000000;
  // use these for LINE_DROP_COMP mode
  // band_center 124.000000;
  // band_width 2.000000;
  // transducer ratios only apply to LINE_DROP_COMP mode
  current_transducer_ratio 1500.000000;
  power_transducer_ratio 20.000000;
  dwell_time 15.000000;
  raise_taps 16;
  lower_taps 16;
  regulation 0.100000;
  Type B;
  compensator_r_setting_A 0.600000;
  compensator_x_setting_A 1.300000;
  // comment out the manual tap setting if using automatic control
  tap_pos_A 10;
  compensator_r_setting_B 1.400000;
  compensator_x_setting_B 2.600000;
  // comment out the manual tap setting if using automatic control
  tap_pos_B 4;
  compensator_r_setting_C 0.200000;
  compensator_x_setting_C 1.400000;
  // comment out the manual tap setting if using automatic control
  tap_pos_C 6;
}
object regulator {
  name "reg_reg4";
  from "n160";
  to "n160r";
  phases ABC;
  configuration "rcon_reg4";
}
object regulator_configuration {
  name "rcon_reg3";
  connect_type WYE_WYE;
	Control MANUAL; // LINE_DROP_COMP;
  // use these for OUTPUT_VOLTAGE mode
  // band_center 2400.000000;
  // band_width 20.000000;
  // use these for LINE_DROP_COMP mode
  // band_center 120.000000;
  // band_width 1.000000;
  // transducer ratios only apply to LINE_DROP_COMP mode
  current_transducer_ratio 250.000000;
  power_transducer_ratio 20.000000;
  dwell_time 15.000000;
  raise_taps 16;
  lower_taps 16;
  regulation 0.100000;
  Type B;
  compensator_r_setting_A 0.400000;
  compensator_x_setting_A 0.400000;
  // comment out the manual tap setting if using automatic control
  tap_pos_A 2;
  compensator_r_setting_C 0.400000;
  compensator_x_setting_C 0.400000;
  // comment out the manual tap setting if using automatic control
  tap_pos_C 0;
}
object regulator {
  name "reg_reg3";
  from "n25";
  to "n25r";
  phases AC;
  configuration "rcon_reg3";
}
object regulator_configuration {
  name "rcon_reg2";
  connect_type WYE_WYE;
	Control MANUAL; // LINE_DROP_COMP;
  // use these for OUTPUT_VOLTAGE mode
  // band_center 2400.000000;
  // band_width 40.000000;
  // use these for LINE_DROP_COMP mode
  // band_center 120.000000;
  // band_width 2.000000;
  // transducer ratios only apply to LINE_DROP_COMP mode
  current_transducer_ratio 250.000000;
  power_transducer_ratio 20.000000;
  dwell_time 15.000000;
  raise_taps 16;
  lower_taps 16;
  regulation 0.100000;
  Type B;
  compensator_r_setting_A 0.400000;
  compensator_x_setting_A 0.400000;
  // comment out the manual tap setting if using automatic control
  tap_pos_A 0;
}
object regulator {
  name "reg_reg2";
  from "n9";
  to "n9r";
  phases A;
  configuration "rcon_reg2";
}
object line_configuration {
  name "lcon_11_C";
  z33 1.32920+1.34750j;
  c33 11.9873;
}
object line_configuration {
  name "lcon_1_ABC";
  z11 0.457600+1.07800j;
  c11 15.0567;
  z12 0.153500+0.384900j;
  c12 -1.8519;
  z13 0.158000+0.423600j;
  c13 -3.0888;
  z21 0.156000+0.501700j;
  c21 -4.8590;
  z22 0.466600+1.04820j;
  c22 15.8641;
  z23 0.158000+0.423600j;
  c23 -3.0888;
  z31 0.153500+0.384900j;
  c31 -1.8519;
  z32 0.158000+0.423600j;
  c32 -3.0888;
  z33 0.461500+1.06510j;
  c33 14.3156;
}
object line_configuration {
  name "lcon_12_ABC";
  z11 1.52090+0.752100j;
  c11 178.3093;
  z12 0.492400+0.215700j;
  c12 0.0000;
  z13 0.519800+0.277500j;
  c13 0.0000;
  z21 0.519800+0.277500j;
  c21 0.0000;
  z22 1.53290+0.716200j;
  c22 178.3093;
  z23 0.519800+0.277500j;
  c23 0.0000;
  z31 0.492400+0.215700j;
  c31 0.0000;
  z32 0.519800+0.277500j;
  c32 0.0000;
  z33 1.52090+0.752100j;
  c33 178.3093;
}
object line_configuration {
  name "lcon_2_ABC";
  z11 0.466600+1.04820j;
  c11 15.8641;
  z12 0.156000+0.501700j;
  c12 -4.8590;
  z13 0.153500+0.384900j;
  c13 -1.8519;
  z21 0.158000+0.423600j;
  c21 -3.0888;
  z22 0.461500+1.06510j;
  c22 14.3156;
  z23 0.153500+0.384900j;
  c23 -1.8519;
  z31 0.156000+0.501700j;
  c31 -4.8590;
  z32 0.153500+0.384900j;
  c32 -1.8519;
  z33 0.457600+1.07800j;
  c33 15.0567;
}
object line_configuration {
  name "lcon_3_ABC";
  z11 0.461500+1.06510j;
  c11 14.3156;
  z12 0.158000+0.423600j;
  c12 -3.0888;
  z13 0.156000+0.501700j;
  c13 -4.8590;
  z21 0.153500+0.384900j;
  c21 -1.8519;
  z22 0.457600+1.07800j;
  c22 15.0567;
  z23 0.156000+0.501700j;
  c23 -4.8590;
  z31 0.158000+0.423600j;
  c31 -3.0888;
  z32 0.156000+0.501700j;
  c32 -4.8590;
  z33 0.466600+1.04820j;
  c33 15.8641;
}
object line_configuration {
  name "lcon_4_ABC";
  z11 0.461500+1.06510j;
  c11 14.3156;
  z12 0.153500+0.384900j;
  c12 -1.8519;
  z13 0.156000+0.501700j;
  c13 -4.8590;
  z21 0.158000+0.423600j;
  c21 3.0888;
  z22 0.466600+1.04820j;
  c22 15.8641;
  z23 0.156000+0.501700j;
  c23 -4.8590;
  z31 0.153500+0.384900j;
  c31 -1.8519;
  z32 0.156000+0.501700j;
  c32 -4.8590;
  z33 0.457600+1.07800j;
  c33 15.0567;
}
object line_configuration {
  name "lcon_5_ABC";
  z11 0.466600+1.04820j;
  c11 15.8641;
  z12 0.158000+0.423600j;
  c12 -3.0888;
  z13 0.153500+0.384900j;
  c13 -1.8519;
  z21 0.156000+0.501700j;
  c21 -4.8590;
  z22 0.457600+1.07800j;
  c22 15.0567;
  z23 0.153500+0.384900j;
  c23 -1.8519;
  z31 0.158000+0.423600j;
  c31 -3.0888;
  z32 0.153500+0.384900j;
  c32 -1.8519;
  z33 0.461500+1.06510j;
  c33 14.3156;
}
object line_configuration {
  name "lcon_6_ABC";
  z11 0.457600+1.07800j;
  c11 15.0567;
  z12 0.156000+0.501700j;
  c12 -4.8590;
  z13 0.158000+0.423600j;
  c13 -3.0888;
  z21 0.153500+0.384900j;
  c21 -1.8519;
  z22 0.461500+1.06510j;
  c22 14.3156;
  z23 0.158000+0.423600j;
  c23 -3.0888;
  z31 0.156000+0.501700j;
  c31 -4.8590;
  z32 0.158000+0.423600j;
  c32 -3.0888;
  z33 0.466600+1.04820j;
  c33 15.8641;
}
object line_configuration {
  name "lcon_7_AC";
  z11 0.457600+1.07800j;
  c11 13.5684;
  z13 0.153500+0.384900j;
  c13 -2.7981;
  z31 0.153500+0.384900j;
  c31 -2.7981;
  z33 0.461500+1.06510j;
  c33 13.7143;
}
object line_configuration {
  name "lcon_8_AB";
  z11 0.457600+1.07800j;
  c11 13.5684;
  z12 0.153500+0.384900j;
  c12 -2.7981;
  z21 0.153500+0.384900j;
  c21 -2.7981;
  z22 0.461500+1.06510j;
  c22 13.7143;
}
object line_configuration {
  name "lcon_9_A";
  z11 1.32920+1.34750j;
  c11 11.9873;
}
object line_configuration {
  name "lcon_10_B";
  z22 1.32920+1.34750j;
  c22 11.9873;
}
object capacitor {
  name "cap_c88a";
  parent "n88";
  phases AN;
  phases_connected AN;
  nominal_voltage 2401.78;
  cap_nominal_voltage 2402.00;
  capacitor_A 50000.00;
  switchA CLOSED;
}
object capacitor {
  name "cap_c90b";
  parent "n90";
  phases BN;
  phases_connected BN;
  nominal_voltage 2401.78;
  cap_nominal_voltage 2402.00;
  capacitor_B 50000.00;
  switchB CLOSED;
}
object capacitor {
  name "cap_c92c";
  parent "n92";
  phases CN;
  phases_connected CN;
  nominal_voltage 2401.78;
  cap_nominal_voltage 2402.00;
  capacitor_C 50000.00;
  switchC CLOSED;
}
object capacitor {
  name "cap_c83";
  parent "n83";
  phases ABCN;
  phases_connected ABCN;
  nominal_voltage 2401.78;
  cap_nominal_voltage 2402.00;
  capacitor_A 200000.00;
  switchA CLOSED;
  capacitor_B 200000.00;
  switchB CLOSED;
  capacitor_C 200000.00;
  switchC CLOSED;
}
object overhead_line {
  name "line_l50";
  from "n50";
  to "n51";
  phases ABC;
  length 250.0046;
  configuration "lcon_4_ABC";
}
object overhead_line {
  name "line_l52";
  from "n52";
  to "n53";
  phases ABC;
  length 200.0037;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l51";
  from "n51";
  to "n151";
  phases ABC;
  length 500.0092;
  configuration "lcon_4_ABC";
}
object overhead_line {
  name "line_l54";
  from "n54";
  to "n55";
  phases ABC;
  length 275.0050;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l53";
  from "n53";
  to "n54";
  phases ABC;
  length 125.0023;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l56";
  from "n55";
  to "n56";
  phases ABC;
  length 275.0050;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l55";
  from "n54";
  to "n57";
  phases ABC;
  length 350.0064;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l58";
  from "n57";
  to "n60";
  phases ABC;
  length 750.0137;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l57";
  from "n57";
  to "n58";
  phases B;
  length 250.0046;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l59";
  from "n58";
  to "n59";
  phases B;
  length 250.0046;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l111";
  from "n110";
  to "n112";
  phases A;
  length 125.0023;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l110";
  from "n110";
  to "n111";
  phases A;
  length 575.0105;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l113";
  from "n113";
  to "n114";
  phases A;
  length 325.0060;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l112";
  from "n112";
  to "n113";
  phases A;
  length 525.0096;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l115";
  from "n149";
  to "n1";
  phases ABC;
  length 400.0073;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l114";
  from "n135";
  to "n35";
  phases ABC;
  length 375.0069;
  configuration "lcon_4_ABC";
}
object overhead_line {
  name "line_l117";
  from "n160r";
  to "n67";
  phases ABC;
  length 350.0064;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l116";
  from "n152";
  to "n52";
  phases ABC;
  length 400.0073;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l118";
  from "n197";
  to "n101";
  phases ABC;
  length 250.0046;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l61";
  from "n60";
  to "n62";
  phases ABC;
  length 250.0046;
  configuration "lcon_12_ABC";
}
object overhead_line {
  name "line_l60";
  from "n60";
  to "n61";
  phases ABC;
  length 550.0101;
  configuration "lcon_5_ABC";
}
object overhead_line {
  name "line_l63";
  from "n63";
  to "n64";
  phases ABC;
  length 350.0064;
  configuration "lcon_12_ABC";
}
object overhead_line {
  name "line_l62";
  from "n62";
  to "n63";
  phases ABC;
  length 175.0032;
  configuration "lcon_12_ABC";
}
object overhead_line {
  name "line_l65";
  from "n65";
  to "n66";
  phases ABC;
  length 325.0060;
  configuration "lcon_12_ABC";
}
object overhead_line {
  name "line_l64";
  from "n64";
  to "n65";
  phases ABC;
  length 425.0078;
  configuration "lcon_12_ABC";
}
object overhead_line {
  name "line_l67";
  from "n67";
  to "n72";
  phases ABC;
  length 275.0050;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l66";
  from "n67";
  to "n68";
  phases A;
  length 200.0037;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l69";
  from "n68";
  to "n69";
  phases A;
  length 275.0050;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l68";
  from "n67";
  to "n97";
  phases ABC;
  length 250.0046;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l100";
  from "n101";
  to "n102";
  phases C;
  length 225.0041;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l102";
  from "n102";
  to "n103";
  phases C;
  length 325.0060;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l101";
  from "n101";
  to "n105";
  phases ABC;
  length 275.0050;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l104";
  from "n105";
  to "n106";
  phases B;
  length 225.0041;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l103";
  from "n103";
  to "n104";
  phases C;
  length 700.0128;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l106";
  from "n106";
  to "n107";
  phases B;
  length 575.0105;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l105";
  from "n105";
  to "n108";
  phases ABC;
  length 325.0060;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l108";
  from "n108";
  to "n300";
  phases ABC;
  length 1000.0183;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l107";
  from "n108";
  to "n109";
  phases A;
  length 450.0082;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l109";
  from "n109";
  to "n110";
  phases A;
  length 300.0055;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l70";
  from "n69";
  to "n70";
  phases A;
  length 325.0060;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l1";
  from "n1";
  to "n2";
  phases B;
  length 175.0032;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l2";
  from "n1";
  to "n3";
  phases C;
  length 250.0046;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l72";
  from "n72";
  to "n73";
  phases C;
  length 275.0050;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l3";
  from "n1";
  to "n7";
  phases ABC;
  length 300.0055;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l71";
  from "n70";
  to "n71";
  phases A;
  length 275.0050;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l4";
  from "n3";
  to "n4";
  phases C;
  length 200.0037;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l74";
  from "n73";
  to "n74";
  phases C;
  length 350.0064;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l5";
  from "n3";
  to "n5";
  phases C;
  length 325.0060;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l73";
  from "n72";
  to "n76";
  phases ABC;
  length 200.0037;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l6";
  from "n5";
  to "n6";
  phases C;
  length 250.0046;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l76";
  from "n76";
  to "n77";
  phases ABC;
  length 400.0073;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l7";
  from "n7";
  to "n8";
  phases ABC;
  length 200.0037;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l75";
  from "n74";
  to "n75";
  phases C;
  length 400.0073;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l78";
  from "n77";
  to "n78";
  phases ABC;
  length 100.0018;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l8";
  from "n8";
  to "n12";
  phases B;
  length 225.0041;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l77";
  from "n76";
  to "n86";
  phases ABC;
  length 700.0128;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l9";
  from "n8";
  to "n9";
  phases A;
  length 225.0041;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l79";
  from "n78";
  to "n79";
  phases ABC;
  length 225.0041;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l81";
  from "n80";
  to "n81";
  phases ABC;
  length 175.0032;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l80";
  from "n78";
  to "n80";
  phases ABC;
  length 475.0087;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l83";
  from "n81";
  to "n84";
  phases C;
  length 675.0124;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l82";
  from "n81";
  to "n82";
  phases ABC;
  length 250.0046;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l85";
  from "n84";
  to "n85";
  phases C;
  length 475.0087;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l84";
  from "n82";
  to "n83";
  phases ABC;
  length 250.0046;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l87";
  from "n87";
  to "n88";
  phases A;
  length 175.0032;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l86";
  from "n86";
  to "n87";
  phases ABC;
  length 450.0082;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l89";
  from "n89";
  to "n90";
  phases B;
  length 250.0046;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l88";
  from "n87";
  to "n89";
  phases ABC;
  length 275.0050;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l90";
  from "n89";
  to "n91";
  phases ABC;
  length 225.0041;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l92";
  from "n91";
  to "n93";
  phases ABC;
  length 225.0041;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l91";
  from "n91";
  to "n92";
  phases C;
  length 300.0055;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l94";
  from "n93";
  to "n95";
  phases ABC;
  length 300.0055;
  configuration "lcon_6_ABC";
}
object overhead_line {
  name "line_l93";
  from "n93";
  to "n94";
  phases A;
  length 275.0050;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l96";
  from "n97";
  to "n98";
  phases ABC;
  length 275.0050;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l95";
  from "n95";
  to "n96";
  phases B;
  length 200.0037;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l10";
  from "n8";
  to "n13";
  phases ABC;
  length 300.0055;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l98";
  from "n99";
  to "n100";
  phases ABC;
  length 300.0055;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l97";
  from "n98";
  to "n99";
  phases ABC;
  length 550.0101;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l12";
  from "n13";
  to "n34";
  phases C;
  length 150.0027;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l11";
  from "n9r";
  to "n14";
  phases A;
  length 425.0078;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l99";
  from "n100";
  to "n450";
  phases ABC;
  length 800.0147;
  configuration "lcon_3_ABC";
}
object overhead_line {
  name "line_l14";
  from "n14";
  to "n11";
  phases A;
  length 250.0046;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l13";
  from "n13";
  to "n18";
  phases ABC;
  length 825.0151;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l16";
  from "n15";
  to "n16";
  phases C;
  length 375.0069;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l15";
  from "n14";
  to "n10";
  phases A;
  length 250.0046;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l18";
  from "n18";
  to "n19";
  phases A;
  length 250.0046;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l17";
  from "n15";
  to "n17";
  phases C;
  length 350.0064;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l19";
  from "n18";
  to "n21";
  phases ABC;
  length 300.0055;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l21";
  from "n21";
  to "n22";
  phases B;
  length 525.0096;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l20";
  from "n19";
  to "n20";
  phases A;
  length 325.0060;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l23";
  from "n23";
  to "n24";
  phases C;
  length 550.0101;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l22";
  from "n21";
  to "n23";
  phases ABC;
  length 250.0046;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l25";
  from "n25r";
  to "n26";
  phases AC;
  length 350.0064;
  configuration "lcon_7_AC";
}
object overhead_line {
  name "line_l24";
  from "n23";
  to "n25";
  phases ABC;
  length 275.0050;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l27";
  from "n26";
  to "n27";
  phases AC;
  length 275.0050;
  configuration "lcon_7_AC";
}
object overhead_line {
  name "line_l26";
  from "n25";
  to "n28";
  phases ABC;
  length 200.0037;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l29";
  from "n27";
  to "n33";
  phases A;
  length 500.0092;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l28";
  from "n26";
  to "n31";
  phases C;
  length 225.0041;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l30";
  from "n28";
  to "n29";
  phases ABC;
  length 300.0055;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l32";
  from "n30";
  to "n250";
  phases ABC;
  length 200.0037;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l31";
  from "n29";
  to "n30";
  phases ABC;
  length 350.0064;
  configuration "lcon_2_ABC";
}
object overhead_line {
  name "line_l34";
  from "n34";
  to "n15";
  phases C;
  length 100.0018;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l33";
  from "n31";
  to "n32";
  phases C;
  length 300.0055;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l36";
  from "n35";
  to "n40";
  phases ABC;
  length 250.0046;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l35";
  from "n35";
  to "n36";
  phases AB;
  length 650.0119;
  configuration "lcon_8_AB";
}
object overhead_line {
  name "line_l38";
  from "n36";
  to "n38";
  phases B;
  length 250.0046;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l37";
  from "n36";
  to "n37";
  phases A;
  length 300.0055;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l39";
  from "n38";
  to "n39";
  phases B;
  length 325.0060;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l41";
  from "n40";
  to "n42";
  phases ABC;
  length 250.0046;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l40";
  from "n40";
  to "n41";
  phases C;
  length 325.0060;
  configuration "lcon_11_C";
}
object overhead_line {
  name "line_l43";
  from "n42";
  to "n44";
  phases ABC;
  length 200.0037;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l42";
  from "n42";
  to "n43";
  phases B;
  length 500.0092;
  configuration "lcon_10_B";
}
object overhead_line {
  name "line_l45";
  from "n44";
  to "n47";
  phases ABC;
  length 250.0046;
  configuration "lcon_1_ABC";
}
object overhead_line {
  name "line_l44";
  from "n44";
  to "n45";
  phases A;
  length 200.0037;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l47";
  from "n47";
  to "n48";
  phases ABC;
  length 150.0027;
  configuration "lcon_4_ABC";
}
object overhead_line {
  name "line_l46";
  from "n45";
  to "n46";
  phases A;
  length 300.0055;
  configuration "lcon_9_A";
}
object overhead_line {
  name "line_l49";
  from "n49";
  to "n50";
  phases ABC;
  length 250.0046;
  configuration "lcon_4_ABC";
}
object overhead_line {
  name "line_l48";
  from "n47";
  to "n49";
  phases ABC;
  length 250.0046;
  configuration "lcon_4_ABC";
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw5";
  from "n97";
  to "n197";
  phases ABC;
  status CLOSED;
  phase_A_state SW5_STATE;  // schedule
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw4";
  from "n60";
  to "n160";
  phases ABC;
  status CLOSED;
  phase_A_state SW4_STATE;  // schedule
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw7";
  from "n151";
  to "n300";
  phases ABC;
  status OPEN;
  phase_A_state SW7_STATE;  // schedule
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw6";
  from "n61";
  to "n61s";
  phases ABC;
  status CLOSED;
  phase_A_state SW6_STATE;  // schedule
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw8";
  from "n54";
  to "n94";
  phases A;
  status OPEN;
  phase_A_state SW8_STATE;  // schedule
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw1";
  from "n150r";
  to "n149";
  phases ABC;
  status CLOSED;
  phase_A_state SW1_STATE;  // schedule
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw3";
  from "n18";
  to "n135";
  phases ABC;
  status CLOSED;
  phase_A_state SW3_STATE;  // schedule
}
object switch { // CIM LoadBreakSwitch
  name "swt_sw2";
  from "n13";
  to "n152";
  phases ABC;
  status CLOSED;
  phase_A_state SW2_STATE;  // schedule
}
object transformer_configuration {
  name "xcon_reg1a";
  connect_type WYE_WYE;
  primary_voltage 2401.777;
  secondary_voltage 2401.777;
  power_rating 5000.000;
  resistance 0.000001;
  reactance 0.000010;
}
object transformer {
  name "xf_reg1a";
  from "n150";
  to "n150r";
  phases ABC;
  configuration "xcon_reg1a";
  // vector group Yy;
}
object transformer_configuration {
  name "xcon_xfm1";
  connect_type DELTA_DELTA;
  primary_voltage 2401.777;
  secondary_voltage 277.128;
  power_rating 150.000;
  resistance 0.012700;
  reactance 0.027200;
}
object transformer {
  name "xf_xfm1";
  from "n61s";
  to "n610";
  phases ABC;
  configuration "xcon_xfm1";
  // vector group Dd;
}
object meter {
  name "n88";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s88a";
  parent "n88";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n89";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n197"; // downstream of SW5
  phases ABCN;
  nominal_voltage 2401.78;
  object complex_assert { // SW1-SW6 closed at beginning
    target voltage_A;
    operation REAL;
    value 2411.63;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 0:30:00';
  };
  object complex_assert { // SW1 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 1:30:00';
  };
  object complex_assert { // SW2 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 3:30:00';
  };
  object complex_assert { // SW3 open
    target voltage_A;
    operation REAL;
    value 2438.94;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 5:30:00';
  };
  object complex_assert { // SW4 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 7:30:00';
  };
  object complex_assert { // SW5 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 9:30:00';
  };
  object complex_assert { // SW6 open, disconnects no load
    target voltage_A;
    operation REAL;
    value 2411.63;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 11:30:00';
  };
  object complex_assert { // SW1-SW6 closed at end
    target voltage_A;
    operation REAL;
    value 2411.63;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 16:30:00';
  };
}
object node {
  name "n110";
  phases AN;
  nominal_voltage 2401.78;
}
object meter {
  name "n111";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s111a";
  parent "n111";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n112";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s112a";
  parent "n112";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n113";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s113a";
  parent "n113";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n114";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s114a";
  parent "n114";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n90";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s90b";
  parent "n90";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object node {
  name "n61s";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n91";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n92";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s92c";
  parent "n92";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object node {
  name "n93";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n94";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s94a";
  parent "n94";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n95";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s95b";
  parent "n95";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object meter {
  name "n96";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s96b";
  parent "n96";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object node {
  name "n97";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n10";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s10a";
  parent "n10";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n98";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s98a";
  parent "n98";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n11";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s11a";
  parent "n11";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n99";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s99b";
  parent "n99";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object meter {
  name "n12";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s12b";
  parent "n12";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object node {
  name "n13";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n14";
  phases AN;
  nominal_voltage 2401.78;
}
object node {
  name "n15";
  phases CN;
  nominal_voltage 2401.78;
}
object meter {
  name "n16";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s16c";
  parent "n16";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n17";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s17c";
  parent "n17";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object node {
  name "n18";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n19";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s19a";
  parent "n19";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n160r";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n1";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s1a";
  parent "n1";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n2";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s2b";
  parent "n2";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object node {
  name "n3";
  phases CN;
  nominal_voltage 2401.78;
}
object meter {
  name "n4";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s4c";
  parent "n4";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n5";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s5c";
  parent "n5";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object meter {
  name "n6";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s6c";
  parent "n6";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n7";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s7a";
  parent "n7";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object node {
  name "n8";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n9";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s9a";
  parent "n9";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n20";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s20a";
  parent "n20";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n21";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n22";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s22b";
  parent "n22";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object node {
  name "n23";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n24";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s24c";
  parent "n24";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object node {
  name "n25";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n26";
  phases ACN;
  nominal_voltage 2401.78;
}
object node {
  name "n27";
  phases ACN;
  nominal_voltage 2401.78;
}
object meter {
  name "n28";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s28a";
  parent "n28";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n29";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s29a";
  parent "n29";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n250";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n135";
  phases ABCN;
  nominal_voltage 2401.78;
  object complex_assert { // SW1-SW6 closed at beginning
    target voltage_A;
    operation REAL;
    value 2290.91;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 0:30:00';
  };
  object complex_assert { // SW1 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 1:30:00';
  };
  object complex_assert { // SW2 open
    target voltage_A;
    operation REAL;
    value 2332.52;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 3:30:00';
  };
  object complex_assert { // SW3 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 5:30:00';
  };
  object complex_assert { // SW4 open
    target voltage_A;
    operation REAL;
    value 2319.94;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 7:30:00';
  };
  object complex_assert { // SW5 open
    target voltage_A;
    operation REAL;
    value 2302.86;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 9:30:00';
  };
  object complex_assert { // SW6 open, disconnects no load
    target voltage_A;
    operation REAL;
    value 2290.91;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 11:30:00';
  };
  object complex_assert { // SW1-SW6 closed at end
    target voltage_A;
    operation REAL;
    value 2290.91;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 16:30:00';
  };
}
object node {
  name "n9r";
  phases AN;
  nominal_voltage 2401.78;
}
object meter {
  name "n30";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s30c";
  parent "n30";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n31";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s31c";
  parent "n31";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object meter {
  name "n32";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s32c";
  parent "n32";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object meter {
  name "n33";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s33a";
  parent "n33";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n34";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s34c";
  parent "n34";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n35";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s35a";
  parent "n35";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n36";
  phases ABN;
  nominal_voltage 2401.78;
}
object meter {
  name "n37";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s37a";
  parent "n37";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n38";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s38b";
  parent "n38";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object meter {
  name "n39";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s39b";
  parent "n39";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object node {
  name "n150r"; // upstream of SW1
  phases ABCN;
  nominal_voltage 2401.78;
  object complex_assert { // SW1-SW6 closed at beginning
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 0:30:00';
  };
  object complex_assert { // SW1 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 1:30:00';
  };
  object complex_assert { // SW2 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 3:30:00';
  };
  object complex_assert { // SW3 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 5:30:00';
  };
  object complex_assert { // SW4 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 7:30:00';
  };
  object complex_assert { // SW5 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 9:30:00';
  };
  object complex_assert { // SW6 open, disconnects no load
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 11:30:00';
  };
  object complex_assert { // SW1-SW6 closed at end
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 16:30:00';
  };
}
object node {
  name "n300";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n149"; // downstream of SW1
  phases ABCN;
  nominal_voltage 2401.78;
  object complex_assert { // SW1-SW6 closed at beginning
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 0:30:00';
  };
  object complex_assert { // SW1 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 1:30:00';
  };
  object complex_assert { // SW2 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 3:30:00';
  };
  object complex_assert { // SW3 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 5:30:00';
  };
  object complex_assert { // SW4 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 7:30:00';
  };
  object complex_assert { // SW5 open
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 9:30:00';
  };
  object complex_assert { // SW6 open, disconnects no load
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 11:30:00';
  };
  object complex_assert { // SW1-SW6 closed at end
    target voltage_A;
    operation REAL;
    value 2401.78;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 16:30:00';
  };
}
object node {
  name "n40";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n41";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s41c";
  parent "n41";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object meter {
  name "n42";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s42a";
  parent "n42";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n43";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s43b";
  parent "n43";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object node {
  name "n44";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n45";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s45a";
  parent "n45";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n46";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s46a";
  parent "n46";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n47";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s47";
  parent "n47";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 14.5725-10.4090j;
  constant_current_B -16.3007-7.41571j;
  constant_current_C 1.72815+17.8247j;
}
object meter {
  name "n48";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s48";
  parent "n48";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 29.1451-20.8179j;
  constant_current_B -32.6014-14.8314j;
  constant_current_C 3.45630+35.6493j;
}
object meter {
  name "n49";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s49b";
  parent "n49";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 14.5725-10.4090j;
  constant_current_B -32.6014-14.8314j;
  constant_current_C -0.0747330+16.7838j;
}
object substation {
  name "n150";
  bustype SWING;
  phases ABCN;
  nominal_voltage 2401.78;
  base_power 12MVA;
  power_convergence_value 100VA;
  positive_sequence_voltage ${VSOURCE};
  object complex_assert { // SW1-SW6 closed at beginning
    target distribution_load;
    operation REAL;
    value 3512730;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 0:30:00';
  };
  object complex_assert { // SW1 open
    target distribution_load;
    operation REAL;
    value 0.0;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 1:30:00';
  };
  object complex_assert { // SW2 open
    target distribution_load;
    operation REAL;
    value 1502030;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 3:30:00';
  };
  object complex_assert { // SW3 open
    target distribution_load;
    operation REAL;
    value 2778930;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 5:30:00';
  };
  object complex_assert { // SW4 open
    target distribution_load;
    operation REAL;
    value 2043960;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 7:30:00';
  };
  object complex_assert { // SW5 open
    target distribution_load;
    operation REAL;
    value 3191610;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 9:30:00';
  };
  object complex_assert { // SW6 open, disconnects no load
    target distribution_load;
    operation REAL;
    value 3512730;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 11:30:00';
  };
  object complex_assert { // SW1-SW6 closed at end
    target distribution_load;
    operation REAL;
    value 3512730;
    within 0.01e+06;
    once ONCE_TRUE;
    in '2000-01-01 16:30:00';
  };
}
object node {
  name "n151";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n152"; // downstream of SW2
  phases ABCN;
  nominal_voltage 2401.78;
  object complex_assert { // SW1-SW6 closed at beginning
    target voltage_A;
    operation REAL;
    value 2319.13;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 0:30:00';
  };
  object complex_assert { // SW1 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 1:30:00';
  };
  object complex_assert { // SW2 open
    target voltage_A;
    operation REAL;
    value 0.0;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 3:30:00';
  };
  object complex_assert { // SW3 open
    target voltage_A;
    operation REAL;
    value 2343.84;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 5:30:00';
  };
  object complex_assert { // SW4 open
    target voltage_A;
    operation REAL;
    value 2347.84;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 7:30:00';
  };
  object complex_assert { // SW5 open
    target voltage_A;
    operation REAL;
    value 2331.02;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 9:30:00';
  };
  object complex_assert { // SW6 open, disconnects no load
    target voltage_A;
    operation REAL;
    value 2319.13;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 11:30:00';
  };
  object complex_assert { // SW1-SW6 closed at end
    target voltage_A;
    operation REAL;
    value 2319.13;
    within 0.1;
    once ONCE_TRUE;
    in '2000-01-01 16:30:00';
  };
}
object node {
  name "n25r";
  phases ACN;
  nominal_voltage 2401.78;
}
object meter {
  name "n50";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s50c";
  parent "n50";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n51";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s51a";
  parent "n51";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n52";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s52a";
  parent "n52";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n53";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s53a";
  parent "n53";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n54";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n55";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s55a";
  parent "n55";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n56";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s56b";
  parent "n56";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object node {
  name "n57";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n58";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s58b";
  parent "n58";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object meter {
  name "n59";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s59b";
  parent "n59";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object node {
  name "n160"; // downstream of SW4
  phases ABCN;
  nominal_voltage 2401.78;
 object complex_assert { // SW1-SW6 closed at beginning
   target voltage_A;
   operation REAL;
   value 2271.33;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 0:30:00';
 };
 object complex_assert { // SW1 open
   target voltage_A;
   operation REAL;
   value 0.0;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 1:30:00';
 };
 object complex_assert { // SW2 open
   target voltage_A;
   operation REAL;
   value 0.0;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 3:30:00';
 };
 object complex_assert { // SW3 open
   target voltage_A;
   operation REAL;
   value 2296.8;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 5:30:00';
 };
 object complex_assert { // SW4 open
   target voltage_A;
   operation REAL;
   value 0.0;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 7:30:00';
 };
 object complex_assert { // SW5 open
   target voltage_A;
   operation REAL;
   value 2300.29;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 9:30:00';
 };
 object complex_assert { // SW6 open, disconnects no load
   target voltage_A;
   operation REAL;
   value 2271.33;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 11:30:00';
 };
 object complex_assert { // SW1-SW6 closed at end
   target voltage_A;
   operation REAL;
   value 2271.33;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 16:30:00';
 };
}
object meter {
  name "n60";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s60a";
  parent "n60";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object node {
  name "n61";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n62";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s62c";
  parent "n62";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n63";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s63a";
  parent "n63";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n64";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s64b";
  parent "n64";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -28.2336-19.7570j;
}
object meter {
  name "n65";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s65a";
  parent "n65";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 14.5725-10.4090j;
  constant_current_B -16.3007-7.41571j;
  constant_current_C 3.45630+35.6493j;
}
object meter {
  name "n66";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s66c";
  parent "n66";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_C -2.99325+34.3295j;
}
object node {
  name "n67";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n68";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s68a";
  parent "n68";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n69";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s69a";
  parent "n69";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n450";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n610"; // downstream of SW6
  phases ABCN;
  nominal_voltage 277.13;
 object complex_assert { // SW1-SW6 closed at beginning
   target voltage_A;
   operation REAL;
   value 262.077;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 0:30:00';
 };
 object complex_assert { // SW1 open
   target voltage_A;
   operation REAL;
   value 0.0;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 1:30:00';
 };
 object complex_assert { // SW2 open
   target voltage_A;
   operation REAL;
   value 0.0;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 3:30:00';
 };
 object complex_assert { // SW3 open
   target voltage_A;
   operation REAL;
   value 265.017;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 5:30:00';
 };
 object complex_assert { // SW4 open
   target voltage_A;
   operation REAL;
   value 270.118;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 7:30:00';
 };
 object complex_assert { // SW5 open
   target voltage_A;
   operation REAL;
   value 265.419;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 9:30:00';
 };
 object complex_assert { // SW6 open, disconnects no load
   target voltage_A;
   operation REAL;
   value 0.0;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 11:30:00';
 };
 object complex_assert { // SW1-SW6 closed at end
   target voltage_A;
   operation REAL;
   value 262.077;
   within 0.1;
   once ONCE_TRUE;
   in '2000-01-01 16:30:00';
 };
}
object meter {
  name "n70";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s70a";
  parent "n70";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 8.32717-4.16358j;
}
object meter {
  name "n71";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s71a";
  parent "n71";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object node {
  name "n72";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n73";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s73c";
  parent "n73";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n74";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s74c";
  parent "n74";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n75";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s75c";
  parent "n75";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n76";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s76a";
  parent "n76";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 43.7176-33.3087j;
  constant_current_B -32.6014-14.8314j;
  constant_current_C 3.45630+35.6493j;
}
object meter {
  name "n77";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s77b";
  parent "n77";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object node {
  name "n78";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n79";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s79a";
  parent "n79";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n100";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s100c";
  parent "n100";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object node {
  name "n101";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n102";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s102c";
  parent "n102";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object meter {
  name "n103";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s103c";
  parent "n103";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n104";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s104c";
  parent "n104";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object node {
  name "n105";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n106";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s106b";
  parent "n106";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object meter {
  name "n107";
  phases BN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s107b";
  parent "n107";
  phases BN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object meter {
  name "n80";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s80b";
  parent "n80";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
object node {
  name "n108";
  phases ABCN;
  nominal_voltage 2401.78;
}
object node {
  name "n81";
  phases ABCN;
  nominal_voltage 2401.78;
}
object meter {
  name "n109";
  phases AN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s109a";
  parent "n109";
  phases AN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n82";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s82a";
  parent "n82";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_A 16.6543-8.32717j;
}
object meter {
  name "n83";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s83c";
  parent "n83";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object meter {
  name "n84";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s84c";
  parent "n84";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -0.557814+9.29333j;
}
object meter {
  name "n85";
  phases CN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s85c";
  parent "n85";
  phases CN;
  nominal_voltage 2401.78;
  constant_current_C -1.11563+18.5867j;
}
object meter {
  name "n86";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s86b";
  parent "n86";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -7.76935-5.12975j;
}
object meter {
  name "n87";
  phases ABCN;
  nominal_voltage 2401.78;
}
object load {
  name "ld_s87b";
  parent "n87";
  phases ABCN;
  nominal_voltage 2401.78;
  constant_current_B -15.5387-10.2595j;
}
