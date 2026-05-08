// ═══════════════════════════════════════════════════════════════════
//  Wolf CHA-07 Außenfühler-Emulator — DIGIPOT Untergehäuse
//  Datei: cad/bottom_cover.scad
//  Druck: FDM, PETG oder PLA, 0,2 mm Schichthöhe, 3 Wände
// ═══════════════════════════════════════════════════════════════════
//
//  Draufsicht (von unten gesehen):
//
//          Seite D  (kurz, 32 mm)  ← QWIIC-Schlitz
//          ┌──────────────────────┐
//          │                      │
//  Seite A │                      │ Seite C
//  (lang   │                      │ (lang
//  70 mm)  │                      │  70 mm)
//          │                      │
//          └──────────────────────┘
//          Seite B  (kurz, 32 mm)  ← beide Schrauben
//
//  Schrauben: horizontal, von außen durch Seite B, je 1 mm von A und C.
//  QWIIC:     Schlitz in Seite D, Kabel läuft von innen nach außen.

// ───────────────────────────────────────────────────────────────────
//  ❶  LILYGO T-DISPLAY-S3 GREY SHELL
// ───────────────────────────────────────────────────────────────────

shell_w = 70.0;   // [mm] Außenmaß lang  (A↔C, X-Achse)
shell_d = 32.0;   // [mm] Außenmaß kurz  (B↔D, Y-Achse)

// Schrauben auf Seite B — je 1 mm von Seite A bzw. Seite C
screw_ya      =  1.0;             // [mm] Abstand von Seite A  (Y = 1)
screw_yc      =  shell_d - 1.0;  // [mm] Abstand von Seite C  (Y = 31)

// Z-Höhe der Schrauben: wird unten im Berechnungsblock gesetzt.
// Bitte am Gerät nachmessen und ggf. überschreiben.

screw_d       =  2.2;   // [mm] Schaftbohrung M2
screw_head_d  =  4.2;   // [mm] Senkkopf-Ø (M2 = 3,8 mm)
screw_head_h  =  2.0;   // [mm] Senktiefe
boss_d        =  8.0;   // [mm] Boss-Ø (gibt 2,9 mm Wand um M2-Loch)
boss_pad      =  1.5;   // [mm] Boss-Überstand über Seite-B-Außenwand

// ───────────────────────────────────────────────────────────────────
//  ❷  SOLDERED DIGIPOT 50 kΩ (MCP4018T-503)
// ───────────────────────────────────────────────────────────────────

digipot_pcb_w  = 22.0;  // [mm] Platine X
digipot_pcb_d  = 22.0;  // [mm] Platine Y
digipot_pcb_h  =  2.0;  // [mm] Platinenstärke
digipot_comp_h =  5.0;  // [mm] Bauhöhe inkl. QWIIC-Buchse
digipot_margin =  1.5;  // [mm] Luft rund um Platine

// ───────────────────────────────────────────────────────────────────
//  ❸  QWIIC-KABELSCHLITZ  — Seite D (linke Stirnseite)
// ───────────────────────────────────────────────────────────────────

qwiic_slot_w  =  7.0;  // [mm] Breite in Y (Kabel + QWIIC-Buchse)
qwiic_slot_h  =  4.5;  // [mm] Höhe  in Z
// Y-Position: zentriert in Seite D
qwiic_slot_y  =  shell_d / 2 - qwiic_slot_w / 2;
// Z-Position: 1 mm über Bodenwand
qwiic_slot_z  =  wall_t + 1;

// ───────────────────────────────────────────────────────────────────
//  ❹  GEHÄUSE-PARAMETER
// ───────────────────────────────────────────────────────────────────

wall_t    = 2.0;
overlap_h = 3.0;
fit_clear = 0.3;

// ───────────────────────────────────────────────────────────────────
//  BERECHNETE GRÖßEN
// ───────────────────────────────────────────────────────────────────

inner_h = digipot_pcb_h + digipot_comp_h + 3.0;
total_h = inner_h + overlap_h;

// Schraubenhöhe: Kragen-Mitte — ggf. nachmessen und anpassen
screw_z = inner_h + overlap_h / 2;  // [mm]  ← MESSEN

