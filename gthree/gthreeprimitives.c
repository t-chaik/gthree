#include <math.h>
#include <epoxy/gl.h>

#include "gthreeprimitives.h"
#include "gthreeattribute.h"

enum {
  AXIS_X,
  AXIS_Y,
  AXIS_Z,
};

static void
push3i (GArray *array, guint16 a, guint16 b, guint16 c)
{
  g_array_append_val (array, a);
  g_array_append_val (array, b);
  g_array_append_val (array, c);
}

static void
push3 (GArray *array, float f1, float f2, float f3)
{
  g_array_append_val (array, f1);
  g_array_append_val (array, f2);
  g_array_append_val (array, f3);
}

static void
push3v (GArray *array, float *fs)
{
  push3 (array, fs[0], fs[1], fs[2]);
}

static void
push_vec3 (GArray *array, const graphene_vec3_t *v)
{
  push3 (array,
         graphene_vec3_get_x (v),
         graphene_vec3_get_y (v),
         graphene_vec3_get_z (v));
}

static void
push2 (GArray *array, float f1, float f2)
{
  g_array_append_val (array, f1);
  g_array_append_val (array, f2);
}

/* Takes ownership of index */
static GthreeAttribute *
add_indexv (GthreeGeometry *geometry, GArray *index)
{
  g_autoptr(GthreeAttribute) a = gthree_attribute_new_from_uint16 ("index", (guint16 *)index->data, index->len, 1);
  gthree_geometry_set_index (geometry, a);
  g_array_free (index, TRUE);

  return a;
}

/* Takes ownership of vects */
static GthreeAttribute *
add_vec3v (GthreeGeometry *geometry, const char *name, GArray *vects)
{
  g_autoptr(GthreeAttribute) a = gthree_attribute_new_from_float (name, (float *)vects->data, vects->len / 3, 3);
  gthree_geometry_add_attribute (geometry, name, a);
  g_array_free (vects, TRUE);
  return a; /* Ref owned by geometry */
}

/* Takes ownership of vects */
static GthreeAttribute *
add_vec2v (GthreeGeometry *geometry, const char *name, GArray *vects)
{
  g_autoptr(GthreeAttribute) a = gthree_attribute_new_from_float (name, (float *)vects->data, vects->len / 2, 2);
  gthree_geometry_add_attribute (geometry, name, a);
  g_array_free (vects, TRUE);
  return a; /* Ref owned by geometry */
}

static GthreeAttribute *
add_positionv (GthreeGeometry *geometry, GArray *position)
{
  return add_vec3v (geometry, "position", position);
}

static GthreeAttribute *
add_normalv (GthreeGeometry *geometry, GArray *normal)
{
  return add_vec3v (geometry, "normal", normal);
}

static GthreeAttribute *
add_uvv (GthreeGeometry *geometry, GArray *uv)
{
  return add_vec2v (geometry, "uv", uv);
}

static void
build_plane (GthreeGeometry *geometry, GArray *vertices, GArray *normals, GArray *uvs, GArray *index,
             int u, int v, int udir, int vdir, float width, float height, float depth, int materialIndex,
             int widthSegments, int heightSegments, int depthSegments)
{
  int w;
  int ix, iy;
  float gridX = widthSegments;
  float gridY = heightSegments;
  float gridX1, gridY1, segment_width, segment_height;
  float width_half = width / 2;
  float height_half = height / 2;
  float normal[3];
  int index_offset = vertices->len / 3;
  int group_start = index->len;

  if ((u == AXIS_X && v == AXIS_Y) || (u == AXIS_Y && v == AXIS_X))
    {
      w = AXIS_Z;
    }
  else if ((u == AXIS_X && v == AXIS_Z) || (u == AXIS_Z && v == AXIS_X))
    {
      w = AXIS_Y;
      gridY = depthSegments;
    }
  else /* ((u == AXIS_Z && v == AXIS_Y) || (u == AXIS_Y && v == AXIS_Z)) */
    {
      w = AXIS_X;
      gridX = depthSegments;
    }

  gridX1 = gridX + 1;
  gridY1 = gridY + 1;

  segment_width = width / gridX;
  segment_height = height / gridY;

  normal[u] = 0;
  normal[v] = 0;
  normal[w] = depth >= 0 ? 1 : -1;

  for (iy = 0; iy < gridY1; iy++)
    {
      for (ix = 0; ix < gridX1; ix++)
        {
          float vector[3];

          vector[u] = ( ix * segment_width - width_half ) * udir;
          vector[v] = ( iy * segment_height - height_half ) * vdir;
          vector[w] = depth;

          push3v (vertices, vector);
          push3v (normals, normal);
          push2 (uvs, ix / (float)gridX, 1 - iy / (float)gridY);
        }
    }

  for (iy = 0; iy < gridY; iy++)
    {
      for (ix = 0; ix < gridX; ix++)
        {
          int a = index_offset + ix + gridX1 * iy ;
          int b = index_offset + ix + gridX1 * ( iy + 1 );
          int c = index_offset + ( ix + 1 ) + gridX1 * ( iy + 1 );
          int d = index_offset + ( ix + 1 ) + gridX1 * iy;

          /* Each rect has two tris, a, b, d, and b, c, d */
          push3i (index, a, b, d);
          push3i (index, b, c, d);
        }
    }

  gthree_geometry_add_group (geometry, group_start, index->len - group_start, materialIndex);
}

