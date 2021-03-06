#ifndef __GTHREE_RESOURCE_H__
#define __GTHREE_RESOURCE_H__

#if !defined (__GTHREE_H_INSIDE__) && !defined (GTHREE_COMPILATION)
#error "Only <gthree/gthree.h> can be included directly."
#endif

#include <glib-object.h>
#include <gthree/gthreetypes.h>

G_BEGIN_DECLS

#define GTHREE_TYPE_RESOURCE      (gthree_resource_get_type ())
#define GTHREE_RESOURCE(inst)     (G_TYPE_CHECK_INSTANCE_CAST ((inst), GTHREE_TYPE_RESOURCE, GthreeResource))
#define GTHREE_RESOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTHREE_TYPE_RESOURCE, GthreeResourceClass))
#define GTHREE_IS_RESOURCE(inst)  (G_TYPE_CHECK_INSTANCE_TYPE ((inst), GTHREE_TYPE_RESOURCE))
#define GTHREE_IS_RESOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTHREE_TYPE_RESOURCE))
#define GTHREE_RESOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTHREE_TYPE_RESOURCE, GthreeResourceClass))

struct _GthreeResource {
  GObject parent;
};

typedef struct {
  GObjectClass parent_class;

  guint32 realize_data_size;

  void (*set_used) (GthreeResource *resource,
                    gboolean        used);
  void (*unrealize) (GthreeResource *resource,
                     GthreeRenderer   *renderer);

  gpointer padding[8];
} GthreeResourceClass;

typedef struct {
  GthreeRenderer *realized_for;
  gboolean dirty;
} GthreeResourceRealizeData;

GTHREE_API
GType gthree_resource_get_type (void) G_GNUC_CONST;

GTHREE_API
void     gthree_resource_set_realized_for (GthreeResource  *resource,
                                           GthreeRenderer   *renderer);
GTHREE_API
gpointer gthree_resource_get_data_for  (GthreeResource  *resource,
                                        GthreeRenderer   *renderer);
GTHREE_API
gboolean gthree_resource_is_realized_for  (GthreeResource  *resource,
                                           GthreeRenderer   *renderer);
GTHREE_API
void     gthree_resource_unrealize        (GthreeResource  *resource,
                                           GthreeRenderer   *renderer);
GTHREE_API
gboolean gthree_resource_get_used        (GthreeResource  *resource);
GTHREE_API
void     gthree_resource_set_used         (GthreeResource  *resource,
                                           gboolean         used);

G_END_DECLS

#endif /* __GTHREE_RESOURCE_H__ */