outer_w = shell_w + 2 * wall_t;
outer_d = shell_d + 2 * wall_t;

collar_inner_w = shell_w + 2 * fit_clear;
collar_inner_d = shell_d + 2 * fit_clear;

// Standoff-Koordinaten (Platine zentriert in XY)
so_x0 = (shell_w - digipot_pcb_w) / 2 + digipot_margin * 0.4;
so_y0 = (shell_d - digipot_pcb_d) / 2 + digipot_margin * 0.4;
so_x1 = so_x0 + digipot_pcb_w - digipot_margin * 0.8;
so_y1 = so_y0 + digipot_pcb_d - digipot_margin * 0.8;

// Seite B: Außenfläche bei X = shell_w + wall_t
side_b_outer = shell_w + wall_t;

$fn = 48;

module so(x, y) { translate([x, y, wall_t]) cylinder(d=2.0, h=1.2); }

// ═══════════════════════════════════════════════════════════════════
//  HAUPTKÖRPER
// ═══════════════════════════════════════════════════════════════════

difference() {

    union() {

        // ── Kasten + Kragen, innen ausgehöhlt ──
        difference() {
            union() {
                translate([-wall_t, -wall_t, 0])
                    cube([outer_w, outer_d, inner_h]);
                translate([-wall_t, -wall_t, inner_h])
                    cube([outer_w, outer_d, overlap_h]);
            }
            // Innenraum Kasten
            translate([0, 0, wall_t])
                cube([shell_w, shell_d, inner_h - wall_t + 0.01]);
            // Innenraum Kragen
            translate([-fit_clear, -fit_clear, inner_h])
                cube([collar_inner_w, collar_inner_d, overlap_h + 1]);
        }

        // ── Boss-Zylinder Seite B — beide Schrauben ──
        // Ragen über Seite-B-Außenwand nach rechts (+X) heraus.
        translate([side_b_outer, screw_ya, screw_z])
            rotate([0, 90, 0])
            cylinder(d=boss_d, h=boss_pad);

        translate([side_b_outer, screw_yc, screw_z])
            rotate([0, 90, 0])
            cylinder(d=boss_d, h=boss_pad);

        // ── Standoffs für DIGIPOT-Platine ──
        so(so_x0, so_y0);
        so(so_x1, so_y0);
        so(so_x0, so_y1);
        so(so_x1, so_y1);
    }

    // ── Schraubenlöcher Seite B (von außen nach innen, -X-Richtung) ──
    // Startpunkt liegt 1 mm hinter der Boss-Außenfläche.
    _hole_start_x = side_b_outer + boss_pad + 1;
    _hole_depth   = boss_pad + wall_t - fit_clear + 2;

    translate([_hole_start_x, screw_ya, screw_z])
        rotate([0, -90, 0])
        cylinder(d=screw_d, h=_hole_depth);
    translate([_hole_start_x, screw_ya, screw_z])
        rotate([0, -90, 0])
        cylinder(d=screw_head_d, h=screw_head_h + 1);

    translate([_hole_start_x, screw_yc, screw_z])
        rotate([0, -90, 0])
        cylinder(d=screw_d, h=_hole_depth);
    translate([_hole_start_x, screw_yc, screw_z])
        rotate([0, -90, 0])
        cylinder(d=screw_head_d, h=screw_head_h + 1);

    // ── QWIIC-Schlitz Seite D (linke Stirnwand, -X-Richtung) ──
    translate([-wall_t - 1, qwiic_slot_y, qwiic_slot_z])
        cube([wall_t + 2, qwiic_slot_w, qwiic_slot_h]);
}

// ───────────────────────────────────────────────────────────────────
//  DEBUG: LilyGo-Referenzquader (* entfernen zum Einblenden)
// ───────────────────────────────────────────────────────────────────

*color("SteelBlue", 0.25)
    translate([0, 0, inner_h])
        cube([shell_w, shell_d, 14]);

// ───────────────────────────────────────────────────────────────────
//  SCHRAUBENZUGABE
//  boss_pad + (wall_t − fit_clear) = 1,5 + 1,7 = 3,2 mm
//  Beispiel: originale M2×5 → neue M2×8
// ───────────────────────────────────────────────────────────────────
