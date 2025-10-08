#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "map2img.h"

#define DEBUG

int main() {
    // let's hardcode some stuff for now:
    char* filename = "/media/sdb1/Spiele/doom/DOOM1.WAD";
    char* mapname  = "E1M1";
    Header wadheader;
    char wad_ident[5] = "    ";

    FILE* wadfile = fopen(filename, "rb");
    if (wadfile == NULL) {
        fprintf(stderr, "Could not open %s!\n", filename);
        return 1;
    }

    // read the header:
    fread(&wadheader, sizeof(Header), 1, wadfile);
    strncpy(wad_ident, wadheader.identification, 4);
#ifdef DEBUG
    printf("<!--\n");
    printf("%s => %s\n", filename, wad_ident);
    printf("num_lumps: %d\n", wadheader.num_lumps);
    printf("infotableofs: %d\n", wadheader.infotableofs);
    printf("-->\n");
#endif

    Direntry direntry;
    Direntry d_linedefs;
    Direntry d_vertexes;
    bool found = false;
    for (int i=0; i<wadheader.num_lumps; ++i) {
        fseek(wadfile, wadheader.infotableofs+(i*sizeof(Direntry)), SEEK_SET);
        fread(&direntry, sizeof(Direntry), 1, wadfile);
        if (strncmp(direntry.name, mapname, strlen(mapname)) == 0) {
            // Linedefs come 2 entries after mapname:
            fseek(wadfile, wadheader.infotableofs+((i+2)*sizeof(Direntry)), SEEK_SET);
            fread(&d_linedefs, sizeof(Direntry), 1, wadfile);
            // Vertexes are 4 after mapname:
            fseek(wadfile, wadheader.infotableofs+((i+4)*sizeof(Direntry)), SEEK_SET);
            fread(&d_vertexes, sizeof(Direntry), 1, wadfile);
            found = true;
        }
    }
    if (found) {
        Linedef* linedefs = malloc(d_linedefs.size);
        Vertex* vertexes  = malloc(d_vertexes.size);
        long int num_vertexes = d_vertexes.size/sizeof(Vertex);
        long int num_linedefs = d_linedefs.size/sizeof(Linedef);
#ifdef DEBUG
        printf("<!--\n");
        printf("%ld Linedefs pos: %d / size: %d\n", num_linedefs, d_linedefs.filepos, d_linedefs.size);
        printf("%ld Vertexes pos: %d / size: %d\n", num_vertexes, d_vertexes.filepos, d_vertexes.size);
        printf("-->\n");
#endif
        fseek(wadfile, d_linedefs.filepos, SEEK_SET);
        fread(linedefs, d_linedefs.size, 1, wadfile);
        fseek(wadfile, d_vertexes.filepos, SEEK_SET);
        fread(vertexes, d_vertexes.size, 1, wadfile);
        // SVG stuff:
        int max_x = -9999;
        int min_x = 0;
        int max_y = -9999;
        int min_y = 0;
        for (int i=0; i<num_vertexes; ++i) {
            if(vertexes[i].x > max_x) max_x = vertexes[i].x;
            if(vertexes[i].x < min_x) min_x = vertexes[i].x;
            if(vertexes[i].y > max_y) max_y = vertexes[i].y;
            if(vertexes[i].y < min_y) min_y = vertexes[i].y;
        }
        // generate offset so we only get positive variables:
        int x_off = 0;
        int y_off = 0;
        if (min_x<0) x_off = min_x * -1;
        if (min_y<0) y_off = min_y * -1;
#ifdef DEBUG
        printf("<!--\n");
        printf("min/max x: %d / %d\n", min_x, max_x);
        printf("min/max y: %d / %d\n", min_y, max_y);
        printf("-->\n");
#endif

        printf("<svg version=\"1.1\"");
        printf(" width=\"%d\" height=\"%d\">\n", max_x + x_off, max_y + y_off);
        for (int i=0; i<num_linedefs; ++i) {
            int16_t v_index_start = linedefs[i].v_start;
            int16_t v_index_end   = linedefs[i].v_end;
            Vertex start = vertexes[v_index_start];
            Vertex end   = vertexes[v_index_end];
#ifdef DEBUG
            printf("<!-- Flags: %d-->\n", linedefs[i].flags);
#endif
            printf("<line x1=\"%d\" y1=\"%d\"", start.x + x_off, start.y + y_off);
            printf(" x2=\"%d\" y2=\"%d\"", end.x + x_off, end.y + y_off);
            printf(" stroke=\"%s\" stroke-width=\"4\"/>\n", (linedefs[i].flags & 4 == 4) ? "black" : "grey");
        }
        printf("</svg>\n");
        free(linedefs);
        free(vertexes);

    }

    fclose(wadfile);
    return 0;
}
