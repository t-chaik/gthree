#include <math.h>
#include <epoxy/gl.h>

#include "gthreeresource.h"
#include "gthreeprivate.h"
#include "gthreeenums.h"

typedef struct _ListNode ListNode;

/* This is a circular doubly linked list where the first is the head */
struct _ListNode {
  ListNode *next;
  ListNode *prev;
};

static ListNode *
list_init (ListNode *head)
{
  head->next = head;
  head->prev = head;
  return head;
}

ListNode *
list_head_new (void)
{
  ListNode *head = g_new0 (ListNode, 1);
  return list_init (head);
}

static void
list_node_insert_before (ListNode *old_node,
                         ListNode *new_node)
{
  ListNode *old_prev = old_node->prev;

  new_node->next = old_node;
  new_node->prev = old_prev;

  old_node->prev = new_node;
  old_prev->next = new_node;
}

static void
list_node_unlink (ListNode *node)
{
  ListNode *prev = node->prev;
  ListNode *next = node->next;

  g_assert (node->next != NULL);
  g_assert (node->prev != NULL);

  prev->next = next;
  next->prev = prev;

  node->prev = NULL;
  node->next = NULL;
}

static void
list_append (ListNode *head,
             ListNode *element)
{
  list_node_insert_before (head, element);
}

static GQuark list_head_q;
static GQuark lazy_deletes_q;

static void
renderer_init (void)
{
  if (list_head_q != 0)
    return;

  list_head_q = g_quark_from_static_string ("gthree-resource-head");
  lazy_deletes_q = g_quark_from_static_string ("gthree-resource-lazy-deletes");
}

static ListNode *
renderer_get_list_head (GthreeRenderer  *renderer)
{
  ListNode *head;

  head = g_object_get_qdata (G_OBJECT (renderer), list_head_q);
  if (head == NULL)
    {
      head = list_head_new ();
      g_object_set_qdata_full (G_OBJECT (renderer),  list_head_q, head, g_free);
    }

  return head;
}

typedef struct {
  GthreeRenderer *renderer;
  gboolean used;

  ListNode resource_list;
} GthreeResourcePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GthreeResource, gthree_resource, G_TYPE_OBJECT);


static void
gthree_resource_init (GthreeResource *resource)
{
}

static void
gthree_resource_finalize (GObject *obj)
{
  GthreeResource *resource = GTHREE_RESOURCE (obj);
  GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);

  if (priv->renderer != NULL)
    gthree_resource_unrealize (resource, priv->renderer);

  g_assert (priv->renderer == NULL);

  G_OBJECT_CLASS (gthree_resource_parent_class)->finalize (obj);
}

static void
gthree_resource_real_set_used (GthreeResource *resource,
                               gboolean used)
{
  GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);
  priv->used = used;
}

static void
gthree_resource_class_init (GthreeResourceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  renderer_init ();

  gobject_class->finalize = gthree_resource_finalize;
  klass->set_used = gthree_resource_real_set_used;
}

gboolean
gthree_resource_is_realized (GthreeResource *resource)
{
  GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);

  return priv->renderer != NULL;
}

static GthreeResource *
node_to_resource (ListNode *node)
{
  GthreeResource *resource;

  gsize offset = G_STRUCT_OFFSET(GthreeResourcePrivate, resource_list) + GthreeResource_private_offset;

  resource = G_STRUCT_MEMBER_P (node, -offset);

  /* TODO: Remove checks */
  {
    GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);
    g_assert (&priv->resource_list == node);
  }

  return resource;
}

void
gthree_resources_unrealize_all_for (GthreeRenderer *renderer)
{
  ListNode *head, *node;

  renderer_init ();

  gthree_renderer_push_current (renderer);

  head = renderer_get_list_head (renderer);
  node = head->next;
  while (node != head)
    {
      GthreeResource *resource = node_to_resource (node);

      /* Step to next before unrealizing and unlinking */
      node = node->next;

      gthree_resource_unrealize (resource, renderer);
    }

  gthree_resources_flush_deletes (renderer);

  gthree_renderer_pop_current (renderer);
}

