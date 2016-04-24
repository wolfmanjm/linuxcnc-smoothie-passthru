# linuxcnc-smoothie-passthru
Passthru program for Smoothie that reads linuxcnc parallel port

Just edit main.cpp to set currents required.

Parallel port mappings are as follows....



  PP2  P0_16  // xpulse	   
  PP3  P0_18  // Xdir	 	   
  PP4  P0_26  // ypulse	   
  PP5  P0_25  // Ydir	 	   
  PP6  P2_11  // zpulse	   
  PP7  P0_17  // Zdir	 	   
  PP8  P2_6   // apulse	   
  PP9  P2_8   // Adir	 	   
  PP14 P0_15  // enable	   
  PP11 P1_30  // probe optiona
  PP12 P3_26  // xlimit	   
  PP13 P3_25  // ylimit	   
  PP15 P1_23  // zlimit	   
  PP16 XX     // Bpulse	   
  PP17 P4_29  // spindle/relay
 