GthreeGeometry *
gthree_geometry_new_box (float width, float height, float depth,
                         int widthSegments, int heightSegments, int depthSegments)
{
  GthreeGeometry *geometry;
  float width_half = width / 2;
  float height_half = height / 2;
  float depth_half = depth / 2;
  GArray *positions, *normals, *uvs, *index;

  positions = g_array_new (FALSE, FALSE, sizeof (float));
  normals = g_array_new (FALSE, FALSE, sizeof (float));
  uvs = g_array_new (FALSE, FALSE, sizeof (float));
  index = g_array_new (FALSE, FALSE, sizeof (guint16));

  geometry = gthree_geometry_new ();

  build_plane (geometry, positions, normals, uvs, index, AXIS_Z, AXIS_Y, -1, -1, depth, height, width_half, 0, widthSegments, heightSegments, depthSegments); // px
  build_plane (geometry, positions, normals, uvs, index, AXIS_Z, AXIS_Y,  1, -1, depth, height, - width_half, 1, widthSegments, heightSegments, depthSegments); // nx
  build_plane (geometry, positions, normals, uvs, index, AXIS_X, AXIS_Z,  1,  1, width, depth, height_half, 2, widthSegments, heightSegments, depthSegments ); // py
  build_plane (geometry, positions, normals, uvs, index, AXIS_X, AXIS_Z,  1, -1, width, depth, - height_half, 3, widthSegments, heightSegments, depthSegments ); // ny
  build_plane (geometry, positions, normals, uvs, index, AXIS_X, AXIS_Y,  1, -1, width, height, depth_half, 4, widthSegments, heightSegments, depthSegments ); // pz
  build_plane (geometry, positions, normals, uvs, index, AXIS_X, AXIS_Y, -1, -1, width, height, - depth_half, 5, widthSegments, heightSegments, depthSegments ); // nz

  add_indexv (geometry, index);
  add_positionv (geometry, positions);
  add_normalv (geometry, normals);
  add_uvv (geometry, uvs);

  return geometry;
}

GthreeGeometry *
gthree_geometry_new_plane (float width, float height,
                           int widthSegments, int heightSegments)
{
  GthreeGeometry *geometry;
  GArray *positions, *normals, *uvs, *index;

  positions = g_array_new (FALSE, FALSE, sizeof (float));
  normals = g_array_new (FALSE, FALSE, sizeof (float));
  uvs = g_array_new (FALSE, FALSE, sizeof (float));
  index = g_array_new (FALSE, FALSE, sizeof (guint16));

  geometry = gthree_geometry_new ();

  build_plane (geometry, positions, normals, uvs, index, AXIS_X, AXIS_Y,  1, -1, width, height, 0, 0, widthSegments, heightSegments, 1); // pz

  add_indexv (geometry, index);
  add_positionv (geometry, positions);
  add_normalv (geometry, normals);
  add_uvv (geometry, uvs);

  return geometry;
}

