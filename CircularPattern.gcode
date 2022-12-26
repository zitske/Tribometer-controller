G94; Feed per minute
X0.0000Y0.0000F2400.0; Go to position 0 with Feed
G00 X72.5000 Y-25.5000 Z5.0000; Fast positioning 
G1 Z-1.0000 F900.0; Linear interpolation
G2 X82.5000 Y-15.5000 I10.0000 J0.0000 F2400.0
G2 X92.5000 Y-25.5000 I0.0000 J-10.0000
G2 X82.5000 Y-35.5000 I-10.0000 J0.0000
G2 X72.5000 Y-25.5000 I0.0000 J10.0000
G00 Z5.0000
G00 Z20.0000
G00 X0.0000 Y0.0000

(
----Variables----
cc (clock wise) = bool
radius = int
middleX = double
middleY = double
times = int

Start
G94
X0.0000 Y0.0000 F2400.0;
G00 X$middleX Y$middleY;

Loop(times)
G1 X$(middleX+radius) F2400.0
if(cc){G2}else{G3} X$radiusX I$if(cc){radius}else{-radius} J0

Finish
G00 X0.0000 Y0.0000
)
