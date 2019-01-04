#ifndef VOXELGRID_H
# define VOXELGRID_H

#define VOXEL_GRID_SUPPORT 1

typedef float t_voxel;

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
    double min_value;
    double max_value;
    double mean_value;
    int nzero;
    t_voxel *vdata;	/* points to 3D array of voxels */
} vgrid;

/* function prototypes */
void voxel_command __PROTO((void));
void vclear_command __PROTO((void));
void vfill_command __PROTO((void));
void f_voxel __PROTO((union argument *x));
void init_voxelsupport __PROTO((void));
void set_vgrid __PROTO((void));
void set_vgrid_range __PROTO((void));
void show_vgrid __PROTO((void));
void gpfree_vgrid __PROTO((struct udvt_entry *grid));
void unset_vgrid __PROTO((void));
udvt_entry *get_vgrid_by_name __PROTO((char *name));
t_voxel voxel __PROTO((double vx, double vy, double vz));

#endif /* VOXELGRID_H */
