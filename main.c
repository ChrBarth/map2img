#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "map2img.h"

#define ARG_IMPLEMENTATION
#include "args.h"

bool find_map(FILE* wadfile, Direntry* direntry, Direntry* d_vertexes, Direntry* d_linedefs, char* mapname, int num_lumps, int infotableofs) {
    for (int i=0; i<num_lumps; ++i) {
        // TODO: check if fseek/fread
        fseek(wadfile, infotableofs+(i*sizeof(Direntry)), SEEK_SET);
        fread(direntry, sizeof(Direntry), 1, wadfile);
        if (strncmp(direntry->name, mapname, strlen(mapname)) == 0) {
            // Linedefs come 2 entries after mapname:
            fseek(wadfile, infotableofs+((i+2)*sizeof(Direntry)), SEEK_SET);
            fread(d_linedefs, sizeof(Direntry), 1, wadfile);
            // Vertexes are 4 after mapname:
            fseek(wadfile, infotableofs+((i+4)*sizeof(Direntry)), SEEK_SET);
            fread(d_vertexes, sizeof(Direntry), 1, wadfile);
            return true;
        }
    }
    return false;
}

void generate_minmax(int* max_x, int* min_x, int* max_y, int* min_y, Vertex* vertexes, int num_vertexes) {
    for (int i=1; i<num_vertexes; ++i) {
        if(vertexes[i].x > *max_x) *max_x = vertexes[i].x;
        if(vertexes[i].x < *min_x) *min_x = vertexes[i].x;
        if(vertexes[i].y > *max_y) *max_y = vertexes[i].y;
        if(vertexes[i].y < *min_y) *min_y = vertexes[i].y;
    }
}

void generate_offsets(int* x_off, int* y_off, int min_x, int min_y) {
    // generate offset so we only get positive variables:
    if (min_x<0) *x_off = min_x * -1;
    if (min_y<0) *y_off = min_y * -1;
    if (min_x>0) *x_off = min_x;
    if (min_y>0) *y_off = min_y;
}

void output_svg(int width, int height, int x_off, int max_y, int num_linedefs, Linedef* linedefs, Vertex* vertexes, FILE* output, bool verbose) {
    fprintf(output, "<svg version=\"1.1\"");
    fprintf(output, " width=\"%d\" height=\"%d\">\n", width, height);
    for (int i=0; i<num_linedefs; ++i) {
        int16_t v_index_start = linedefs[i].v_start;
        int16_t v_index_end   = linedefs[i].v_end;
        Vertex start = vertexes[v_index_start];
        Vertex end   = vertexes[v_index_end];

        if (verbose) {
            fprintf(output, "<!-- Linedef %d - Flags: %d / Special: %d -->\n", i, linedefs[i].flags, linedefs[i].special);
        }

        fprintf(output, "<line x1=\"%d\" y1=\"%d\"", start.x + x_off, max_y - start.y );
        fprintf(output, " x2=\"%d\" y2=\"%d\"", end.x + x_off, max_y - end.y);
        fprintf(output, " stroke=\"");
        // TODO: put this somewhere else
        switch(linedefs[i].special) {
            case 0:
                fprintf(output, "black");
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
                fprintf(output, "grey");
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
                fprintf(output, "2");
                break;
            default:
                fprintf(output, "4");
                break;
        }
        fprintf(output, "\"/>\n");
    }
    fprintf(output, "</svg>\n");
}

void list_maps(char* filename) {
    FILE* fh = fopen(filename, "r");

    Header wadheader;
    fread(&wadheader, sizeof(wadheader), 1, fh);

    Direntry *direntry;

    int num_maps = 0;

    direntry = malloc(wadheader.num_lumps * sizeof(Direntry));
    char entrystring[9];

    for (unsigned int x=0; x<wadheader.num_lumps; x++)
    {
        fseek(fh, wadheader.infotableofs+(x*sizeof(Direntry)), SEEK_SET);
        fread(&direntry[x], sizeof(Direntry), 1, fh);

        if (strlen(direntry[x].name) >= 4) {
            if (strncmp(direntry[x].name, "MAP", 3) == 0 && direntry[x].name[3]>47 && direntry[x].name[3]<58) {
                num_maps++;
                strncpy(entrystring, direntry[x].name, 8);
                printf("%d: %s (pos: %d, size: %d)\n", x, entrystring, direntry[x].filepos, direntry[x].size);
                }
            if (direntry[x].name[0] == 'E' && direntry[x].name[2] == 'M') {
                num_maps++;
                strncpy(entrystring, direntry[x].name, 8);
                printf("%d: %s (pos: %d, size: %d)\n", x, entrystring, direntry[x].filepos, direntry[x].size);
            }
        }
    }
    printf("%d map%s found\n", num_maps, num_maps!=1 ? "s" : "");
    free(direntry);
}