GthreeGeometry *
gthree_geometry_new_sphere_full (float radius,
                                 int widthSegments, int heightSegments,
                                 float phiStart, float phiLength,
                                 float thetaStart, float thetaLength)
{
  GthreeGeometry *geometry;
  int x, y, vertex_count, vertices_w, vertices_h;
  g_autofree int *vertices = NULL;
  graphene_sphere_t bound;
  graphene_point3d_t center;
  float thetaEnd;
  GArray *positions, *normals, *uvs, *index;

  geometry = gthree_geometry_new ();

  widthSegments = MAX(3, widthSegments);
  heightSegments = MAX(2, heightSegments);

  thetaEnd = MIN (thetaStart + thetaLength, G_PI);

  vertices_w = widthSegments + 1;
  vertices_h = heightSegments + 1;
  vertices = g_new (int, vertices_w * vertices_h);

  positions = g_array_new (FALSE, FALSE, sizeof (float));
  normals = g_array_new (FALSE, FALSE, sizeof (float));
  uvs = g_array_new (FALSE, FALSE, sizeof (float));
  index = g_array_new (FALSE, FALSE, sizeof (guint16));

  vertex_count = 0;
  for (y = 0; y <= heightSegments; y++)
    {
      float v, u_offset;

      v = (float)y / heightSegments;

      // special case for the poles
      u_offset = 0;
      if (y == 0 && thetaStart == 0)
        u_offset = 0.5 / widthSegments;
      else if (y == heightSegments && thetaEnd == G_PI)
        u_offset = - 0.5 / widthSegments;

      for (x = 0; x <= widthSegments; x++)
        {
          float u;
          float vx, vy, vz;
          graphene_vec3_t normalv;

          u = (float)x / widthSegments;

          vx = - radius * cos (phiStart + u * phiLength) * sin (thetaStart + v * thetaLength);
          vy = radius * cos (thetaStart + v * thetaLength);
          vz = radius * sin (phiStart + u * phiLength) * sin (thetaStart + v * thetaLength);

          push3 (positions, vx, vy, vz);

          graphene_vec3_init (&normalv, vx, vy, vz);
          graphene_vec3_normalize (&normalv, &normalv);

          push_vec3 (normals, &normalv);

          push2 (uvs, u + u_offset, 1 - v);

          vertices[x + y * vertices_w] = vertex_count++;
        }
    }

  for (y = 0; y < heightSegments; y++)
    {
      for (x = 0; x < widthSegments; x++)
        {
          int a = vertices[(y    ) * vertices_w + (x + 1)];
          int b = vertices[(y    ) * vertices_w + (x    )];
          int c = vertices[(y + 1) * vertices_w + (x    )];
          int d = vertices[(y + 1) * vertices_w + (x + 1)];

          if (y != 0 || thetaStart > 0)
            push3i (index, a, b, d);
          if (y != heightSegments - 1  || thetaEnd < G_PI)
            push3i (index, b, c, d);
        }
    }

  add_indexv (geometry, index);
  add_positionv (geometry, positions);
  add_normalv (geometry, normals);
  add_uvv (geometry, uvs);

  graphene_sphere_init (&bound, graphene_point3d_init (&center, 0, 0, 0), radius);
  gthree_geometry_set_bounding_sphere  (geometry, &bound);

  return geometry;
}

GthreeGeometry *
gthree_geometry_new_sphere (float radius,
                            int   widthSegments,
                            int   heightSegments)
{
  return gthree_geometry_new_sphere_full (radius, widthSegments, heightSegments,
                                          0, 2 * G_PI, 0, G_PI);
}

