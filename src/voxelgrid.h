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
    double stddev;
    int nzero;
    t_voxel *vdata;	/* points to 3D array of voxels */
} vgrid;

typedef struct isosurface_opt{
    int tessellation;	/* 0 = mixed  1 = triangles only */
    int inside_offset;	/* difference between front/back linetypes */
} isosurface_opt;

/* function prototypes */
void voxel_command(void);
void vclear_command(void);
void vfill_command(void);
void f_voxel(union argument *x);
void init_voxelsupport(void);
void set_vgrid(void);
void set_vgrid_range(void);
void show_vgrid(void);
void gpfree_vgrid(struct udvt_entry *grid);
void unset_vgrid(void);
udvt_entry *get_vgrid_by_name(char *name);
void check_grid_ranges(void);
t_voxel voxel(double vx, double vy, double vz);
void set_isosurface(void);
void show_isosurface(void);

/* variables */
extern isosurface_opt isosurface_options;

#endif /* VOXELGRID_H */
