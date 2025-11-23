#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "map2img.h"
#include <errno.h>

#define ARG_IMPLEMENTATION
#include "args.h"

void output_svg(Imginfo* imginfo, Wadinfo* wadinfo, FILE* output, bool verbose, Header* wadheader);

bool find_map(FILE* wadfile, Direntry* direntry, Direntry* d_vertexes, Direntry* d_linedefs, Direntry* d_things, char* mapname, int num_lumps, int infotableofs) {
    int res;
    for (int i=0; i<num_lumps; ++i) {
        res = fseek(wadfile, infotableofs+(i*sizeof(Direntry)), SEEK_SET);
        if (res < 0) {
            fprintf(stderr, "find_map(): %s\n", strerror(errno));
            return false;
        }
        size_t bytes_read;
        bytes_read = fread(direntry, 1, sizeof(Direntry), wadfile);
        if (bytes_read != sizeof(Direntry)) {
            fprintf(stderr, "find_map(): Direntry read failed, got %zu, expected %zu bytes!\n", bytes_read, sizeof(Direntry));
            return false;
        }
        if (strncmp(direntry->name, mapname, strlen(mapname)) == 0) {
            // Things are listed right after the mapname:
            res = fseek(wadfile, infotableofs+((i+1)*sizeof(Direntry)), SEEK_SET);
            if (res < 0) {
                fprintf(stderr, "find_map() - things: %s\n", strerror(errno));
                return false;
            }
            bytes_read = fread(d_things, 1, sizeof(Direntry), wadfile);
            if (bytes_read != sizeof(Direntry)) {
                fprintf(stderr, "find_map(): Direntry things read failed, got %zu, expected %zu bytes!\n", bytes_read, sizeof(Direntry));
                return false;
            }
            // Linedefs come 2 entries after mapname:
            res = fseek(wadfile, infotableofs+((i+2)*sizeof(Direntry)), SEEK_SET);
            if (res < 0) {
                fprintf(stderr, "find_map() - linedefs: %s\n", strerror(errno));
                return false;
            }
            bytes_read = fread(d_linedefs, 1, sizeof(Direntry), wadfile);
            if (bytes_read != sizeof(Direntry)) {
                fprintf(stderr, "find_map(): Direntry linedefs read failed, got %zu, expected %zu bytes!\n", bytes_read, sizeof(Direntry));
                return false;
            }
            // Vertexes are 4 after mapname:
            res = fseek(wadfile, infotableofs+((i+4)*sizeof(Direntry)), SEEK_SET);
            if (res < 0) {
                fprintf(stderr, "find_map() - vertexes: %s\n", strerror(errno));
                return false;
            }
            bytes_read = fread(d_vertexes, 1, sizeof(Direntry), wadfile);
            if (bytes_read != sizeof(Direntry)) {
                fprintf(stderr, "find_map(): Direntry vertexes read failed, got %zu, expected %zu bytes!\n", bytes_read, sizeof(Direntry));
                return false;
            }
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
    size_t bytes_read;
    bytes_read = fread(&wadheader, 1, sizeof(wadheader), fh);
    if (bytes_read != sizeof(wadheader)) {
        fprintf(stderr, "list_maps() fread wadheader failed (got %zu, expected %zu bytes)!\n", bytes_read, sizeof(wadheader));
        return false;
    }

    Direntry *direntry = malloc(wadheader.num_lumps * sizeof(Direntry));

    int num_maps = 0;

    char entrystring[9];

    printf("Reading %s (%d lumps)...\n", filename, wadheader.num_lumps);
    for (unsigned int x=0; x<wadheader.num_lumps; x++)
    {
        int res = fseek(fh, wadheader.infotableofs+(x*sizeof(Direntry)), SEEK_SET);
        if (res < 0) {
            fprintf(stderr, "list_maps(): %s", strerror(errno));
            return false;
        }
        bytes_read = fread(&direntry[x], 1, sizeof(Direntry), fh);
        if (bytes_read != sizeof(wadheader)) {
            fprintf(stderr, "list_maps() fread Direntry failed (got %zu, expected %zu bytes)!\n", bytes_read, sizeof(Direntry));
            return false;
        }

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
    Wadinfo wadinfo;
    char wad_ident[5]   = "    ";
    Imginfo imginfo;
    imginfo.draw_things = false;
    imginfo.scale       = 0.5;
    imginfo.padding     = 0;

    // commandline arguments:
    arglist myarglist;
    init_list(&myarglist, argv[0], "converts a doom map to an svg image");
    add_arg(&myarglist, "-v", BOOL, "verbose output", false);
    add_arg(&myarglist, "-f", STRING, "WAD file", true);
    add_arg(&myarglist, "-m", STRING, "map name (e.g. E1M1)", false);
    add_arg(&myarglist, "-o", STRING, "output file name", false);
    add_arg(&myarglist, "-l", BOOL, "lists all maps in the wad file and exits", false);
    add_arg(&myarglist, "-t", BOOL, "draw things", false);
    add_arg(&myarglist, "-s", FLOAT, "scale factor (default: 0.5)", false);
    add_arg(&myarglist, "-p", INTEGER, "additional padding from the image borders (default: 0)", false);
    if (!parse_args(&myarglist, argc, argv)) {
        fprintf(stderr, "Error parsing arguments!\n");
        print_help(&myarglist);
        free_args(&myarglist);
        return 1;
    }
    wadinfo.filename      = get_string_val(&myarglist, "-f");
    wadinfo.mapname       = get_string_val(&myarglist, "-m");
    char* output_filename = NULL;

    if (is_set(&myarglist, "-p")) {
        imginfo.padding = get_int_val(&myarglist, "-p");
    }

    if (is_set(&myarglist, "-s")) {
        imginfo.scale = get_float_val(&myarglist, "-s");
    }

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
        if (!list_maps(wadinfo.filename)) return 1;
        free_args(&myarglist);
        return 0;
    }

    if (!is_set(&myarglist, "-m")) {
        fprintf(stderr, "ERROR: either -m [mapname] or -l has to be set!\n");
        free_args(&myarglist);
        return 1;
    }
    // End commandline arguments

    FILE* wadfile = fopen(wadinfo.filename, "rb");
    if (wadfile == NULL) {
        fprintf(stderr, "Could not open %s!\n", wadinfo.filename);
        return 1;
    }

    // read the header:
    size_t bytes_read;
    bytes_read = fread(&wadheader, 1, sizeof(Header), wadfile);
    if (bytes_read != sizeof(Header)) {
        fprintf(stderr, "fread Header failed (got %zu, expected %zu bytes)!\n", bytes_read, sizeof(Header));
        return 1;
    }
    strncpy(wadinfo.wad_ident, wadheader.identification, 4);
    if (strcmp(wadinfo.wad_ident, "IWAD") != 0 && strcmp(wadinfo.wad_ident, "PWAD") != 0) {
        fprintf(stderr, "ERROR, no wadfile (wad_ident: %s)\n", wadinfo.wad_ident);
        return 1;
    }

    Direntry direntry;
    Direntry d_linedefs;
    Direntry d_vertexes;
    Direntry d_things;
    if (find_map(wadfile, &direntry, &d_vertexes, &d_linedefs, &d_things, wadinfo.mapname, wadheader.num_lumps, wadheader.infotableofs)) {
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

        int res;
        res = fseek(wadfile, d_linedefs.filepos, SEEK_SET);
        if (res < 0) {
            fprintf(stderr, "fseek linedefs failed: %s\n", strerror(errno));
            return 1;
        }
        bytes_read = fread(wadinfo.linedefs, 1, d_linedefs.size, wadfile);
        if (bytes_read != d_linedefs.size) {
            fprintf(stderr, "fread linedefs failed (got %zu, expected %zu bytes)!\n", bytes_read, (size_t)d_linedefs.size);
            return 1;
        }
        res = fseek(wadfile, d_vertexes.filepos, SEEK_SET);
        if (res < 0) {
            fprintf(stderr, "fseek vertexes failed: %s\n", strerror(errno));
            return 1;
        }
        bytes_read = fread(wadinfo.vertexes, 1, d_vertexes.size, wadfile);
        if (bytes_read != d_vertexes.size) {
            fprintf(stderr, "fread vertexes failed (got %zu, expected %zu bytes)!\n", bytes_read, (size_t)d_vertexes.size);
            return 1;
        }
        res = fseek(wadfile, d_things.filepos, SEEK_SET);
        if (res < 0) {
            fprintf(stderr, "fseek things failed: %s\n", strerror(errno));
            return 1;
        }
        bytes_read = fread(wadinfo.things, 1, d_things.size, wadfile);
        if (bytes_read != d_things.size) {
            fprintf(stderr, "fread things failed (got %zu, expected %zu bytes)!\n", bytes_read, (size_t)d_things.size);
            return 1;
        }


        wadinfo.num_vertexes = d_vertexes.size/sizeof(Vertex);
        wadinfo.num_linedefs = d_linedefs.size/sizeof(Linedef);
        wadinfo.num_things   = d_things.size/sizeof(Thing);

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
            
        output_svg(&imginfo, &wadinfo, output, verbose, &wadheader);

        free(wadinfo.linedefs);
        free(wadinfo.vertexes);
        free(wadinfo.things);
        if (output_filename) {
            fclose(output);
        }

    }
    else {
        fprintf(stderr, "%s not found in %s!\n", wadinfo.mapname, wadinfo.filename);
        free_args(&myarglist);
        fclose(wadfile);
        return 1;
    }

    free_args(&myarglist);
    fclose(wadfile);
    return 0;
}