static void
generate_cylinder_cap (gboolean        top,
                       GthreeGeometry *geometry,
                       float           height,
                       float           radius,
                       int             radialSegments,
                       float           thetaStart,
                       float           thetaLength,
                       GArray         *positions,
                       GArray         *normals,
                       GArray         *uvs,
                       GArray         *index)
{
  int center_index_start, center_index_end, x;
  int group_start;
  float sign = top ? 1 : -1;
  float half_height = top ? 0.5 * height : -0.5 * height;

  // save the index of the first center vertex
  center_index_start = positions->len / 3;

  // first we generate the center vertex data of the cap.
  // because the geometry needs one set of uvs per face,
  // we must generate a center vertex per face/segment

  for (x = 1; x <= radialSegments; x++)
    {
      push3 (positions, 0, half_height, 0);
      push3 (normals, 0, sign, 0);
      push2 (uvs, 0.5, 0.5);
  }

  center_index_end = positions->len / 3;

  // now we generate the surrounding vertices, normals and uvs
  group_start = index->len;

  for (x = 0; x <= radialSegments; x++)
    {
      float u = (float)x / radialSegments;
      float theta = u * thetaLength + thetaStart;

      float cosTheta = cosf (theta);
      float sinTheta = sinf (theta);

      // vertex
      push3 (positions,
             radius * sinTheta,
             half_height,
             radius * cosTheta);

      // normal
      push3 (normals, 0, sign, 0);

      // uv
      push2 (uvs,
             (cosTheta * 0.5) + 0.5,
             (sinTheta * 0.5 * sign) + 0.5);
    }

  for (x = 0; x < radialSegments; x++)
    {
      int c = center_index_start + x;
      int i = center_index_end + x;

      if (top)
        push3i (index, i, i +1, c);
      else
        push3i (index, i + 1, i, c);
    }

  gthree_geometry_add_group (geometry, group_start, index->len - group_start, top ? 1 : 2);
}


GthreeGeometry *
gthree_geometry_new_cylinder_full (float    radiusTop,
                                   float    radiusBottom,
                                   float    height,
                                   int      radialSegments,
                                   int      heightSegments,
                                   gboolean openEnded,
                                   float    thetaStart,
                                   float    thetaLength)
{
  GthreeGeometry *geometry;
  int x, y;
  float tanTheta;
  gboolean has_top, has_bottom;
  graphene_vec3_t *the_normals;
  GArray *positions, *normals, *uvs, *index;
  int group_start;

  positions = g_array_new (FALSE, FALSE, sizeof (float));
  normals = g_array_new (FALSE, FALSE, sizeof (float));
  uvs = g_array_new (FALSE, FALSE, sizeof (float));
  index = g_array_new (FALSE, FALSE, sizeof (guint16));

  geometry = g_object_new (gthree_geometry_get_type (), NULL);

  radialSegments = MAX(radialSegments, 3);
  heightSegments = MAX(heightSegments, 1);

  has_top = !openEnded && radiusTop > 0;
  has_bottom = !openEnded && radiusBottom > 0;

  the_normals = g_newa (graphene_vec3_t, radialSegments + 1);
  tanTheta = (radiusBottom - radiusTop) / height;

  for (y = 0; y <= heightSegments; y++)
    {
      float v = y * 1.0 / heightSegments;
      float radius = v * (radiusBottom - radiusTop) + radiusTop;
      float vy = (0.5 - v) * height;

      for (x = 0; x <= radialSegments; x++)
        {
          float u = x * 1.0 / radialSegments;
          float angle = u * thetaLength + thetaStart;
          float vx, vz;
          float nx, ny, nz;

          vx = radius * sin (angle);
          vz = radius * cos (angle);

          push3 (positions, vx, vy, vz);

          if (y == 0)
            {
              nx = vx;
              nz = vz;
              ny = sqrt (nx * nx + nz * nz) * tanTheta;
              graphene_vec3_init (&the_normals[x], nx, ny, nz);
              graphene_vec3_normalize (&the_normals[x], &the_normals[x]);
            }
          push_vec3 (normals, &the_normals[x]);
          push2 (uvs, u, v);
        }
    }

  group_start = index->len;

  for (y = 0; y < heightSegments; y++)
    {
      for (x = 0; x < radialSegments; x++)
        {
          int a = x + y * (radialSegments + 1);
          int b = x + (y + 1) * (radialSegments + 1);
          int c = x + 1 + (y + 1) * (radialSegments + 1);
          int d = x + 1 + y * (radialSegments + 1);

          push3i (index, a, b, d);
          push3i (index, b, c, d);
        }
    }

  gthree_geometry_add_group (geometry, group_start, index->len - group_start, 0);

  if (has_top)
    generate_cylinder_cap (TRUE,
                           geometry,
                           height, radiusTop,
                           radialSegments,
                           thetaStart, thetaLength,
                           positions, normals, uvs, index);

  if (has_bottom)
    generate_cylinder_cap (FALSE,
                           geometry,
                           height, radiusBottom,
                           radialSegments,
                           thetaStart, thetaLength,
                           positions, normals, uvs, index);

  add_indexv (geometry, index);
  add_positionv (geometry, positions);
  add_normalv (geometry, normals);
  add_uvv (geometry, uvs);

  return geometry;
};

