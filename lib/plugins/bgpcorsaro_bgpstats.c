/*
 * bgpcorsaro
 *
 * Chiara Orsini, CAIDA, UC San Diego
 * corsaro-info@caida.org
 *
 * Copyright (C) 2012 The Regents of the University of California.
 *
 * This file is part of bgpcorsaro.
 *
 * bgpcorsaro is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bgpcorsaro is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bgpcorsaro.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"
#include "bgpcorsaro_int.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bgpstream_lib.h"
#include "bgpdump_util.h"

#include "utils.h"
#include "wandio_utils.h"

#include "bgpcorsaro_io.h"
#include "bgpcorsaro_log.h"
#include "bgpcorsaro_plugin.h"

#include "khash.h"
#include "bgpcorsaro_bgpstats.h"

/** @file
 *
 * @brief Bgpcorsaro BgpStats plugin implementation
 *
 * @author Chiara Orsini
 *
 */

/** The number of output file pointers to support non-blocking close at the end
    of an interval. If the wandio buffers are large enough that it takes more
    than 1 interval to drain the buffers, consider increasing this number */
#define OUTFILE_POINTERS 2

/** The name of this plugin */
#define PLUGIN_NAME "bgpstats"

/** The version of this plugin */
#define PLUGIN_VERSION "0.1"

/** Common plugin information across all instances */
static bgpcorsaro_plugin_t bgpcorsaro_bgpstats_plugin = {
  PLUGIN_NAME,                                          /* name */
  PLUGIN_VERSION,                                       /* version */
  BGPCORSARO_PLUGIN_ID_BGPSTATS,                        /* id */
  BGPCORSARO_PLUGIN_GENERATE_PTRS(bgpcorsaro_bgpstats), /* func ptrs */
  BGPCORSARO_PLUGIN_GENERATE_TAIL,
};

#define METRIC_PREFIX "bgp"


/* Collector related functions */

typedef struct collectordata {
  char *dump_project;
  char *dump_collector; /* graphite-safe version of the name */
  uint64_t num_records[BGPSTREAM_RECORD_TYPE_MAX];
  uint64_t num_elem[BGPSTREAM_ELEM_TYPE_MAX];
} collectordata_t;

static void graphite_safe(char *p)
{
  if(p == NULL)
    {
      return;
    }

  while(*p != '\0')
    {
      if(*p == '.')
	{
	  *p = '_';
	}
      if(*p == '*')
	{
	  *p = '-';
	}
      p++;
    }
}

static collectordata_t *collectordata_create(const char *project,
					     const char *collector)
{
  collectordata_t *collector_data;
  assert(project != NULL);
  assert(collector != NULL);

  if((collector_data = malloc_zero(sizeof(collectordata_t))) == NULL)
    {
      return NULL;
    }

  if((collector_data->dump_project = strdup(project)) == NULL)
    {
      free(collector_data);
      return NULL;
    }

  if((collector_data->dump_collector = strdup(collector)) == NULL)
    {
      free(collector_data->dump_project);
      free(collector_data);
      return NULL;
    }

  /* make the project name graphite-safe */
  graphite_safe(collector_data->dump_project);

  /* make the collector name graphite-safe */
  graphite_safe(collector_data->dump_collector);


  return collector_data;
}

static void collectordata_update(bgpcorsaro_record_t * record,
				 collectordata_t *collector_data)
{
  assert(collector_data != NULL);
  bgpstream_record_t * bs_record = BS_REC(record);
  bgpstream_elem_t * bs_elem_queue;
  bgpstream_elem_t * bs_iterator;

  collector_data->num_records[bs_record->status]++;
  if(BS_REC(record)->status == VALID_RECORD)
    {
      bs_elem_queue = bgpstream_get_elem_queue(bs_record);
      bs_iterator = bs_elem_queue;
      while(bs_iterator != NULL)
	{
	  collector_data->num_elem[bs_iterator->type]++;
	  bs_iterator = bs_iterator->next;
	}
      bgpstream_destroy_elem_queue(bs_elem_queue);
    }
  return;
}

