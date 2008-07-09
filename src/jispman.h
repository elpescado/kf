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

#include <glib.h>
#include "jisp.h"

typedef struct _KfJispManager {
} KfJispManager;

/**
 * \brief Type of a Jisp package
 **/
typedef enum {
	KF_JISP_TYPE_ROSTER	= 1 << 1,	/** Roster icons */
	KF_JISP_TYPE_EMOTICONS	= 1 << 2	/** Emoticons	*/
} KfJispType;

/**
 * \brief A pattern
 **/
typedef struct {
	gchar *pattern;		/** A text pattern */
	KfJisp *jisp;		/** A iconset used for contacts that match this pattern */
	GPatternSpec *gpat;	/** An internal representation of this pattern, should be considered private */
} KfJispPattern;


KfJispManager *kf_jisp_manager_new (void);
void kf_jisp_manager_free (KfJispManager *jispman);
void kf_jisp_manager_add_jisp (KfJispManager *jispman, KfJisp *jisp);
void kf_jisp_manager_remove_jisp (KfJispManager *jispman, KfJisp *jisp);
GList *kf_jisp_manager_build_emoticons (KfJispManager *jispman);
GList *kf_jisp_manager_get_list (KfJispManager *jispman, KfJispType criteria);
KfJisp *kf_jisp_manager_get (KfJispManager *jispman, const gchar *path);
KfJispPattern *kf_jisp_manager_add_pattern (KfJispManager *jispman, const gchar *jid, KfJisp *jisp);
void kf_jisp_pattern_remove (KfJispManager *jispman, KfJispPattern *pattern);
KfJisp *kf_jisp_manager_get_jisp_for_jid (KfJispManager *jispman, const gchar *jid);
KfJispPattern *kf_jisp_pattern_new (const gchar *pattern, KfJisp *jisp);
void kf_jisp_pattern_free (KfJispPattern *pat);