GthreeGeometry *
gthree_geometry_new_cylinder (float radius,
                              float height)
{
  return gthree_geometry_new_cylinder_full (radius, radius, height, 8, 1, FALSE, 0, 2 * G_PI);
}

GthreeGeometry *
gthree_geometry_new_torus_full (float radius,
                                float tube,
                                int   radialSegments,
                                int   tubularSegments,
                                float arc)
{
  GthreeGeometry *geometry;
  GArray *positions, *normals, *uvs, *index;
  int i, j;

  geometry = g_object_new (gthree_geometry_get_type (), NULL);

  positions = g_array_new (FALSE, FALSE, sizeof (float));
  normals = g_array_new (FALSE, FALSE, sizeof (float));
  uvs = g_array_new (FALSE, FALSE, sizeof (float));
  index = g_array_new (FALSE, FALSE, sizeof (guint16));

  for (j = 0; j <= radialSegments; j++)
    {
      for (i = 0; i <= tubularSegments; i++)
        {
          float u = i * arc / tubularSegments;
          float v = j * 2 * G_PI / radialSegments;
          graphene_vec3_t normal, center;
          graphene_vec3_t vertex;

          graphene_vec3_init (&vertex,
                              (radius + tube * cos (v)) * cos (u),
                              (radius + tube * cos (v)) * sin (u),
                              tube * sin (v));

          push_vec3 (positions, &vertex);

          graphene_vec3_init (&center, radius * cos (u), radius * sin (u), 0);
          graphene_vec3_subtract (&vertex, &center, &normal);
          graphene_vec3_normalize (&normal, &normal);

          push_vec3 (normals, &normal);

          push2 (uvs, i * 1.0 / tubularSegments, j * 1.0 / tubularSegments);
        }
    }

  for (j = 1; j <= radialSegments; j++)
    {
      for (i = 1; i <= tubularSegments; i++)
        {
          int a = (tubularSegments + 1) * j + i - 1;
          int b = (tubularSegments + 1) * (j - 1) + i - 1;
          int c = (tubularSegments + 1) * (j - 1) + i;
          int d = (tubularSegments + 1) * j + i;

          push3i (index, a, b, d);
          push3i (index, b, c, d);
        }
    }

  add_indexv (geometry, index);
  add_positionv (geometry, positions);
  add_normalv (geometry, normals);
  add_uvv (geometry, uvs);

  return geometry;
};

GthreeGeometry *
gthree_geometry_new_torus (float radius,
                           float tube)
{
  return gthree_geometry_new_torus_full (radius, tube, 8, 6, 2 * G_PI);
}

GthreeGeometry *
gthree_geometry_new_circle_full (float    radius,
                                 int      segments,
                                 float    thetaStart,
                                 float    thetaLength)
{
  GthreeGeometry *geometry;
  int s, i;
  GArray *positions, *normals, *uvs, *index;

  positions = g_array_new (FALSE, FALSE, sizeof (float));
  normals = g_array_new (FALSE, FALSE, sizeof (float));
  uvs = g_array_new (FALSE, FALSE, sizeof (float));
  index = g_array_new (FALSE, FALSE, sizeof (guint16));

  geometry = g_object_new (gthree_geometry_get_type (), NULL);

  segments = MAX(segments, 3);

  // center point

  push3 (positions, 0, 0, 0);
  push3 (normals, 0, 0, 1);
  push2 (uvs,  0.5, 0.5);

  for ( s = 0, i = 3; s <= segments; s ++, i += 3 )
    {
      float segment = thetaStart + (float)s / segments * thetaLength;
      float vx = radius * cosf (segment);
      float vy = radius * sinf (segment);

      // vertex
      push3 (positions, vx, vy, 0);

      // normal
      push3 (normals, 0, 0, 1);

      // uvs
      push2 (uvs,
             (vx / radius + 1) / 2,
             (vy / radius + 1 ) / 2);
    }

  // indices
  for (i = 1; i <= segments; i ++)
    push3i (index, i, i + 1, 0);

  add_indexv (geometry, index);
  add_positionv (geometry, positions);
  add_normalv (geometry, normals);
  add_uvv (geometry, uvs);

  return geometry;
};

