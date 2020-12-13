$fn=100;
h=2.1;
coil_h=1.75;
coil_d1=29.5;
coil_d2=39;
b=58.4;
l=62.8;
cooling_holes=1;

difference()
{
    union()
    {
        linear_extrude(h) square([b,l],true);
        translate([-b/2,0,2]) rotate([0,90,0]) linear_extrude(b) circle(d=4);
    }
    union()
    {
        translate([(b-coil_d2)/2-3,0,h-coil_h]) difference()
        {
          linear_extrude(coil_h+5) circle(d=coil_d2);
          linear_extrude(coil_h+5) circle(d=coil_d1);
        }
        if( cooling_holes )
        {
            translate([(b-coil_d2)/2-3,0,0]) linear_extrude(coil_h+5) circle(d=coil_d1-10);
            
            xxx=5;
            yyy=10;
            translate([-b/2+6,-l/2+6+2*yyy,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6,-l/2+6+yyy,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+1*yyy,-l/2+6+yyy,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6,-l/2+6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+1*yyy,-l/2+6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+2*yyy,-l/2+6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+3*yyy,-l/2+6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+4*yyy,-l/2+6,0]) linear_extrude(coil_h+1) circle(d=xxx);

            translate([-b/2+6,l/2-6-2*yyy,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6,l/2-6-yyy,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+1*yyy,l/2-6-yyy,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6,l/2-6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+1*yyy,l/2-6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+2*yyy,l/2-6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+3*yyy,l/2-6,0]) linear_extrude(coil_h+1) circle(d=xxx);
            translate([-b/2+6+4*yyy,l/2-6,0]) linear_extrude(coil_h+1) circle(d=xxx);
        }
        translate([-b/2+20,l/4+7-1,h-coil_h]) linear_extrude(coil_h+1) square([30,14],true);
    }
}


translate([b/2-1,-27,0]) union()
{
    linear_extrude(4) square([2,4],true);
    translate([2,0,2]) linear_extrude(2) square([2+1,4],true);
}
translate([b/2-1,0,0]) union()
{
    linear_extrude(4) square([2,4],true);
    translate([2,0,2]) linear_extrude(2) square([2+1,4],true);
}
translate([b/2-1,+27,0]) union()
{
    linear_extrude(4) square([2,4],true);
    translate([2,0,2]) linear_extrude(2) square([2+1,4],true);
}

translate([-b/2-1-1.5/2,0,0]) union()
{
    difference()
    {
      //linear_extrude(9+2) square([3+1.5,8],true);
      translate([-0.5,0,0]) linear_extrude(9+2) square([3+1.5+1,8],true);
      translate([4.8,0,4]) rotate([0,-20,0]) linear_extrude(9+2) square([3+1.5,8],true);
      translate([-1.5,0,-2.3]) rotate([0,-20,0]) linear_extrude(9+2) square([3+1.5,8],true);
    }
    //translate([-1.5-1.5/2,4,9+1]) rotate([90,0,0]) linear_extrude(8) circle(d=3);
    translate([-1.5-1.5/2,4,9+1+1]) rotate([90,0,0]) linear_extrude(8) circle(d=5);
}
