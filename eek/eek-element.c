/* 
 * Copyright (C) 2010-2011 Daiki Ueno <ueno@unixuser.org>
 * Copyright (C) 2010-2011 Red Hat, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/**
 * SECTION:eek-element
 * @short_description: Base class of a keyboard element
 *
 * The #EekElementClass class represents a keyboard element, which
 * shall be used to implement #EekKeyboard, #EekSection, or #EekKey.
 */

#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif  /* HAVE_CONFIG_H */

#include "eek-element.h"
#include "eek-container.h"
#include "eek-serializable.h"

enum {
    PROP_0,
    PROP_NAME,
    PROP_BOUNDS,
    PROP_LAST
};

static void eek_serializable_iface_init (EekSerializableIface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (EekElement, eek_element, G_TYPE_OBJECT,
                                  G_IMPLEMENT_INTERFACE (EEK_TYPE_SERIALIZABLE,
                                                         eek_serializable_iface_init));

#define EEK_ELEMENT_GET_PRIVATE(obj)                                  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), EEK_TYPE_ELEMENT, EekElementPrivate))


struct _EekElementPrivate
{
    gchar *name;
    EekBounds bounds;
    EekElement *parent;
};

static GVariant *
_g_variant_new_bounds (EekBounds *bounds)
{
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("ad"));
    g_variant_builder_add (&builder, "d", bounds->x);
    g_variant_builder_add (&builder, "d", bounds->y);
    g_variant_builder_add (&builder, "d", bounds->width);
    g_variant_builder_add (&builder, "d", bounds->height);

    return g_variant_builder_end (&builder);
}

static void
_g_variant_get_bounds (GVariant *variant, EekBounds *bounds)
{
    g_variant_get_child (variant, 0, "d", &bounds->x);
    g_variant_get_child (variant, 1, "d", &bounds->y);
    g_variant_get_child (variant, 2, "d", &bounds->width);
    g_variant_get_child (variant, 3, "d", &bounds->height);
}

static void
eek_element_real_serialize (EekSerializable *self,
                            GVariantBuilder *builder)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);

    g_variant_builder_add (builder, "s", priv->name == NULL ? "" : priv->name);
    g_variant_builder_add (builder, "v", _g_variant_new_bounds (&priv->bounds));
}

static gsize
eek_element_real_deserialize (EekSerializable *self,
                              GVariant        *variant,
                              gsize            index)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);
    GVariant *bounds;

    g_variant_get_child (variant, index++, "s", &priv->name);
    g_variant_get_child (variant, index++, "v", &bounds);
    _g_variant_get_bounds (bounds, &priv->bounds);

    return index;
}

static void
eek_serializable_iface_init (EekSerializableIface *iface)
{
    iface->serialize = eek_element_real_serialize;
    iface->deserialize = eek_element_real_deserialize;
}

static void
eek_element_real_set_parent (EekElement *self,
                             EekElement *parent)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);

    if (!parent) {
        g_return_if_fail (priv->parent);
        /* release self-reference acquired when setting parent */
        g_object_unref (self);
        priv->parent = NULL;
    } else {
        g_return_if_fail (!priv->parent);
        g_object_ref (self);
        priv->parent = parent;
    }
}

static EekElement *
eek_element_real_get_parent (EekElement *self)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);

    return priv->parent;
}

static void
eek_element_real_set_name (EekElement  *self,
                           const gchar *name)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);

    g_free (priv->name);
    priv->name = g_strdup (name);

    g_object_notify (G_OBJECT(self), "name");
}

static G_CONST_RETURN gchar *
eek_element_real_get_name (EekElement *self)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);

    return priv->name;
}

static void
eek_element_real_set_bounds (EekElement *self,
                             EekBounds *bounds)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);

    priv->bounds = *bounds;
}

static void
eek_element_real_get_bounds (EekElement *self,
                             EekBounds  *bounds)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(self);

    g_return_if_fail (bounds);
    *bounds = priv->bounds;

    g_object_notify (G_OBJECT(self), "bounds");
}

static void
eek_element_finalize (GObject *object)
{
    EekElementPrivate *priv = EEK_ELEMENT_GET_PRIVATE(object);

    g_free (priv->name);
    G_OBJECT_CLASS (eek_element_parent_class)->finalize (object);
}

static void
eek_element_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_NAME:
        eek_element_set_name (EEK_ELEMENT(object),
                              g_value_get_string (value));
        break;
    case PROP_BOUNDS:
        eek_element_set_bounds (EEK_ELEMENT(object),
                                g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
eek_element_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    EekBounds  bounds;

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, eek_element_get_name (EEK_ELEMENT(object)));
        break;
    case PROP_BOUNDS:
        eek_element_get_bounds (EEK_ELEMENT(object), &bounds);
        g_value_set_boxed (value, &bounds);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
