#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "map2img.h"

#define MONSTER_SIZE 16
#define WEAPON_SIZE  12
#define KEY_SIZE     12
#define AMMO_SIZE     8
#define ITEM_SIZE     8
#define LINEDEF_WIDTH 4
#define LINEDEF_SLIM  2
#define WIDTH  imginfo->width * imginfo->scale + (2 * imginfo->padding)
#define HEIGHT imginfo->height * imginfo->scale + (2 * imginfo->padding)
#define REAL_X(x) imginfo->padding + (x + imginfo->x_off)*imginfo->scale
#define REAL_Y(y) imginfo->padding + (imginfo->max_y - y)*imginfo->scale

void draw_direction(FILE* output, Thing t, double x, double y, float scale, const char* color) {
    double x2 = x;
    double y2 = y;
    double len = (MONSTER_SIZE + 4) * scale;
    // 0° -> east, 90° -> south, ...
    switch(t.angle) {
        case 270:
            y2 = y + len;
            break;
        case 315:
            x2 = x + len;
            y2 = y + len;
            break;
        case 0:
            x2 = x + len;
            break;
        case 45:
            x2 = x + len;
            y2 = y - len;
            break;
        case 90:
            y2 = y - len;
            break;
        case 134:
            x2 = x2 - len;
            y2 = y2 - len;
            break;
        case 180:
            x2 = x2 - len;
            break;
        case 225:
            x2 = x2 - len;
            y2 = y2 + len;
            break;
        default:
            break;
    }
    fprintf(output, "<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\" stroke=\"%s", x, y, x2, y2, color);
}

