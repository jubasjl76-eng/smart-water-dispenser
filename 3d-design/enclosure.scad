/**
 * Smart Pet Water Dispenser - 3D Enclosure Design
 * OpenSCAD script
 * 
 * Features:
 * - Water pump mount
 * - Water reservoir
 * - Ultrasonic sensor mount for level detection
 * - TDS sensor mount
 * - ESP32 microcontroller placement
 * - Removable lid
 * - Drain valve option
 */

// ============== PARAMETERS ==============
WATER_WIDTH = 120;    // mm
WATER_DEPTH = 120;    // mm
WATER_HEIGHT = 200;   // mm (taller for water column)
WALL_THICKNESS = 3;   // mm
RESERVOIR_DIAMETER = 100; // mm
PUMP_MOUNT_HOLE = 12; // mm (for typical water pump)

// ============== WATER RESERVOIR ==============
module water_reservoir() {
  difference() {
    // Outer cylinder
    cylinder(r = RESERVOIR_DIAMETER / 2, h = WATER_HEIGHT * 0.7, center = true);
    
    // Inner cavity
    cylinder(r = RESERVOIR_DIAMETER / 2 - WALL_THICKNESS, h = WATER_HEIGHT * 0.7 + 1, center = true);
  }
}

// ============== PUMP MOUNT ==============
module pump_mount() {
  // Pump base plate
  difference() {
    cube([50, 40, 8], center = true);
    
    // Mounting holes (4 corners)
    translate([-18, -15, 0]) cylinder(r = 2, h = 10, center = true);
    translate([18, -15, 0]) cylinder(r = 2, h = 10, center = true);
    translate([-18, 15, 0]) cylinder(r = 2, h = 10, center = true);
    translate([18, 15, 0]) cylinder(r = 2, h = 10, center = true);
    
    // Center hole for pump
    cylinder(r = PUMP_MOUNT_HOLE / 2, h = 10, center = true);
  }
  
  // Outlet pipe
  translate([0, 25, 0]) {
    difference() {
      cylinder(r = 8, h = 20, center = true);
      cylinder(r = 6, h = 25, center = true);
    }
  }
}

// ============== WATER LEVEL SENSOR MOUNT ==============
module sensor_mount() {
  // Bracket for HC-SR04
  difference() {
    cube([25, 20, 10], center = true);
    // Sensor cutout
    translate([0, 0, 5]) cube([18, 15, 10], center = true);
  }
  
  // Mounting tabs
  translate([-10, -12, 0]) cube([3, 4, 3]);
  translate([10, -12, 0]) cube([3, 4, 3]);
}

// ============== TDS SENSOR HOUSING ==============
module tds_sensor_mount() {
  // Housing for TDS probe
  difference() {
    cube([15, 15, 30], center = true);
    // Probe hole (10mm diameter)
    cylinder(r = 5, h = 35, center = true);
  }
}

// ============== ESP32 MOUNT ==============
module esp32_mount() {
  difference() {
    cube([55, 35, 3], center = true);
    translate([-22, -14, 0]) cylinder(r = 1.5, h = 5, center = true);
    translate([22, -14, 0]) cylinder(r = 1.5, h = 5, center = true);
    translate([-22, 14, 0]) cylinder(r = 1.5, h = 5, center = true);
    translate([22, 14, 0]) cylinder(r = 1.5, h = 5, center = true);
  }
}

// ============== MAIN ENCLOSURE ==============
module main_enclosure() {
  difference() {
    // Outer shell
    cube([WATER_WIDTH, WATER_DEPTH, WATER_HEIGHT], center = true);
    
    // Inner cavity
    translate([0, 0, WALL_THICKNESS]) {
      cube([
        WATER_WIDTH - 2 * WALL_THICKNESS,
        WATER_DEPTH - 2 * WALL_THICKNESS,
        WATER_HEIGHT - WALL_THICKNESS
      ], center = true);
    }
    
    // Lid opening
    translate([0, 0, WATER_HEIGHT / 2 - 5]) {
      cube([WATER_WIDTH - 10, WATER_DEPTH - 10, 10], center = true);
    }
    
    // Reservoir opening (top)
    translate([0, 0, WATER_HEIGHT * 0.15]) {
      cylinder(r = RESERVOIR_DIAMETER / 2 - WALL_THICKNESS, h = 20, center = true);
    }
    
    // Drain valve opening (bottom front)
    translate([0, -WATER_DEPTH / 2 + WALL_THICKNESS, -WATER_HEIGHT / 3]) {
      cube([15, 10, 15], center = true);
    }
    
    // Ventilation
    for (i = [-30, 0, 30]) {
      for (j = [-40, -20, 0, 20, 40]) {
        translate([i, -WATER_DEPTH / 2 + WALL_THICKNESS / 2, j]) {
          rotate([90, 0, 0]) cylinder(r = 2, h = WALL_THICKNESS + 1);
        }
      }
    }
    
    // Cable grommets
    translate([-40, WATER_DEPTH / 2, -50]) cylinder(r = 5, h = WALL_THICKNESS + 1);
    translate([40, WATER_DEPTH / 2, -50]) cylinder(r = 5, h = WALL_THICKNESS + 1);
  }
}

// ============== REMOVABLE LID ==============
module removable_lid() {
  difference() {
    cube([WATER_WIDTH - 8, WATER_DEPTH - 8, WALL_THICKNESS], center = true);
    translate([0, 0, 0]) cube([30, 10, WALL_THICKNESS + 2], center = true);
  }
  
  // Handle
  translate([0, 0, WALL_THICKNESS / 2 + 3]) {
    difference() {
      cube([30, 15, 6], center = true);
      cube([24, 9, 10], center = true);
    }
  }
  
  // Sealing ring groove
  translate([0, 0, -WALL_THICKNESS / 2]) {
    difference() {
      cube([WATER_WIDTH - 16, WATER_DEPTH - 16, 3], center = true);
      cube([WATER_WIDTH - 24, WATER_DEPTH - 24, 10], center = true);
    }
  }
}

// ============== ASSEMBLY VIEW ==============
module assembly() {
  // Main enclosure
  color([0.6, 0.7, 0.9]) main_enclosure();
  
  // Lid
  color([0.5, 0.6, 0.8]) translate([0, 0, WATER_HEIGHT / 2 + WALL_THICKNESS / 2]) {
    removable_lid();
  }
  
  // Reservoir (inside)
  color([0.7, 0.8, 1.0, 0.5]) translate([0, 0, 0]) {
    water_reservoir();
  }
  
  // Pump mount
  color([0.3, 0.3, 0.3]) translate([0, -WATER_DEPTH / 4, -WATER_HEIGHT / 4]) {
    pump_mount();
  }
  
  // ESP32 mount
  color([0.2, 0.2, 0.5]) translate([0, -WATER_DEPTH / 4 + 30, 0]) {
    esp32_mount();
  }
  
  // Sensor mount
  color([0.2, 0.2, 0.2]) translate([30, WATER_DEPTH / 4 - 10, 0]) {
    sensor_mount();
  }
  
  // TDS sensor
  color([0.4, 0.4, 0.4]) translate([-30, WATER_DEPTH / 4 - 10, 0]) {
    tds_sensor_mount();
  }
}

// Render the assembly
assembly();

// ============== DIMENSIONS ==============
echo("Water Dispenser Dimensions:");
echo(str("Width: ", WATER_WIDTH, "mm"));
echo(str("Depth: ", WATER_DEPTH, "mm"));
echo(str("Height: ", WATER_HEIGHT, "mm"));
echo(str("Reservoir Diameter: ", RESERVOIR_DIAMETER, "mm"));