eek_element_class_init (EekElementClass *klass)
{
    GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);
    GParamSpec        *pspec;

    g_type_class_add_private (gobject_class,
                              sizeof (EekElementPrivate));

    klass->set_parent = eek_element_real_set_parent;
    klass->get_parent = eek_element_real_get_parent;
    klass->set_name = eek_element_real_set_name;
    klass->get_name = eek_element_real_get_name;
    klass->set_bounds = eek_element_real_set_bounds;
    klass->get_bounds = eek_element_real_get_bounds;

    gobject_class->set_property = eek_element_set_property;
    gobject_class->get_property = eek_element_get_property;
    gobject_class->finalize     = eek_element_finalize;

    /**
     * EekElement:name:
     *
     * The name of #EekElement.
     */
    pspec = g_param_spec_string ("name",
                                 "Name",
                                 "Name",
                                 NULL,
                                 G_PARAM_READWRITE);
    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     pspec);

    /**
     * EekElement:bounds:
     *
     * The bounding box of #EekElement.
     */
    pspec = g_param_spec_boxed ("bounds",
                                "Bounds",
                                "Bounding box of the element",
                                EEK_TYPE_BOUNDS,
                                G_PARAM_READWRITE);
    g_object_class_install_property (gobject_class,
                                     PROP_BOUNDS,
                                     pspec);
}

static void
eek_element_init (EekElement *self)
{
    EekElementPrivate *priv;

    priv = self->priv = EEK_ELEMENT_GET_PRIVATE(self);
    priv->name = NULL;
    memset (&priv->bounds, 0, sizeof priv->bounds);
}

/**
 * eek_element_set_parent:
 * @element: an #EekElement
 * @parent: an #EekElement
 *
 * Set the parent of @element to @parent.
 */
void
eek_element_set_parent (EekElement *element,
                        EekElement *parent)
{
    g_return_if_fail (EEK_IS_ELEMENT(element));
    g_return_if_fail (EEK_IS_ELEMENT(parent));
    EEK_ELEMENT_GET_CLASS(element)->set_parent (element, parent);
}

/**
 * eek_element_get_parent:
 * @element: an #EekElement
 *
 * Get the parent of @element.
 * Returns: an #EekElement if the parent is set
 */
EekElement *
eek_element_get_parent (EekElement *element)
{
    g_return_val_if_fail (EEK_IS_ELEMENT(element), NULL);
    return EEK_ELEMENT_GET_CLASS(element)->get_parent (element);
}

/**
 * eek_element_set_name:
 * @element: an #EekElement
 * @name: name of @element
 *
 * Set the name of @element to @name.
 */
void
eek_element_set_name (EekElement  *element,
                      const gchar *name)
{
    g_return_if_fail (EEK_IS_ELEMENT(element));
    EEK_ELEMENT_GET_CLASS(element)->set_name (element, name);
}

/**
 * eek_element_get_name:
 * @element: an #EekElement
 *
 * Get the name of @element.
 * Returns: the name of @element or NULL when the name is not set
 */
G_CONST_RETURN gchar *
eek_element_get_name (EekElement  *element)
{
    g_return_val_if_fail (EEK_IS_ELEMENT(element), NULL);
    return EEK_ELEMENT_GET_CLASS(element)->get_name (element);
}

/**
 * eek_element_set_bounds:
 * @element: an #EekElement
 * @bounds: bounding box of @element
 *
 * Set the bounding box of @element to @bounds.  Note that if @element
 * has parent, X and Y positions of @bounds are relative to the parent
 * position.
 */
void
eek_element_set_bounds (EekElement  *element,
                        EekBounds   *bounds)
{
    g_return_if_fail (EEK_IS_ELEMENT(element));
    EEK_ELEMENT_GET_CLASS(element)->set_bounds (element, bounds);
}

/**
 * eek_element_get_bounds:
 * @element: an #EekElement
 * @bounds: pointer where bounding box of @element will be stored
 *
 * Get the bounding box of @element.  Note that if @element has
 * parent, position of @bounds are relative to the parent.  To obtain
 * the absolute position, use eek_element_get_absolute_position().
 */
void
eek_element_get_bounds (EekElement  *element,
                        EekBounds   *bounds)
{
    g_return_if_fail (EEK_IS_ELEMENT(element));
    EEK_ELEMENT_GET_CLASS(element)->get_bounds (element, bounds);
}

/**
 * eek_element_get_absolute_position:
 * @element: an #EekElement
 * @x: pointer where the X coordinate of @element will be stored
 * @y: pointer where the Y coordinate of @element will be stored
 *
 * Compute the absolute position of @element.
 */
void
eek_element_get_absolute_position (EekElement *element,
                                   gdouble    *x,
                                   gdouble    *y)
{
    EekBounds bounds;
    gdouble ax = 0.0, ay = 0.0;

    do {
        eek_element_get_bounds (element, &bounds);
        ax += bounds.x;
        ay += bounds.y;
    } while ((element = eek_element_get_parent (element)) != NULL);
    *x = ax;
    *y = ay;
}

/**
 * eek_element_set_position:
 * @element: an #EekElement
 * @x: X coordinate of top left corner
 * @y: Y coordinate of top left corner
 *
 * Set the relative position of @element.
 */
void
eek_element_set_position (EekElement *element,
                          gdouble     x,
                          gdouble     y)
{
    EekBounds bounds;

    eek_element_get_bounds (element, &bounds);
    bounds.x = x;
    bounds.y = y;
    eek_element_set_bounds (element, &bounds);
}

/**
 * eek_element_set_size:
 * @element: an #EekElement
 * @width: width of @element
 * @height: height of @element
 *
 * Set the size of @element.
 */
void
eek_element_set_size (EekElement  *element,
                      gdouble      width,
                      gdouble      height)
{
    EekBounds bounds;

    eek_element_get_bounds (element, &bounds);
    bounds.width = width;
    bounds.height = height;
    eek_element_set_bounds (element, &bounds);
}
