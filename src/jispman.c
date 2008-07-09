/* file jispman.c */
/*
 * kf linux jabber client
 * ----------------------
 *
 * Copyright (C) 2003-2004 Przemys³aw Sitek <psitek@rams.pl> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "jispman.h"
#include "emoticons.h"
#include "kf.h"

struct _KfJispManager {
	GHashTable *jisps;	/** A hash-table of all JISP pachages managed by this object, indexed by their filename */
	GList *patterns;	/** A list of patterns */
};


/**
 * \brief create a new instance of KfJispManager
 * \return a new instance of KfJispManager
 **/
KfJispManager *kf_jisp_manager_new (void)
{
	KfJispManager *jispman = g_new0 (KfJispManager, 1);

	jispman->jisps = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) kf_jisp_unref);
	
	return jispman;
}


/**
 * \brief Delete an instance of KfJispManager and free all its resources
 * \param jispman a KfJispManager to free
 **/
void kf_jisp_manager_free (KfJispManager *jispman)
{
	g_return_if_fail (jispman);

	g_hash_table_destroy (jispman->jisps);
	g_free (jispman);
}


/**
 * \brief Add a KfJisp to KfJispManager
 **/
void kf_jisp_manager_add_jisp (KfJispManager *jispman, KfJisp *jisp)
{
	g_return_if_fail (jispman);
	g_return_if_fail (jisp);

	g_hash_table_insert (jispman->jisps, jisp->filename, kf_jisp_ref (jisp));
}


/**
 * \brief Remove a KfJisp from a KfJispManager
 **/
void kf_jisp_manager_remove_jisp (KfJispManager *jispman, KfJisp *jisp)
{
	/* TODO */
	if (! g_hash_table_remove (jispman->jisps, jisp->filename))
		foo_debug ("Unable to delete jisp %s\n", jisp->filename);
}


/**
 * \brief Build a list of emoticons
 * \param jispman a KfJispManager to obtain list from
 * \return a new GList of KfEmo, should be freed with g_list_free when no longer used
 **/
GList *kf_jisp_manager_build_emoticons (KfJispManager *jispman)
{
	/* TODO */
	return NULL;
}


/**
 * \brief Get a list of JISPs
 * \param jispman a KfJispManager to obtain list from
 * \param criteria type of Jisp packages to include in returned list
 * \retuirn a newly-allocated GList of KfJisp, should be freed with g_list_free when no longer used
 **/
GList *kf_jisp_manager_get_list (KfJispManager *jispman, KfJispType criteria)
{
	if (criteria == 0)
		criteria = KF_JISP_TYPE_ROSTER | KF_JISP_TYPE_EMOTICONS;
	/* TODO */
	return NULL;
}


/**
 * \brief Get a KfJisp from a kfJispManager
 * \param jispman a Jisp manager
 * \param path a path to this icon package
 * \return a KfJisp or NULL if there is no such Jisp package. If you want to use it, call
 *         kf_jisp_ref and kf_jisp_unref if you finish with it, to track its reference count.
 **/
KfJisp *kf_jisp_manager_get (KfJispManager *jispman, const gchar *path)
{
	g_return_val_if_fail (jispman, NULL);
	g_return_val_if_fail (path, NULL);

	return g_hash_table_lookup (jispman->jisps, path);
}


/**
 * \brief Add a patern to KfJispManager
 *
 * A pattern is used to map specific iconsets for various contacts, based on their JID
 *
 * \param jispman A KfJispManager
 * \param jid A pattern, such as '*@icq.server'
 * \param jisp A roster JISP package
 **/
KfJispPattern *kf_jisp_manager_add_pattern (KfJispManager *jispman, const gchar *jid, KfJisp *jisp)
{
	KfJispPattern *pattern = kf_jisp_pattern_new (jid, jisp);
	g_list_append (jispman->patterns, pattern);
	return NULL;
}


/**
 * \brief Remove a pattern from a KfJispManager
 **/
void kf_jisp_pattern_remove (KfJispManager *jispman, KfJispPattern *pattern)
{
	g_return_if_fail (jispman);
	g_return_if_fail (pattern);
	
	g_list_remove (jispman->patterns, pattern);
}


/**
 * \brief Get a JISP for a particular JID
 * \param jispman a Jisp manager used as a source
 * \param jid a JabberID
 * \return a KfJisp or NULL if there is no such Jisp package. If you want to use it, call
 *         kf_jisp_ref and kf_jisp_unref if you finish with it, to track its reference count.
 **/
KfJisp *kf_jisp_manager_get_jisp_for_jid (KfJispManager *jispman, const gchar *jid)
{
	g_return_val_if_fail (jispman, NULL);
	g_return_val_if_fail (jid, NULL);
	
	GList *tmp;

	for (tmp = jispman->patterns; tmp; tmp = tmp->next) {
		KfJispPattern *pattern = tmp->data;

		if (g_pattern_match_string (pattern->gpat, jid))
			return pattern->jisp;
	}
	return NULL;
}


/**
 * \brief create a new KfJispPattern
 *
 * JISP will be referenced (kf_jisp_ref (jisp) will be called
 *
 * \param pattern a pattern (eg '*@icq.server')
 * \param jisp an iconset connected to this pattern
 * \return a new KfJispPattern
 **/
KfJispPattern *kf_jisp_pattern_new (const gchar *pattern, KfJisp *jisp)
{
	g_return_val_if_fail (pattern, NULL);
	g_return_val_if_fail (jisp, NULL);
	
	KfJispPattern *pat = g_new0 (KfJispPattern, 1);
	pat->pattern = g_strdup (pattern);
	pat->jisp = kf_jisp_ref (jisp);
	pat->gpat = g_pattern_spec_new (pattern);

	return pat;
}

/**
 * \brief Free a KfJispPattern and all its resources
 */
void kf_jisp_pattern_free (KfJispPattern *pat)
{
	g_return_if_fail (pat);

	g_free (pat->pattern);
	kf_jisp_unref (pat->jisp);
	g_pattern_spec_free (pat->gpat);
	g_free (pat);
}