GthreeGeometry *
gthree_geometry_new_circle (float    radius,
                            int      segments)
{
  return gthree_geometry_new_circle_full (radius, segments, 0, 2 * G_PI);
}


typedef struct {
  graphene_vec3_t position;
  graphene_vec3_t normal;
} DecalVertex;

static void
push_decal_vertex (GArray *decal_vertices,
                   const graphene_matrix_t *matrix_world,
                   const graphene_matrix_t *object_to_projector_matrix,
                   DecalVertex v)
{
  graphene_vec4_t v4;

  /* Transform to projection space */
  graphene_vec4_init_from_vec3 (&v4, &v.position, 1.0);
  graphene_matrix_transform_vec4 (object_to_projector_matrix, &v4, &v4);
  graphene_vec4_get_xyz (&v4, &v.position);

  graphene_matrix_transform_vec3 (matrix_world, &v.normal, &v.normal);

  g_array_append_val (decal_vertices, v);
}


static GArray *
get_decal_vertices_from_geometry (GthreeGeometry *geometry,
                                  const graphene_matrix_t *matrix_world,
                                  const graphene_matrix_t *projector_matrix)
{
  GArray *decal_vertices = g_array_new (FALSE, TRUE, sizeof (DecalVertex));
  GthreeAttribute *position_attribute = gthree_geometry_get_position (geometry);
  GthreeAttribute *normal_attribute = gthree_geometry_get_normal (geometry);
  GthreeAttribute *index = gthree_geometry_get_index (geometry);
  graphene_matrix_t projector_matrix_inverse;
  graphene_matrix_t object_to_projector_matrix;
  int i;

  graphene_matrix_inverse (projector_matrix, &projector_matrix_inverse);

  // transform the vertex to world space, then to projector space
  graphene_matrix_multiply (matrix_world, &projector_matrix_inverse, &object_to_projector_matrix);

  if (index != NULL)
    {
      // indexed geometry
      int count = gthree_attribute_get_count (index);

      for (i = 0; i < count; i++)
        {
          int idx = gthree_attribute_get_uint (index, i);
          DecalVertex v = { 0 };

          gthree_attribute_get_vec3 (position_attribute, idx, &v.position);
          if (normal_attribute)
            gthree_attribute_get_vec3 (normal_attribute, idx, &v.normal);

          push_decal_vertex (decal_vertices, matrix_world, &object_to_projector_matrix, v);
        }
    }
  else
    {
      // non-indexed geometry
      int count = gthree_attribute_get_count (position_attribute);
      int idx;

      for (idx = 0; idx < count; idx++)
        {
          DecalVertex v = { 0 };

          gthree_attribute_get_vec3 (position_attribute, idx, &v.position);
          if (normal_attribute)
            gthree_attribute_get_vec3 (normal_attribute, idx, &v.normal);

          push_decal_vertex (decal_vertices, matrix_world, &object_to_projector_matrix, v);
        }
    }

  return decal_vertices;
}

static DecalVertex
clip (const DecalVertex *v0, const DecalVertex *v1, const graphene_vec3_t *plane, float s)
{
  DecalVertex v;

  float d0 = graphene_vec3_dot (&v0->position, plane) - s;
  float d1 = graphene_vec3_dot (&v1->position, plane) - s;

  float s0 = d0 / ( d0 - d1 );

  graphene_vec3_interpolate (&v0->position, &v1->position, s0, &v.position);
  graphene_vec3_interpolate (&v0->normal, &v1->normal, s0, &v.normal);

  return v;
}

