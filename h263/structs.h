#ifndef _STRUCTS_H_
#define _STRUCTS_H_

typedef struct {
  int  w, h;
  int  ws, hs;       /* subimage dimensions */
  Byte *y, *u, *v;
} Picture;

typedef struct {
  int   w, h;
  short *mode;
  short *mx, *my;
} MVField;

#endif   /* #ifndef _STRUCTS_H_ */
