#ifndef MAP2IMG_H_

typedef short int16_t;

// https://doomwiki.org/wiki/Vertex
typedef struct {
    int16_t x;
    int16_t y;
} Vertex;

// https://doomwiki.org/wiki/Linedef
typedef struct {
    int16_t v_start;
    int16_t v_end;
    int16_t flags;
    int16_t special;
    int16_t tag;
    int16_t f_sidenum;
    int16_t b_sidenum;
} Linedef;

// https://doomwiki.org/wiki/WAD
typedef struct {
    char identification[4];
    int num_lumps;
    int infotableofs;
} Header;

typedef struct {
    int filepos;
    int size;
    char name[8];
} Direntry;

// https://doomwiki.org/wiki/WAD#Lump_order
// Structure for E1M1 in DOOM1.WAD:
/*
 * E1M1 (pos: 67500, size: 0)
 * THINGS (pos: 67500, size: 1380)
 * LINEDEFS (pos: 68880, size: 6650)
 * SIDEDEFS (pos: 75532, size: 19440)
 * VERTEXES (pos: 94972, size: 1868)
 * SEGS (pos: 96840, size: 8784)
 * SSECTORS (pos: 105624, size: 948)
 * NODES (pos: 106572, size: 6608)
 * SECTORS (pos: 113180, size: 2210)
 * REJECT (pos: 115392, size: 904)
 * BLOCKMAP (pos: 116296, size: 6922)
 * */

// so first we find the name of the map (E1M1),
// then the next LINEDEFS and VERTEXES entries

#endif // MAP2IMG_H_