int main(int argc, char** argv) {
    bool verbose = false;
    Header wadheader;
    char wad_ident[5] = "    ";

    // commandline arguments:
    arglist myarglist;
    init_list(&myarglist, argv[0], "converts a doom map to an svg image");
    add_arg(&myarglist, "-v", BOOL, "verbose output", false);
    add_arg(&myarglist, "-f", STRING, "WAD file", true);
    add_arg(&myarglist, "-m", STRING, "map name (e.g. E1M1)", false);
    add_arg(&myarglist, "-o", STRING, "output file name", false);
    add_arg(&myarglist, "-l", BOOL, "lists all maps and exits", false);
    if (!parse_args(&myarglist, argc, argv)) {
        fprintf(stderr, "Error parsing arguments!\n");
        print_help(&myarglist);
        return 1;
    }
    char* filename        = get_string_val(&myarglist, "-f");
    char* mapname         = get_string_val(&myarglist, "-m");
    char* output_filename = NULL;

    if (is_set(&myarglist, "-v")) {
            verbose = true;
            }

    if (is_set(&myarglist, "-o")) {
        output_filename = get_string_val(&myarglist, "-o");
    }
    
    if (is_set(&myarglist, "-l")) {
        list_maps(filename);
        return 0;
    }

    if (!is_set(&myarglist, "-m")) {
        fprintf(stderr, "ERROR: either -m [mapname] or -l has to be set!\n");
        return 1;
    }
    // End commandline arguments

    FILE* wadfile = fopen(filename, "rb");
    if (wadfile == NULL) {
        fprintf(stderr, "Could not open %s!\n", filename);
        return 1;
    }

    // read the header:
    fread(&wadheader, sizeof(Header), 1, wadfile);
    strncpy(wad_ident, wadheader.identification, 4);
    if (strcmp(wad_ident, "IWAD") != 0 && strcmp(wad_ident, "PWAD") != 0) {
        fprintf(stderr, "ERROR, no wadfile (wad_ident: %s)\n", wad_ident);
        return 1;
    }

    Direntry direntry;
    Direntry d_linedefs;
    Direntry d_vertexes;
    if (find_map(wadfile, &direntry, &d_vertexes, &d_linedefs, mapname, wadheader.num_lumps, wadheader.infotableofs)) {
        FILE* output;
        if (output_filename) {
            output = fopen(output_filename, "w");
            if (!output) {
                fprintf(stderr, "ERROR, could not open output file %s\n", output_filename);
                return 1;
            }
        }
        else {
            output = stdout;
        }

        Linedef* linedefs = malloc(d_linedefs.size);
        Vertex* vertexes  = malloc(d_vertexes.size);

        fseek(wadfile, d_linedefs.filepos, SEEK_SET);
        fread(linedefs, d_linedefs.size, 1, wadfile);
        fseek(wadfile, d_vertexes.filepos, SEEK_SET);
        fread(vertexes, d_vertexes.size, 1, wadfile);

        long int num_vertexes = d_vertexes.size/sizeof(Vertex);
        long int num_linedefs = d_linedefs.size/sizeof(Linedef);

        // TODO: this belongs in output_svg() but then we have to pass all the variables
        //       or make them global
        if (verbose) {
            fprintf(output, "<!--\n");
            fprintf(output, "wadfile         : %s => %s\n", filename, wad_ident);
            fprintf(output, "map             : %s\n", mapname);
            fprintf(output, "num_lumps       : %d\n", wadheader.num_lumps);
            fprintf(output, "%ld Linedefs pos: %d / size: %d\n", num_linedefs, d_linedefs.filepos, d_linedefs.size);
            fprintf(output, "%ld Vertexes pos: %d / size: %d\n", num_vertexes, d_vertexes.filepos, d_vertexes.size);
            fprintf(output, "-->\n");
        }

        // SVG stuff:
        int max_x = vertexes[0].x;
        int min_x = vertexes[0].x;
        int max_y = vertexes[0].y;
        int min_y = vertexes[0].y;
        int x_off = 0;
        int y_off = 0;
        generate_minmax(&max_x, &min_x, &max_y, &min_y, vertexes, num_vertexes);
        generate_offsets(&x_off, &y_off, min_x, min_y);
        int width  = max_x + x_off;
        int height = max_y + y_off;
        output_svg(width, height, x_off, max_y, num_linedefs, linedefs, vertexes, output, verbose);

        free(linedefs);
        free(vertexes);
        if (output_filename) {
            fclose(output);
        }

    }
    else {
        fprintf(stderr, "%s not found in %s!\n", mapname, filename);
        free_args(&myarglist);
        fclose(wadfile);
        return 1;
    }

    free_args(&myarglist);
    fclose(wadfile);
    return 0;
}