void
gthree_resources_set_all_unused_for (GthreeRenderer *renderer)
{
  ListNode *head, *node;

  renderer_init ();

  head = renderer_get_list_head (renderer);
  node = head->next;
  while (node != head)
    {
      GthreeResource *resource = node_to_resource (node);
      GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);

      priv->used = FALSE;
      node = node->next;
    }
}

void
gthree_resources_unrealize_unused_for (GthreeRenderer *renderer)
{
  ListNode *head, *node;

  renderer_init ();

  gthree_renderer_push_current (renderer);

  head = renderer_get_list_head (renderer);
  node = head->next;
  while (node != head)
    {
      GthreeResource *resource = node_to_resource (node);
      GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);

      /* Step to next before unrealizing and unlinking */
      node = node->next;

      if (!priv->used)
        gthree_resource_unrealize (resource, renderer);
    }

  gthree_resources_flush_deletes (renderer);

  gthree_renderer_pop_current (renderer);
}

void
gthree_resource_set_realized_for (GthreeResource *resource,
                                  GthreeRenderer   *renderer)
{
  GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);
  ListNode *head;

  g_assert (renderer != NULL);
  g_assert (priv->renderer == NULL);

  priv->renderer = g_object_ref (renderer);

  head = renderer_get_list_head (renderer);
  list_append (head, &priv->resource_list);
}

void
gthree_resource_unrealize (GthreeResource *resource,
                           GthreeRenderer *renderer)
{
  GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);
  GthreeResourceClass *class = GTHREE_RESOURCE_GET_CLASS(resource);

  g_assert (priv->renderer == renderer);

  class->unrealize (resource, renderer);

  list_node_unlink (&priv->resource_list);

  g_object_unref (priv->renderer);
  priv->renderer = NULL;
}

gboolean
gthree_resource_get_used (GthreeResource *resource)
{
  GthreeResourcePrivate *priv = gthree_resource_get_instance_private (resource);

  return priv->used;
}

void
gthree_resource_set_used (GthreeResource *resource,
                          gboolean used)
{

  GthreeResourceClass *class = GTHREE_RESOURCE_GET_CLASS(resource);
  class->set_used (resource, used);
}

static void
do_delete (GthreeResourceKind kind,
           guint id)
{
  switch (kind)
    {
    case GTHREE_RESOURCE_KIND_TEXTURE:
      glDeleteTextures (1, &id);
      break;
    case GTHREE_RESOURCE_KIND_BUFFER:
      glDeleteBuffers (1, &id);
      break;
    case GTHREE_RESOURCE_KIND_FRAMEBUFFER:
      glDeleteFramebuffers (1, &id);
      break;
    case GTHREE_RESOURCE_KIND_RENDERBUFFER:
      glDeleteRenderbuffers (1, &id);
      break;
    }
}

struct LazyDelete {
  GthreeResourceKind kind;
  guint id;
};


static GArray *
renderer_get_lazy_deletes (GthreeRenderer  *renderer)
{
  GArray *array;

  array = g_object_get_qdata (G_OBJECT (renderer), lazy_deletes_q);
  if (array == NULL)
    {
      array = g_array_new (FALSE, FALSE, sizeof (struct LazyDelete));
      g_object_set_qdata_full (G_OBJECT (renderer),  lazy_deletes_q, array, (GDestroyNotify)g_array_unref);
    }

  return array;
}

void
gthree_resource_lazy_delete (GthreeResource *resource,
                             GthreeRenderer *renderer,
                             GthreeResourceKind kind,
                             guint             id)
{
  if (gthree_renderer_get_current () == renderer)
    do_delete (kind, id);
  else
    {
      GArray *array = renderer_get_lazy_deletes (renderer);
      struct LazyDelete lazy = {kind, id};
      g_array_append_val (array, lazy);
    }
}

void
gthree_resources_flush_deletes (GthreeRenderer *renderer)
{
  GArray *array = renderer_get_lazy_deletes (renderer);
  int i;

  for (i = 0; i < array->len; i++)
    {
      struct LazyDelete *lazy = &g_array_index (array, struct LazyDelete, i);
      do_delete (lazy->kind, lazy->id);
    }

  g_array_set_size (array, 0);
}