static void
clip_geometry (GArray *in,
               GArray *out,
               const graphene_vec3_t *size,
               float plane_x,
               float plane_y,
               float plane_z)
{
  graphene_vec3_t plane;
  int i;

  graphene_vec3_init (&plane, plane_x, plane_y, plane_z);
  g_array_set_size (out, 0);

  float s = 0.5 * fabsf (graphene_vec3_dot (size, &plane));

  for (i = 0; i < in->len; i += 3)
    {
      int total = 0;
      DecalVertex nV1, nV2, nV3, nV4;

      float d1 = graphene_vec3_dot (&g_array_index (in, DecalVertex, i + 0).position, &plane) - s;
      float d2 = graphene_vec3_dot (&g_array_index (in, DecalVertex, i + 1).position, &plane) - s;
      float d3 = graphene_vec3_dot (&g_array_index (in, DecalVertex, i + 2).position, &plane) - s;

      gboolean v1Out = d1 > 0;
      gboolean v2Out = d2 > 0;
      gboolean v3Out = d3 > 0;

      // calculate, how many vertices of the face lie outside of the clipping plane
      total = ( v1Out ? 1 : 0 ) + ( v2Out ? 1 : 0 ) + ( v3Out ? 1 : 0 );
      switch (total)
        {
        case 0:
          // the entire face lies inside of the plane, no clipping needed

          g_array_append_val (out, g_array_index (in, DecalVertex, i));
          g_array_append_val (out, g_array_index (in, DecalVertex, i + 1));
          g_array_append_val (out, g_array_index (in, DecalVertex, i + 2));
          break;

        case 1:
          // one vertex lies outside of the plane, perform clipping

          if (v1Out)
            {
              nV1 = g_array_index (in, DecalVertex, i + 1);
              nV2 = g_array_index (in, DecalVertex, i + 2);
              nV3 = clip (&g_array_index (in, DecalVertex, i), &nV1, &plane, s);
              nV4 = clip (&g_array_index (in, DecalVertex, i), &nV2, &plane, s);
            }

          if (v2Out)
            {
              nV1 = g_array_index (in, DecalVertex, i);
              nV2 = g_array_index (in, DecalVertex, i + 2);
              nV3 = clip (&g_array_index (in, DecalVertex, i + 1), &nV1, &plane, s);
              nV4 = clip (&g_array_index (in, DecalVertex, i + 1), &nV2, &plane, s);

              g_array_append_val (out, nV3);
              g_array_append_val (out, nV2);
              g_array_append_val (out, nV1);

              g_array_append_val (out, nV2);
              g_array_append_val (out, nV3);
              g_array_append_val (out, nV4);
              break;
            }

          if (v3Out)
            {
              nV1 = g_array_index (in, DecalVertex, i);
              nV2 = g_array_index (in, DecalVertex, i + 1);
              nV3 = clip (&g_array_index (in, DecalVertex, i + 2), &nV1, &plane, s);
              nV4 = clip (&g_array_index (in, DecalVertex, i + 2), &nV2, &plane, s);
            }

          g_array_append_val (out, nV1);
          g_array_append_val (out, nV2);
          g_array_append_val (out, nV3);

          g_array_append_val (out, nV4);
          g_array_append_val (out, nV3);
          g_array_append_val (out, nV2);

          break;

        case 2:
          // two vertices lies outside of the plane, perform clipping
          if (!v1Out)
            {
              nV1 = g_array_index (in, DecalVertex, i);
              nV2 = clip (&nV1, &g_array_index (in, DecalVertex, i + 1), &plane, s);
              nV3 = clip (&nV1, &g_array_index (in, DecalVertex, i + 2), &plane, s);
            }

          if (!v2Out)
            {
              nV1 = g_array_index (in, DecalVertex, i + 1);
              nV2 = clip (&nV1, &g_array_index (in, DecalVertex, i + 2), &plane, s);
              nV3 = clip (&nV1, &g_array_index (in, DecalVertex, i), &plane, s);
            }

          if (!v3Out)
            {
              nV1 = g_array_index (in, DecalVertex, i + 2);
              nV2 = clip (&nV1, &g_array_index (in, DecalVertex, i), &plane, s);
              nV3 = clip (&nV1, &g_array_index (in, DecalVertex, i + 1), &plane, s);
            }

          g_array_append_val (out, nV1);
          g_array_append_val (out, nV2);
          g_array_append_val (out, nV3);
          break;

        case 3:
          // the entire face lies outside of the plane, so let's discard the corresponding vertices
          break;
        }
    }
}

/*
 * You can use this geometry to create a decal mesh, that serves different kinds of purposes.
 * e.g. adding unique details to models, performing dynamic visual environmental changes or covering seams.
 *
 * position — Position of the decal projector
 * orientation — Orientation of the decal projector
 * size — Size of the decal projector
 *
 * reference: http://blog.wolfire.com/2009/06/how-to-project-decals/
 */