static void collectordata_reset(collectordata_t *collector_data)
{
  memset(collector_data->num_records, 0,
	 sizeof(collector_data->num_records));

  memset(collector_data->num_elem, 0,
	 sizeof(collector_data->num_elem));
}


static void collectordata_destroy(collectordata_t *collector_data)
{
  if(collector_data == NULL)
    {
      return;
    }

  if(collector_data->dump_project != NULL)
    {
      free(collector_data->dump_project);
      collector_data->dump_project = NULL;
    }

  if(collector_data->dump_collector != NULL)
    {
      free(collector_data->dump_collector);
      collector_data->dump_collector = NULL;
    }

  free(collector_data);
}

/** Collectors table khash related functions */

KHASH_INIT(collector_table_t,        /* name */
	   char *,                   /* khkey_t */
	   collectordata_t *,        /* khval_t */
	   1,                        /* kh_is_map */
	   kh_str_hash_func,         /* __hash_func */
	   kh_str_hash_equal         /* __hash_equal */
	   )

/** Holds the state for an instance of this plugin */
struct bgpcorsaro_bgpstats_state_t {
  /** The outfile for the plugin */
  iow_t *outfile;
  /** A set of pointers to outfiles to support non-blocking close */
  iow_t *outfile_p[OUTFILE_POINTERS];
  /** The current outfile */
  int outfile_n;

  /** number of records read for each type */
  int num_records[BGPSTREAM_RECORD_TYPE_MAX];

  /** Hash of collector string to collector data */
  khash_t(collector_table_t) *collectors_table;
};

/** Extends the generic plugin state convenience macro in bgpcorsaro_plugin.h */
#define STATE(bgpcorsaro)						\
  (BGPCORSARO_PLUGIN_STATE(bgpcorsaro, bgpstats, BGPCORSARO_PLUGIN_ID_BGPSTATS))
/** Extends the generic plugin plugin convenience macro in bgpcorsaro_plugin.h */
#define PLUGIN(bgpcorsaro)						\
  (BGPCORSARO_PLUGIN_PLUGIN(bgpcorsaro, BGPCORSARO_PLUGIN_ID_BGPSTATS))

#if 0
/** Print usage information to stderr */
static void usage(bgpcorsaro_plugin_t *plugin)
{
  //TODO modify usage
  fprintf(stderr,
	  "plugin usage: %s [-HmM] [-t mode]\n"
	  "       -H         multi-line, human-readable (default)\n"
	  "       -m         one-line per entry with unix timestamps\n"
	  "       -M         one-line per entry with human readable timestamps (and some other differences that no human could ever comprehend)\n"
	  "       -t dump    timestamps for RIB dumps reflect the time of the dump (default)\n"
	  "       -t change  timestamps for RIB dumps reflect the last route modification\n",
	  plugin->argv[0]);
}

/** Parse the arguments given to the plugin */
static int parse_args(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  struct bgpcorsaro_bgpstats_state_t *state = STATE(bgpcorsaro);
  int opt;
  //TODO modify this: right now it doesn't do anything

  if(plugin->argc <= 0)
    {
      return 0;
    }

  /* NB: remember to reset optind to 1 before using getopt! */
  optind = 1;

  while((opt = getopt(plugin->argc, plugin->argv, ":t:HmM?")) >= 0)
    {
      switch(opt)
	{
	case 'H':
	  break;

	case 'm':
	  break;

	case 'M':
	  break;

	case 't':
	  break;

	case '?':
	case ':':
	default:
	  usage(plugin);
	  return -1;
	}
    }

  /* dump doesn't take any arguments */
  if(optind != plugin->argc)
    {
      usage(plugin);
      return -1;
    }

  return 0;
}
#endif