void output_svg(Imginfo* imginfo, Wadinfo* wadinfo, FILE* output, bool verbose, Header* wadheader) {
    int num_linedefs = wadinfo->num_linedefs;
    int num_things   = wadinfo->num_things;
    Linedef* linedefs = wadinfo->linedefs;
    Vertex*  vertexes = wadinfo->vertexes;
    Thing*   things   = wadinfo->things;
    fprintf(output, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
    fprintf(output, "<svg version=\"1.1\"");
    fprintf(output, "  xmlns=\"http://www.w3.org/2000/svg\"\n");
    fprintf(output, "  xmlns:svg=\"http://www.w3.org/2000/svg\"\n");
    fprintf(output, "  width=\"%g\" height=\"%g\">\n\n", WIDTH, HEIGHT);
    if (verbose) {
        fprintf(output, "<!--\n");
        fprintf(output, "wadfile         : %s => %s\n", wadinfo->filename, wadinfo->wad_ident);
        fprintf(output, "map             : %s\n", wadinfo->mapname);
        fprintf(output, "num_lumps       : %d\n", wadheader->num_lumps);
        fprintf(output, "num_things      : %ld\n", wadinfo->num_things);
        fprintf(output, "Linedefs        : %ld\n", wadinfo->num_linedefs);
        fprintf(output, "Vertexes        : %ld\n", wadinfo->num_vertexes);
        fprintf(output, "scaling         : %g\n", imginfo->scale);
        fprintf(output, "-->\n");
    }
    fprintf(output, "<rect width=\"%g\" height=\"%g\" fill=\"black\" />\n", WIDTH, HEIGHT);
    for (int i=0; i<num_linedefs; ++i) {
        int16_t v_index_start = linedefs[i].v_start;
        int16_t v_index_end   = linedefs[i].v_end;
        Vertex start = vertexes[v_index_start];
        Vertex end   = vertexes[v_index_end];

        if (verbose) {
            fprintf(output, "<!-- Linedef %d - Flags: %d / Special: %d -->\n", i, linedefs[i].flags, linedefs[i].special);
        }

        fprintf(output, "<line x1=\"%g\" y1=\"%g\"", REAL_X(start.x), REAL_Y(start.y));
        fprintf(output, " x2=\"%g\" y2=\"%g\"", REAL_X(end.x), REAL_Y(end.y));
        fprintf(output, " stroke=\"");
        switch(linedefs[i].special) {
            case 0:
                fprintf(output, "white");
                break;
            case 26:
            case 32:
                // Blue door
                fprintf(output, "blue");
                break;
            case 27:
            case 34:
                // Yellow door
                fprintf(output, "yellow");
                break;
            case 28:
            case 33:
                // Red door
                fprintf(output, "red");
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 29:
            case 31:
            case 42:
            case 46:
            case 50:
            case 61:
            case 63:
            case 75:
            case 76:
            case 86:
            case 90:
            case 99:
            case 103:
            case 105:
            case 106:
            case 107:
            case 108:
            case 109:
            case 110:
            case 111:
            case 112:
            case 113:
            case 114:
            case 115:
            case 116:
            case 117:
            case 118:
                // Door
                fprintf(output, "gainsboro");
                break;
            case 7:
            case 8:
                // Stairs
                fprintf(output, "orange");
                break;
            case 11:
            case 51:
            case 52:
            case 124:
                // Exit
                fprintf(output, "springgreen");
                break;
            case 39:
            case 97:
                // Teleport
                fprintf(output, "purple");
                break;
            case 62:
            case 88:
            case 120:
            case 121:
            case 122:
            case 123:
                // Lift
                fprintf(output, "saddlebrown");
                break;
            case 5:
            case 9:
            case 14:
            case 15:
            case 18:
            case 19:
            case 20:
            case 22:
            case 23:
            case 24:
            case 30:
            case 36:
            case 37:
            case 38:
            case 45:
            case 47:
            case 55:
            case 56:
            case 58:
            case 59:
            case 60:
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 69:
            case 70:
            case 71:
                // Floor
                fprintf(output, "slategrey");
                break;
            default:
                fprintf(output, "magenta");
                break;
        }
        fprintf(output, "\" stroke-width=\"");
        switch(linedefs[i].flags) {
            case 4:
                fprintf(output, "%g", LINEDEF_SLIM * imginfo->scale);
                break;
            default:
                fprintf(output, "%g", LINEDEF_WIDTH * imginfo->scale);
                break;
        }
        fprintf(output, "\"/>\n");
    }
    if (imginfo->draw_things) {
        fprintf(output, "<!-- Things: -->\n");
        for (int i=0; i<num_things; ++i) {
            if (verbose) {
                fprintf(output, "<!-- Thing type: %d / angle: %d / flags: %d -->\n", things[i].type, things[i].angle, things[i].flags);
            }
            fprintf(output, "<circle cx=\"%g\" cy=\"%g\" ", REAL_X(things[i].x_pos), REAL_Y(things[i].y_pos));
            fprintf(output, "fill=\"");
            switch(things[i].type) {
                case 68:
                case 64:
                case 3003:
                case 3005:
                case 72:
                case 16:
                case 3002:
                case 65:
                case 69:
                case 3001:
                case 3006:
                case 67:
                case 71:
                case 66:
                case 9:
                case 58:
                case 7:
                case 84:
                case 3004:
                    // Monster
                    fprintf(output, "crimson\" r=\"%g\" />\n", MONSTER_SIZE * imginfo->scale);
                    draw_direction(output, things[i], (float)REAL_X(things[i].x_pos), (float)REAL_Y(things[i].y_pos), imginfo->scale, "yellow");
                    break;
                case 2001:
                case 2002:
                case 2003:
                case 2004:
                case 2005:
                case 2006:
                case 82:
                    // Weapon
                    fprintf(output, "lightsteelblue\" r=\"%g", WEAPON_SIZE * imginfo->scale);
                    break;
                case 2008:
                case 2010:
                case 2048:
                case 2046:
                case 2049:
                case 2007:
                case 2047:
                case 17:
                    // Ammo
                    fprintf(output, "lightsteelblue\" r=\"%g", AMMO_SIZE * imginfo->scale);
                    break;
                case 2013:
                case 2014:
                case 2015:
                case 2023:
                case 2026:
                case 2022:
                case 2045:
                case 83:
                case 2024:
                case 2018:
                case 8:
                case 2012:
                case 2019:
                case 2025:
                case 2011:
                    // Artifact items and powerups
                    fprintf(output, "lavender\" r=\"%g", ITEM_SIZE * imginfo->scale);
                    break;
                case 5:
                case 40:
                    // Blue keys
                    fprintf(output, "blue\" r=\"%g", KEY_SIZE * imginfo->scale);
                    break;
                case 13:
                case 38:
                    // Red keys
                    fprintf(output, "red\" r=\"%g", KEY_SIZE * imginfo->scale);
                    break;
                case 6:
                case 39:
                    // Yellow keys
                    fprintf(output, "yellow\" r=\"%g", KEY_SIZE * imginfo->scale);
                    break;
                case 1:
                case 2:
                case 3:
                case 4:
                case 11:
                    // Player/Deathmatch start
                    fprintf(output, "green\" r=\"%g\" />\n", MONSTER_SIZE * imginfo->scale);
                    draw_direction(output, things[i], (float)REAL_X(things[i].x_pos), (float)REAL_Y(things[i].y_pos), imginfo->scale, "yellow");
                    break;
                default: 
                    fprintf(output, "magenta\" r=\"%g", ITEM_SIZE * imginfo->scale);
                    break;
            }
            fprintf(output, "\" />\n");
        }
    }
    fprintf(output, "</svg>\n");
}
