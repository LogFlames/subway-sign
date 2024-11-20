box_depth = 50;
led_depth = 15;
led_height = 160;
led_width = 320;
thinn_wall = 2;
wall = 6;

krans_depth = 4;
krans_width = 15;

middle_overlap = 10;
overlap_margin = 10;

left_side = false;

bolt_indent = 8.6;

backwall_depth = 2;
backwall_thickness = 2;
backwall_margin = 2;

difference() {
    union() {
        difference() {
            union() {
                intersection() {
                    union() {
                        difference() { // walls
                            cube([led_width + 2 * wall, led_height + 2 * wall, box_depth]);
                            union() {
                                translate([wall, wall, -0.5]) {
                                    cube([led_width, led_height, box_depth + 1]);
                                }
                                translate([thinn_wall, thinn_wall, box_depth - led_depth]) {
                                    cube([led_width + 2 * (wall - thinn_wall), led_height + 2 * (wall - thinn_wall), led_depth + 1]);
                                }
                            }
                        }

                        difference() { // krans
                            translate([wall, wall, box_depth - led_depth - krans_depth]) {
                                cube([led_width, led_height, krans_depth]);
                            }
                            translate([wall + krans_width, wall + krans_width, box_depth - led_depth - krans_depth - 0.5]) {
                                cube([led_width - 2 * krans_width, led_height - 2 * krans_width, krans_depth + 1]);
                            }
                        }
                    }
                    cube([led_width / 2 + thinn_wall, 10000, 10000]); // cut away half
                }

                translate([led_width / 2 + thinn_wall, led_height + wall - krans_width, box_depth - led_depth - krans_depth]) {
                    cube([middle_overlap, krans_width, krans_depth]); // utstick overlap
                }
            }
            translate([led_width / 2 + thinn_wall - middle_overlap, wall, box_depth - led_depth - krans_depth - 0.5]) {
                cube([middle_overlap + 1, krans_width + 1, krans_depth + 1]); // to bort overlap
            }
        }
        translate([led_width / 2 + thinn_wall - middle_overlap - overlap_margin, thinn_wall, box_depth - led_depth]) {
            cube([middle_overlap * 2 + overlap_margin, krans_width + wall - thinn_wall, krans_depth + 1]); // overlap överliggande
        }
    }
    union() { // bolt holes
        translate([thinn_wall + bolt_indent, thinn_wall + bolt_indent, -10]) {
            cylinder(10000, 3, true);
        }
        translate([led_width / 2 + thinn_wall, thinn_wall + bolt_indent, -10]) {
            cylinder(10000, 3, true);
        }
        translate([thinn_wall + bolt_indent, led_height + 2 * wall - thinn_wall - bolt_indent, -10]) {
            cylinder(10000, 3, true);
        }
        translate([led_width / 2 + thinn_wall, led_height + 2 * wall - thinn_wall - bolt_indent, -10]) {
            cylinder(10000, 3, true);
        }
    }
    union() { // backwall
        translate([wall - backwall_depth, wall - backwall_depth, backwall_margin]) {
            cube([led_width + 2 * backwall_depth, led_height + 2 * backwall_depth, backwall_thickness]);
        }
        if (left_side) {
            translate([wall, led_height + wall - 0.5,-1]) {
                cube([10000, wall + 1, backwall_thickness + backwall_margin + 1]);
                translate([-backwall_depth, 0, backwall_margin + 1]) {
                    cube([10000, wall + 1, backwall_thickness]);
                }
            }
        } else {
            translate([wall, -0.5,-1]) {
                cube([10000, wall + 1, backwall_thickness + backwall_margin + 1]);
                translate([-backwall_depth, 0, backwall_margin + 1]) {
                    cube([10000, wall + 1, backwall_thickness]);
                }
            }
        }
    }
}