/** struct bgpdatainfo_t update (inside the interval) */
static int stats_update(struct bgpcorsaro_bgpstats_state_t *state,
			 bgpcorsaro_record_t * record)
{
  assert(state != NULL);
  assert(record != NULL);

  khiter_t k;
  collectordata_t * collector_data;
  char *name_cpy = NULL;
  int khret;
  bgpstream_record_t * bs_record = BS_REC(record);
  assert(bs_record != NULL);

  state->num_records[BS_REC(record)->status]++;

  /* check if this collector is in the hash already */
  if((k = kh_get(collector_table_t, state->collectors_table,
		 bs_record->attributes.dump_collector)) ==
     kh_end(state->collectors_table))
    {
      /* create a new data structure */
      collector_data =
	collectordata_create(bs_record->attributes.dump_project,
			     bs_record->attributes.dump_collector);

      /* copy the name of the collector */
      /* TODO: work out how to not use strings to identify a collector */
      /* TODO: what if two projects use the same collector name? */
      if((name_cpy = strdup(bs_record->attributes.dump_collector)) == NULL)
	{
	  return -1;
	}

      /* add it to the hash */
      k = kh_put(collector_table_t, state->collectors_table,
		 name_cpy, &khret);

      kh_value(state->collectors_table, k) = collector_data;
    }
  else
    {
      /* already exists, just get it */
      collector_data = kh_value(state->collectors_table, k);
    }

  collectordata_update(record, collector_data);
  return 0;
}

/** struct bgpdatainfo_t eoi (end of interval) - write stats at end of
    interval*/
static void stats_dump(struct bgpcorsaro_bgpstats_state_t *state,
		       bgpcorsaro_interval_t *int_end)
{
  assert(state != NULL);
  khiter_t k;
  collectordata_t *collector_data;

  for(k = kh_begin(state->collectors_table);
      k != kh_end(state->collectors_table); ++k)
    {
      if (kh_exist(state->collectors_table, k))
	{
	  collector_data = kh_value(state->collectors_table, k);

	  /* num_valid_records */
	  fprintf(stdout,
		  METRIC_PREFIX".%s.%s.valid_record_cnt %"PRIu64" %d\n",
		  collector_data->dump_project,
		  collector_data->dump_collector,
		  collector_data->num_records[VALID_RECORD],
		  int_end->time);

	  /* rib_entries */
	  fprintf(stdout,
		  METRIC_PREFIX".%s.%s.rib_entry_cnt %"PRIu64" %d\n",
		  collector_data->dump_project,
		  collector_data->dump_collector,
		  collector_data->num_elem[BST_RIB],
		  int_end->time);

	  /* announcements */
	  fprintf(stdout,
		  METRIC_PREFIX".%s.%s.announcement_cnt %"PRIu64" %d\n",
		  collector_data->dump_project,
		  collector_data->dump_collector,
		  collector_data->num_elem[BST_ANNOUNCEMENT],
		  int_end->time);

	  /* withdrawals */
	  fprintf(stdout,
		  METRIC_PREFIX".%s.%s.withdrawal_cnt %"PRIu64" %d\n",
		  collector_data->dump_project,
		  collector_data->dump_collector,
		  collector_data->num_elem[BST_WITHDRAWAL],
		  int_end->time);

	  /* might as well reset here to avoid walking the hash a second time */
	  collectordata_reset(collector_data);
	}
    }

  memset(state->num_records, 0, sizeof(state->num_records));
}

/* == PUBLIC PLUGIN FUNCS BELOW HERE == */

/** Implements the alloc function of the plugin API */
bgpcorsaro_plugin_t *bgpcorsaro_bgpstats_alloc(bgpcorsaro_t *bgpcorsaro)
{
  return &bgpcorsaro_bgpstats_plugin;
}

/** Implements the init_output function of the plugin API */
int bgpcorsaro_bgpstats_init_output(bgpcorsaro_t *bgpcorsaro)
{
  struct bgpcorsaro_bgpstats_state_t *state;
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  assert(plugin != NULL);

  if((state = malloc_zero(sizeof(struct bgpcorsaro_bgpstats_state_t))) == NULL)
    {
      bgpcorsaro_log(__func__, bgpcorsaro,
		     "could not malloc bgpcorsaro_bgpstats_state_t");
      goto err;
    }

  state->collectors_table = kh_init(collector_table_t);

  bgpcorsaro_plugin_register_state(bgpcorsaro->plugin_manager, plugin, state);

#if 0
  /* parse the arguments */
  if(parse_args(bgpcorsaro) != 0)
    {
      return -1;
    }
#endif

  /* defer opening the output file until we start the first interval */

  return 0;

 err:
  bgpcorsaro_bgpstats_close_output(bgpcorsaro);
  return -1;
}

