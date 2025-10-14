#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "map2img.h"

#define ARG_IMPLEMENTATION
#include "args.h"

void output_svg(Imginfo* imginfo, Wadinfo* wadinfo, FILE* output, bool verbose);

bool find_map(FILE* wadfile, Direntry* direntry, Direntry* d_vertexes, Direntry* d_linedefs, Direntry* d_things, char* mapname, int num_lumps, int infotableofs) {
    for (int i=0; i<num_lumps; ++i) {
        // TODO: check if fseek/fread
        fseek(wadfile, infotableofs+(i*sizeof(Direntry)), SEEK_SET);
        fread(direntry, sizeof(Direntry), 1, wadfile);
        if (strncmp(direntry->name, mapname, strlen(mapname)) == 0) {
            // Things are listed right after the mapname:
            fseek(wadfile, infotableofs+((i+1)*sizeof(Direntry)), SEEK_SET);
            fread(d_things, sizeof(Direntry), 1, wadfile);
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

bool list_maps(char* filename) {
    FILE* fh = fopen(filename, "r");
    if (fh == NULL) {
        fprintf(stderr, "ERROR: Could not open %s!\n", filename);
        return false;
    }

    Header wadheader;
    fread(&wadheader, sizeof(wadheader), 1, fh);

    Direntry *direntry = malloc(wadheader.num_lumps * sizeof(Direntry));

    int num_maps = 0;

    char entrystring[9];

    printf("Reading %s (%d lumps)...\n", filename, wadheader.num_lumps);
    for (unsigned int x=0; x<wadheader.num_lumps; x++)
    {
        fseek(fh, wadheader.infotableofs+(x*sizeof(Direntry)), SEEK_SET);
        fread(&direntry[x], sizeof(Direntry), 1, fh);

        if (strnlen(direntry[x].name, 8) >= 4) {
            bool ismap = false;
            char* n = direntry[x].name;
            if (strncmp(n, "MAP", 3) == 0 && n[3]>47 && n[3]<58) {
                // DOOM 2
                ismap = true;
            }
            if (n[0] == 'E' && n[2] == 'M' && n[1]>47 && n[1]<58) {
                // DOOM 1
                ismap = true;
            }
            if (ismap) {
                num_maps++;
                strncpy(entrystring, n, 8);
                printf("%d: %s (pos: %d, size: %d)\n", x, entrystring, direntry[x].filepos, direntry[x].size);
            }
        }
    }
    printf("%d map%s found\n", num_maps, num_maps!=1 ? "s" : "");
    free(direntry);
    fclose(fh);
    return true;
}

int main(int argc, char** argv) {
    bool verbose     = false;
    Header wadheader;
    char wad_ident[5] = "    ";
    Wadinfo wadinfo;
    Imginfo imginfo;
    imginfo.draw_things = false;

    // commandline arguments:
    arglist myarglist;
    init_list(&myarglist, argv[0], "converts a doom map to an svg image");
    add_arg(&myarglist, "-v", BOOL, "verbose output", false);
    add_arg(&myarglist, "-f", STRING, "WAD file", true);
    add_arg(&myarglist, "-m", STRING, "map name (e.g. E1M1)", false);
    add_arg(&myarglist, "-o", STRING, "output file name", false);
    add_arg(&myarglist, "-l", BOOL, "lists all maps and exits", false);
    add_arg(&myarglist, "-t", BOOL, "draw things", false);
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

    if (is_set(&myarglist, "-t")) {
        imginfo.draw_things = true;
    }

    if (is_set(&myarglist, "-o")) {
        output_filename = get_string_val(&myarglist, "-o");
    }
    
    if (is_set(&myarglist, "-l")) {
        if (!list_maps(filename)) return 1;
        free_args(&myarglist);
        return 0;
    }

    if (!is_set(&myarglist, "-m")) {
        fprintf(stderr, "ERROR: either -m [mapname] or -l has to be set!\n");
        free_args(&myarglist);
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
    Direntry d_things;
    if (find_map(wadfile, &direntry, &d_vertexes, &d_linedefs, &d_things, mapname, wadheader.num_lumps, wadheader.infotableofs)) {
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

        wadinfo.header    = wadheader;
        wadinfo.linedefs  = malloc(d_linedefs.size);
        wadinfo.vertexes  = malloc(d_vertexes.size);
        wadinfo.things    = malloc(d_things.size);

        fseek(wadfile, d_linedefs.filepos, SEEK_SET);
        fread(wadinfo.linedefs, d_linedefs.size, 1, wadfile);
        fseek(wadfile, d_vertexes.filepos, SEEK_SET);
        fread(wadinfo.vertexes, d_vertexes.size, 1, wadfile);
        fseek(wadfile, d_things.filepos, SEEK_SET);
        fread(wadinfo.things, d_things.size, 1, wadfile);


        wadinfo.num_vertexes = d_vertexes.size/sizeof(Vertex);
        wadinfo.num_linedefs = d_linedefs.size/sizeof(Linedef);
        wadinfo.num_things   = d_things.size/sizeof(Thing);

        // TODO: this belongs in output_svg() but then we have to pass all the variables
        //       or make them global
        if (verbose) {
            fprintf(output, "<!--\n");
            fprintf(output, "wadfile         : %s => %s\n", filename, wad_ident);
            fprintf(output, "map             : %s\n", mapname);
            fprintf(output, "num_lumps       : %d\n", wadheader.num_lumps);
            fprintf(output, "num_things      : %ld\n", wadinfo.num_things);
            fprintf(output, "%ld Linedefs pos: %d / size: %d\n", wadinfo.num_linedefs, d_linedefs.filepos, d_linedefs.size);
            fprintf(output, "%ld Vertexes pos: %d / size: %d\n", wadinfo.num_vertexes, d_vertexes.filepos, d_vertexes.size);
            fprintf(output, "-->\n");
        }

        // SVG stuff:
        int max_x = wadinfo.vertexes[0].x;
        int min_x = wadinfo.vertexes[0].x;
        int max_y = wadinfo.vertexes[0].y;
        int min_y = wadinfo.vertexes[0].y;
        imginfo.x_off = 0;
        imginfo.y_off = 0;
        generate_minmax(&max_x, &min_x, &max_y, &min_y, wadinfo.vertexes, wadinfo.num_vertexes);
        generate_offsets(&imginfo.x_off, &imginfo.y_off, min_x, min_y);
        imginfo.width  = max_x + imginfo.x_off;
        imginfo.height = max_y + imginfo.y_off;
        imginfo.max_x  = max_x;
        imginfo.max_y  = max_y;
            
        output_svg(&imginfo, &wadinfo, output, verbose);

        free(wadinfo.linedefs);
        free(wadinfo.vertexes);
        free(wadinfo.things);
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
