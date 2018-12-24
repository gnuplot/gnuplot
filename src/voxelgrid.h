#ifndef VOXELGRID_H
# define VOXELGRID_H

#define VOXEL_GRID_SUPPORT 1

typedef double t_voxel;

typedef struct vgrid {
    int size;		/* size x size x size array */
    double vxmin;
    double vxmax;
    double vxdelta;
    double vymin;
    double vymax;
    double vydelta;
    double vzmin;
    double vzmax;
    double vzdelta;
    t_voxel *vdata;	/* points to 3D array of voxels */
} vgrid;

extern vgrid *current_vgrid;
extern struct udvt_entry *udv_VoxelDistance;

/* function prototypes */
void voxel_command __PROTO((void));
void vclear_command __PROTO((void));
void vfill_command __PROTO((void));
void f_voxel __PROTO((union argument *x));
void set_vgrid __PROTO((void));
void set_vgrid_range __PROTO((void));

#endif /* VOXELGRID_H */