/** Implements the close_output function of the plugin API */
int bgpcorsaro_bgpstats_close_output(bgpcorsaro_t *bgpcorsaro)
{
  int i;
  khiter_t k;
  struct bgpcorsaro_bgpstats_state_t *state = STATE(bgpcorsaro);

  if(state != NULL)
    {
      /* close all the outfile pointers */
      for(i = 0; i < OUTFILE_POINTERS; i++)
	{
	  if(state->outfile_p[i] != NULL)
	    {
	      wandio_wdestroy(state->outfile_p[i]);
	      state->outfile_p[i] = NULL;
	    }
	}
      state->outfile = NULL;

      /* free all the keys and values in the collectors table */
      for (k = kh_begin(state->collectors_table);
	   k != kh_end(state->collectors_table); ++k)
	{
	  if (kh_exist(state->collectors_table, k))
	    {
	      /* free the value */
	      collectordata_destroy(kh_value(state->collectors_table, k));
	      /* free the key (string) */
	      free(kh_key(state->collectors_table, k));
	    }
	}

      /* destroy collectors table */
      kh_destroy(collector_table_t, state->collectors_table);

      bgpcorsaro_plugin_free_state(bgpcorsaro->plugin_manager,
				   PLUGIN(bgpcorsaro));
    }
  return 0;
}

/** Implements the start_interval function of the plugin API */
int bgpcorsaro_bgpstats_start_interval(bgpcorsaro_t *bgpcorsaro,
				       bgpcorsaro_interval_t *int_start)
{
  struct bgpcorsaro_bgpstats_state_t *state = STATE(bgpcorsaro);
  if(state->outfile == NULL)
    {
      if((
	  state->outfile_p[state->outfile_n] =
	  bgpcorsaro_io_prepare_file(bgpcorsaro,
				     PLUGIN(bgpcorsaro)->name,
				     int_start)) == NULL)
	{
	  bgpcorsaro_log(__func__, bgpcorsaro, "could not open %s output file",
			 PLUGIN(bgpcorsaro)->name);
	  return -1;
	}
      state->outfile = state->
	outfile_p[state->outfile_n];
    }

  bgpcorsaro_io_write_interval_start(bgpcorsaro, state->outfile, int_start);

  return 0;
}

/** Implements the end_interval function of the plugin API */
int bgpcorsaro_bgpstats_end_interval(bgpcorsaro_t *bgpcorsaro,
				     bgpcorsaro_interval_t *int_end)
{
  struct bgpcorsaro_bgpstats_state_t *state = STATE(bgpcorsaro);

  bgpcorsaro_log(__func__, bgpcorsaro, "Dumping stats for interval %d",
		 int_end->number);
  stats_dump(state, int_end);

  bgpcorsaro_io_write_interval_end(bgpcorsaro, state->outfile, int_end);

  /* if we are rotating, now is when we should do it */
  if(bgpcorsaro_is_rotate_interval(bgpcorsaro))
    {
      /* leave the current file to finish draining buffers */
      assert(state->outfile != NULL);

      /* move on to the next output pointer */
      state->outfile_n = (state->outfile_n+1) %
	OUTFILE_POINTERS;

      if(state->outfile_p[state->outfile_n] != NULL)
	{
	  /* we're gonna have to wait for this to close */
	  wandio_wdestroy(state->outfile_p[state->outfile_n]);
	  state->outfile_p[state->outfile_n] =  NULL;
	}

      state->outfile = NULL;
    }
  return 0;
}

/** Implements the process_record function of the plugin API */
int bgpcorsaro_bgpstats_process_record(bgpcorsaro_t *bgpcorsaro,
				       bgpcorsaro_record_t *record)
{
  struct bgpcorsaro_bgpstats_state_t *state = STATE(bgpcorsaro);

  /* no point carrying on if a previous plugin has already decided we should
     ignore this record */
  if((record->state.flags & BGPCORSARO_RECORD_STATE_FLAG_IGNORE) != 0)
    {
      return 0;
    }

  assert(state != NULL);
  return stats_update(state, record);
}
