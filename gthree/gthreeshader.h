#ifndef __GTHREE_SHADER_H__
#define __GTHREE_SHADER_H__

#include <glib-object.h>
#include <graphene.h>

#include <gthree/gthreeuniforms.h>

G_BEGIN_DECLS

#define GTHREE_TYPE_SHADER      (gthree_shader_get_type ())
#define GTHREE_SHADER(inst)     (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                                                             GTHREE_TYPE_SHADER, \
                                                             GthreeShader))
#define GTHREE_IS_SHADER(inst)  (G_TYPE_CHECK_INSTANCE_TYPE ((inst),    \
                                                             GTHREE_TYPE_SHADER))


typedef struct {
  GObject parent;
} GthreeShader;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GthreeShader, g_object_unref)

typedef struct {
  GObjectClass parent_class;

} GthreeShaderClass;

GTHREE_API
GType gthree_shader_get_type (void) G_GNUC_CONST;

GTHREE_API
GthreeShader *  gthree_shader_new   (GPtrArray *defines,
                                     GthreeUniforms *uniforms,
                                     const char *vertex_shader_text,
                                     const char *fragment_shader_text);

GTHREE_API
GthreeShader *  gthree_shader_clone                                (GthreeShader  *shader);
GTHREE_API
GPtrArray      *gthree_shader_get_defines                          (GthreeShader  *shader);
GTHREE_API
void            gthree_shader_set_defines                          (GthreeShader  *shader,
                                                                    GPtrArray     *defines);
GTHREE_API
GthreeUniforms *gthree_shader_get_uniforms                         (GthreeShader  *shader);
GTHREE_API
const char *    gthree_shader_get_vertex_shader_text               (GthreeShader  *shader);
GTHREE_API
const char *    gthree_shader_get_fragment_shader_text             (GthreeShader  *shader);
GTHREE_API
void            gthree_shader_update_uniform_locations_for_program (GthreeShader  *shader,
                                                                    GthreeProgram *program);
GTHREE_API
gboolean        gthree_shader_equal                                (GthreeShader  *a,
                                                                    GthreeShader  *b);
GTHREE_API
guint           gthree_shader_hash                                 (GthreeShader  *shader);
GTHREE_API
void            gthree_shader_set_name                             (GthreeShader *shader,
                                                                    const char   *name);
GTHREE_API
const char *    gthree_shader_get_name                             (GthreeShader *shader);

GTHREE_API
GthreeShader *gthree_get_shader_from_library   (const char *name);
GTHREE_API
GthreeShader *gthree_clone_shader_from_library (const char *name);

GTHREE_API
GArray *gthree_convolution_shader_build_kernel (float sigma);

G_END_DECLS

#endif /* __GTHREE_SHADER_H__ */