GthreeGeometry *
gthree_geometry_new_decal (GthreeGeometry *original_geometry,
                           const graphene_matrix_t *matrix_world,
                           const graphene_vec3_t *position,
                           const graphene_quaternion_t *orientation,
                           const graphene_vec3_t *size)
{
  GthreeGeometry *geometry;
  g_autoptr(GArray) decal_vertices = NULL;
  g_autoptr(GArray) decal_vertices2 = NULL;
  graphene_matrix_t projector_matrix;
  graphene_point3d_t p3d;
  g_autoptr(GthreeAttribute) a_position = NULL;
  g_autoptr(GthreeAttribute) a_normal = NULL;
  g_autoptr(GthreeAttribute) a_uv = NULL;
  int len;

  graphene_matrix_init_identity (&projector_matrix);
  graphene_matrix_rotate_quaternion (&projector_matrix, orientation);
  graphene_matrix_translate (&projector_matrix,
                             graphene_point3d_init_from_vec3 (&p3d, position));

  // first, create an array of 'DecalVertex'
  // three consecutive 'DecalVertex' represent a single face
  //
  // this data structure will be later used to perform the clipping
  decal_vertices = get_decal_vertices_from_geometry (original_geometry, matrix_world, &projector_matrix);

  // second, clip the geometry so that it doesn't extend out from the projector
  // We swap between the two arrays to avoid reallocating
  decal_vertices2 = g_array_new (FALSE, TRUE, sizeof (DecalVertex));

  clip_geometry (decal_vertices, decal_vertices2, size,  1, 0, 0);
  clip_geometry (decal_vertices2, decal_vertices, size, -1, 0, 0);
  clip_geometry (decal_vertices, decal_vertices2, size, 0,  1, 0);
  clip_geometry (decal_vertices2, decal_vertices, size, 0, -1, 0);
  clip_geometry (decal_vertices, decal_vertices2, size, 0, 0,  1);
  clip_geometry (decal_vertices2, decal_vertices, size, 0, 0, -1);

  // third, generate final vertices, normals and uvs
  geometry = gthree_geometry_new ();

  len = decal_vertices->len;
  a_position = gthree_attribute_new ("position", GTHREE_ATTRIBUTE_TYPE_FLOAT, len, 3, FALSE);
  gthree_geometry_add_attribute (geometry, "position", a_position);

  if (gthree_geometry_get_normal (original_geometry) != NULL)
    {
      a_normal = gthree_attribute_new ("normal", GTHREE_ATTRIBUTE_TYPE_FLOAT, len, 3, FALSE);
      gthree_geometry_add_attribute (geometry, "normal", a_normal);
    }

  a_uv = gthree_attribute_new ("uv", GTHREE_ATTRIBUTE_TYPE_FLOAT, len, 2, FALSE);
  gthree_geometry_add_attribute (geometry, "uv", a_uv);

  for (int i = 0; i < decal_vertices->len; i++)
    {
      const DecalVertex *decalVertex = &g_array_index (decal_vertices, DecalVertex, i);
      graphene_vec4_t v4;
      graphene_vec3_t world;

      // create texture coordinates (we are still in projector space)
      gthree_attribute_set_xy (a_uv, i,
                               0.5 + ( graphene_vec3_get_x (&decalVertex->position) / graphene_vec3_get_x (size)),
                               0.5 + ( graphene_vec3_get_y (&decalVertex->position) / graphene_vec3_get_y (size)));


      // transform the vertex back to world space
      graphene_vec4_init_from_vec3 (&v4, &decalVertex->position, 1.0);
      graphene_matrix_transform_vec4 (&projector_matrix, &v4, &v4);
      graphene_vec4_get_xyz (&v4, &world);

      // now create vertex and normal buffer data
      gthree_attribute_set_vec3 (a_position, i, &world);
      if (a_normal)
        gthree_attribute_set_vec3 (a_normal, i, &decalVertex->normal);
    }

  return geometry;
}

GthreeGeometry *
gthree_geometry_new_decal_from_mesh (GthreeMesh *mesh,
                                     const graphene_vec3_t *position,
                                     const graphene_quaternion_t *orientation,
                                     const graphene_vec3_t *size)
{
  return gthree_geometry_new_decal (gthree_mesh_get_geometry (mesh),
                                    gthree_object_get_world_matrix (GTHREE_OBJECT (mesh)),
                                    position, orientation, size);
}
