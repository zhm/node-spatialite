/*

 extra_tables.c -- Creating all SLD/SE and ISO Metadata extra tables

 version 4.0, 2013 February 16

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
the Initial Developer. All Rights Reserved.

Contributor(s):

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

/*
 
CREDITS:

this module has been partly funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(implementing XML support - ISO Metadata and SLD/SE Styles) 

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite.h>
#include <spatialite_private.h>
#include <spatialite/gaiaaux.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

static int
check_splite_metacatalog (sqlite3 * sqlite)
{
/* checks if "splite_metacatalog" really exists */
    int table_name = 0;
    int column_name = 0;
    int table_name2 = 0;
    int column_name2 = 0;
    int value = 0;
    int count = 0;
    char sql[1024];
    int ret;
    const char *name;
    int i;
    char **results;
    int rows;
    int columns;
/* checking the "splite_metacatalog" table */
    strcpy (sql, "PRAGMA table_info(splite_metacatalog)");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, "table_name") == 0)
		    table_name = 1;
		if (strcasecmp (name, "column_name") == 0)
		    column_name = 1;
	    }
      }
    sqlite3_free_table (results);
/* checking the "splite_metacatalog_statistics" table */
    strcpy (sql, "PRAGMA table_info(splite_metacatalog_statistics)");
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, "table_name") == 0)
		    table_name2 = 1;
		if (strcasecmp (name, "column_name") == 0)
		    column_name2 = 1;
		if (strcasecmp (name, "value") == 0)
		    value = 1;
		if (strcasecmp (name, "count") == 0)
		    count = 1;
	    }
      }
    sqlite3_free_table (results);
    if (table_name && column_name && table_name2 && column_name2 && value
	&& count)
	return 1;
    return 0;
}

static int
metacatalog_statistics (sqlite3 * sqlite, sqlite3_stmt * stmt_out,
			sqlite3_stmt * stmt_del, const char *table,
			const char *column)
{
/* auxiliary - updating "splite_metacatalog_statistics" */
    char *xtable;
    char *xcolumn;
    char *sql_statement;
    int ret;
    sqlite3_stmt *stmt_in;

    xtable = gaiaDoubleQuotedSql (table);
    xcolumn = gaiaDoubleQuotedSql (column);
    sql_statement = sqlite3_mprintf ("SELECT \"%s\", Count(*) FROM \"%s\" "
				     "GROUP BY \"%s\"", xcolumn, xtable,
				     xcolumn);
    free (xcolumn);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_in, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Update MetaCatalog Statistics(4) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

/* deleting all existing rows */
    sqlite3_reset (stmt_del);
    sqlite3_clear_bindings (stmt_del);
    sqlite3_bind_text (stmt_del, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt_del, 2, column, strlen (column), SQLITE_STATIC);
    ret = sqlite3_step (stmt_del);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("populate MetaCatalog Statistics(5) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt_in);
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		sqlite3_bind_text (stmt_out, 1, table, strlen (table),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt_out, 2, column, strlen (column),
				   SQLITE_STATIC);
		switch (sqlite3_column_type (stmt_in, 0))
		  {
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_out, 3,
					  sqlite3_column_int (stmt_in, 0));
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_out, 3,
					   sqlite3_column_double (stmt_in, 0));
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_out, 3,
					 (const char *)
					 sqlite3_column_text (stmt_in, 0),
					 sqlite3_column_bytes (stmt_in, 0),
					 SQLITE_STATIC);
		      break;
		  case SQLITE_BLOB:
		      sqlite3_bind_blob (stmt_out, 3,
					 sqlite3_column_blob (stmt_in, 0),
					 sqlite3_column_bytes (stmt_in, 0),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_out, 3);
		      break;
		  };
		sqlite3_bind_int (stmt_out, 4, sqlite3_column_int (stmt_in, 1));
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e
			  ("populate MetaCatalog Statistics(6) error: \"%s\"\n",
			   sqlite3_errmsg (sqlite));
		      sqlite3_finalize (stmt_in);
		      return 0;
		  }
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;
}

SPATIALITE_DECLARE int
gaiaUpdateMetaCatalogStatistics (sqlite3 * sqlite, const char *table,
				 const char *column)
{
/* Updates the "splite_metacalog_statistics" table */
    char *sql_statement;
    int ret;
    sqlite3_stmt *stmt_in;
    sqlite3_stmt *stmt_out;
    sqlite3_stmt *stmt_del;

    if (!check_splite_metacatalog (sqlite))
      {
	  spatialite_e
	      ("invalid or not existing \"splite_metacatalog_statistics\" table\n");
	  return 0;
      }

/* updating the MetaCatalog statistics */
    sql_statement = sqlite3_mprintf ("SELECT table_name, column_name "
				     "FROM splite_metacatalog WHERE "
				     "Lower(table_name) = Lower(%Q) "
				     "AND Lower(column_name) = Lower(%Q)",
				     table, column);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_in, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Update MetaCatalog Statistics(1) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    sql_statement = "INSERT INTO splite_metacatalog_statistics "
	"(table_name, column_name, value, count) " "VALUES (?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_out, NULL);
    if (ret != SQLITE_OK)
      {
	  sqlite3_finalize (stmt_in);
	  spatialite_e ("Update MetaCatalog Statistics(2) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    sql_statement = "DELETE FROM splite_metacatalog_statistics "
	"WHERE Lower(table_name) = Lower(?) AND Lower(column_name) = Lower(?)";
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt_del, NULL);
    if (ret != SQLITE_OK)
      {
	  sqlite3_finalize (stmt_in);
	  sqlite3_finalize (stmt_out);
	  spatialite_e ("Update MetaCatalog Statistics(3) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *table =
		    (const char *) sqlite3_column_text (stmt_in, 0);
		const char *column =
		    (const char *) sqlite3_column_text (stmt_in, 1);
		if (!metacatalog_statistics
		    (sqlite, stmt_out, stmt_del, table, column))
		  {
		      sqlite3_finalize (stmt_in);
		      sqlite3_finalize (stmt_out);
		      sqlite3_finalize (stmt_del);
		      return 0;
		  }
	    }
      }
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    sqlite3_finalize (stmt_del);
    return 1;
}

static int
check_master_table (sqlite3 * sqlite, const char *master_table,
		    const char *table, const char *column)
{
/* checks if the Master Table could be accessed */
    int table_name = 0;
    int column_name = 0;
    char *sql;
    int ret;
    char *xmaster;
    const char *name;
    int i;
    char **results;
    int rows;
    int columns;
/* checking the Master table */
    xmaster = gaiaDoubleQuotedSql (master_table);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xmaster);
    free (xmaster);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, table) == 0)
		    table_name = 1;
		if (strcasecmp (name, column) == 0)
		    column_name = 1;
	    }
      }
    sqlite3_free_table (results);
    if (table_name && column_name)
	return 1;
    return 0;
}

SPATIALITE_DECLARE int
gaiaUpdateMetaCatalogStatisticsFromMaster (sqlite3 * sqlite,
					   const char *master_table,
					   const char *table_name,
					   const char *column_name)
{
/* Updates the "splite_metacalog_statistics" table (using a Master Table) */
    int ret;
    char *sql_statement;
    sqlite3_stmt *stmt;
    char *xmaster;
    char *xtable;
    char *xcolumn;
    if (!check_master_table (sqlite, master_table, table_name, column_name))
      {
	  spatialite_e
	      ("UpdateMetaCatalogStatisticsFromMaster: mismatching or not existing Master Table\n");
	  return 0;
      }

/* scanning the Master Table */
    xmaster = gaiaDoubleQuotedSql (master_table);
    xtable = gaiaDoubleQuotedSql (table_name);
    xcolumn = gaiaDoubleQuotedSql (column_name);
    sql_statement =
	sqlite3_mprintf ("SELECT \"%s\", \"%s\" FROM \"%s\"", xtable, xcolumn,
			 xmaster);
    free (xmaster);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("UpdateMetaCatalogStatisticsFromMaster(1) error: \"%s\"\n",
	       sqlite3_errmsg (sqlite));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *table =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *column =
		    (const char *) sqlite3_column_text (stmt, 1);
		if (!gaiaUpdateMetaCatalogStatistics (sqlite, table, column))
		  {
		      sqlite3_finalize (stmt);
		      return 0;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return 1;
}

static int
check_unique_index (sqlite3 * sqlite, const char *index, const char *column)
{
/* checks if a column has any Unique constraint - pass two */
    char *xindex;
    char *sql_statement;
    int ret;
    sqlite3_stmt *stmt_in;
    int is_unique = 0;
    int index_parts = 0;

    xindex = gaiaDoubleQuotedSql (index);
    sql_statement = sqlite3_mprintf ("PRAGMA index_info(\"%s\")", xindex);
    free (xindex);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_in, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("populate MetaCatalog(8) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *colname =
		    (const char *) sqlite3_column_text (stmt_in, 2);
		if (strcasecmp (colname, column) == 0)
		    is_unique = 1;
		index_parts++;
	    }
      }
    sqlite3_finalize (stmt_in);

    if (index_parts > 1)
      {
	  /* ignoring any multi-column index */
	  is_unique = 0;
      }
    return is_unique;
}

static int
check_unique (sqlite3 * sqlite, const char *table, const char *column)
{
/* checks if a column has any Unique constraint */
    char *xtable;
    char *sql_statement;
    int ret;
    sqlite3_stmt *stmt_in;
    int is_unique = 0;

    xtable = gaiaDoubleQuotedSql (table);
    sql_statement = sqlite3_mprintf ("PRAGMA index_list(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_in, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("populate MetaCatalog(7) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *idxname =
		    (const char *) sqlite3_column_text (stmt_in, 1);
		if (sqlite3_column_int (stmt_in, 2) == 1)
		  {
		      /* Unique Index */
		      if (check_unique_index (sqlite, idxname, column))
			  is_unique = 1;
		  }
	    }
      }
    sqlite3_finalize (stmt_in);

    return is_unique;
}

static int
check_foreign_key (sqlite3 * sqlite, const char *table, const char *column)
{
/* checks if a column is part of any Foreign Key */
    char *xtable;
    char *sql_statement;
    int ret;
    sqlite3_stmt *stmt_in;
    int is_foreign_key = 0;

    xtable = gaiaDoubleQuotedSql (table);
    sql_statement = sqlite3_mprintf ("PRAGMA foreign_key_list(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_in, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("populate MetaCatalog(6) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *colname =
		    (const char *) sqlite3_column_text (stmt_in, 3);
		if (strcasecmp (colname, column) == 0)
		    is_foreign_key = 1;
	    }
      }
    sqlite3_finalize (stmt_in);

    return is_foreign_key;
}

static int
table_info (sqlite3 * sqlite, sqlite3_stmt * stmt_out, const char *table)
{
/* auxiliary - populating "splite_metacatalog" */
    char *xtable;
    char *sql_statement;
    int ret;
    sqlite3_stmt *stmt_in;

    xtable = gaiaDoubleQuotedSql (table);
    sql_statement = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_in, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("populate MetaCatalog(3) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int is_foreign_key;
		int is_unique;
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		sqlite3_bind_text (stmt_out, 1, table, strlen (table),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt_out, 2,
				   (const char *) sqlite3_column_text (stmt_in,
								       1),
				   sqlite3_column_bytes (stmt_in, 1),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt_out, 3,
				   (const char *) sqlite3_column_text (stmt_in,
								       2),
				   sqlite3_column_bytes (stmt_in, 2),
				   SQLITE_STATIC);
		sqlite3_bind_int (stmt_out, 4, sqlite3_column_int (stmt_in, 3));
		sqlite3_bind_int (stmt_out, 5, sqlite3_column_int (stmt_in, 5));
		is_foreign_key =
		    check_foreign_key (sqlite, table,
				       (const char *)
				       sqlite3_column_text (stmt_in, 1));
		sqlite3_bind_int (stmt_out, 6, is_foreign_key);
		is_unique =
		    check_unique (sqlite, table,
				  (const char *) sqlite3_column_text (stmt_in,
								      1));
		sqlite3_bind_int (stmt_out, 7, is_unique);
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("populate MetaCatalog(4) error: \"%s\"\n",
				    sqlite3_errmsg (sqlite));
		      sqlite3_finalize (stmt_in);
		      return 0;
		  }
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;
}

SPATIALITE_DECLARE int
gaiaCreateMetaCatalogTables (sqlite3 * sqlite)
{
/* Creates both "splite_metacatalog" and "splite_metacalog_statistics" tables */
    char *sql_statement;
    char *err_msg = NULL;
    int ret;
    sqlite3_stmt *stmt_in;
    sqlite3_stmt *stmt_out;

/* creating "splite_metacatalog" */
    sql_statement = "CREATE TABLE splite_metacatalog (\n"
	"table_name TEXT NOT NULL,\n"
	"column_name TEXT NOT NULL,\n"
	"type TEXT NOT NULL,\n"
	"not_null INTEGER NOT NULL,\n"
	"primary_key INTEGER NOT NULL,\n"
	"foreign_key INTEGER NOT NULL,\n"
	"unique_value INTEGER NOT NULL,\n"
	"CONSTRAINT pk_splite_metacatalog PRIMARY KEY (table_name, column_name))";
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TABLE splite_metacatalog - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating "splite_metacatalog_statistics" */
    sql_statement = "CREATE TABLE splite_metacatalog_statistics (\n"
	"table_name TEXT NOT NULL,\n"
	"column_name TEXT NOT NULL,\n"
	"value TEXT,\n"
	"count INTEGER NOT NULL,\n"
	"CONSTRAINT pk_splite_metacatalog_statistics PRIMARY KEY (table_name, column_name, value),\n"
	"CONSTRAINT fk_splite_metacatalog_statistics FOREIGN KEY (table_name, column_name) "
	"REFERENCES splite_metacatalog (table_name, column_name))";
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TABLE splite_metacatalog_statistics - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* populating the MetaCatalog table */
    sql_statement = "SELECT name FROM sqlite_master WHERE type = 'table' "
	"AND sql NOT LIKE 'CREATE VIRTUAL TABLE%'";
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_in, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("populate MetaCatalog(1) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    sql_statement = "INSERT INTO splite_metacatalog "
	"(table_name, column_name, type, not_null, primary_key, foreign_key, unique_value) "
	"VALUES (?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			      &stmt_out, NULL);
    if (ret != SQLITE_OK)
      {
	  sqlite3_finalize (stmt_in);
	  spatialite_e ("populate MetaCatalog(2) error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *table =
		    (const char *) sqlite3_column_text (stmt_in, 0);
		if (!table_info (sqlite, stmt_out, table))
		  {
		      sqlite3_finalize (stmt_in);
		      sqlite3_finalize (stmt_out);
		      return 0;
		  }
	    }
      }
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;
}

static int
check_raster_coverages (sqlite3 * sqlite)
{
/* checking if the "raster_coverages" table already exists */
    int exists = 0;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    sql_statement = "SELECT name FROM sqlite_master WHERE type = 'table' "
	"AND Upper(name) = Upper('raster_coverages')";
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   &errMsg);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	exists = 1;
    sqlite3_free_table (results);
    return exists;
}

static int
check_raster_coverages_ref_sys (sqlite3 * sqlite)
{
/* checking if the "raster_coverages_ref_sys" view already exists */
    int exists = 0;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    sql_statement = "SELECT name FROM sqlite_master WHERE type = 'view' "
	"AND Upper(name) = Upper('raster_coverages_ref_sys')";
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   &errMsg);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	exists = 1;
    sqlite3_free_table (results);
    return exists;
}

static int
create_raster_coverages (sqlite3 * sqlite)
{
/* creating the "raster_coverages" table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE raster_coverages (\n"
	"coverage_name TEXT NOT NULL PRIMARY KEY,\n"
	"title TEXT NOT NULL DEFAULT '*** missing Title ***',\n"
	"abstract TEXT NOT NULL DEFAULT '*** missing Abstract ***',\n"
	"sample_type TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"pixel_type TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"num_bands INTEGER NOT NULL DEFAULT 1,\n"
	"compression TEXT NOT NULL DEFAULT 'NONE',\n"
	"quality INTEGER NOT NULL DEFAULT 100,\n"
	"tile_width INTEGER NOT NULL DEFAULT 512,\n"
	"tile_height INTEGER NOT NULL DEFAULT 512,\n"
	"horz_resolution DOUBLE NOT NULL,\n"
	"vert_resolution DOUBLE NOT NULL,\n"
	"srid INTEGER NOT NULL,\n"
	"nodata_pixel BLOB NOT NULL,\n"
	"palette BLOB,\n"
	"statistics BLOB,\n"
	"extent_minx DOUBLE,\n"
	"extent_miny DOUBLE,\n"
	"extent_maxx DOUBLE,\n"
	"extent_maxy DOUBLE,\n"
	"CONSTRAINT fk_rc_srs FOREIGN KEY (srid) "
	"REFERENCES spatial_ref_sys (srid))";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'raster_coverages' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the raster_coverages triggers */
    sql = "CREATE TRIGGER raster_coverages_name_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages_layers violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_name_update\n"
	"BEFORE UPDATE OF 'coverage_name' ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on raster_coverages violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on raster_coverages violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on raster_coverages violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_sample_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"sample_type must be one of ''1-BIT'' | ''2-BIT'' | ''4-BIT'' | "
	"''INT8'' | ''UINT8'' | ''INT16'' | ''UINT16'' | ''INT32'' | "
	"''UINT32'' | ''FLOAT'' | ''DOUBLE''')\n"
	"WHERE NEW.sample_type NOT IN ('1-BIT', '2-BIT', '4-BIT', "
	"'INT8', 'UINT8', 'INT16', 'UINT16', 'INT32', "
	"'UINT32', 'FLOAT', 'DOUBLE');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_sample_update\n"
	"BEFORE UPDATE OF 'sample_type' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"sample_type must be one of ''1-BIT'' | ''2-BIT'' | ''4-BIT'' | "
	"''INT8'' | ''UINT8'' | ''INT16'' | ''UINT16'' | ''INT32'' | "
	"''UINT32'' | ''FLOAT'' | ''DOUBLE''')\n"
	"WHERE NEW.sample_type NOT IN ('1-BIT', '2-BIT', '4-BIT', "
	"'INT8', 'UINT8', 'INT16', 'UINT16', 'INT32', "
	"'UINT32', 'FLOAT', 'DOUBLE');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pixel_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"pixel_type must be one of ''MONOCHROME'' | ''PALETTE'' | "
	"''GRAYSCALE'' | ''RGB'' | ''MULTIBAND'' | ''DATAGRID''')\n"
	"WHERE NEW.pixel_type NOT IN ('MONOCHROME', 'PALETTE', "
	"'GRAYSCALE', 'RGB', 'MULTIBAND', 'DATAGRID');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pixel_update\n"
	"BEFORE UPDATE OF 'pixel_type' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"pixel_type must be one of ''MONOCHROME'' | ''PALETTE'' | "
	"''GRAYSCALE'' | ''RGB'' | ''MULTIBAND'' | ''DATAGRID''')\n"
	"WHERE NEW.pixel_type NOT IN ('MONOCHROME', 'PALETTE', "
	"'GRAYSCALE', 'RGB', 'MULTIBAND', 'DATAGRID');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_bands_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"num_bands must be >= 1')\nWHERE NEW.num_bands < 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_bands_update\n"
	"BEFORE UPDATE OF 'num_bands' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"num_bands must be >= 1')\nWHERE NEW.num_bands < 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_compression_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"compression must be one of ''NONE'' | ''DEFLATE'' | ''LZMA'' | "
	"''PNG'' | ''JPEG'' | ''LOSSY_WEBP'' | ''LOSSLESS_WEBP'' | "
	"''CCITTFAX4''')\n"
	"WHERE NEW.compression NOT IN ('NONE', 'DEFLATE', 'LZMA', "
	"'PNG', 'JPEG', 'LOSSY_WEBP', 'LOSSLESS_WEBP', " "'CCITTFAX4');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_compression_update\n"
	"BEFORE UPDATE OF 'compression' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"compression must be one of ''NONE'' | ''DEFLATE'' | ''LZMA'' | "
	"''PNG'' | ''JPEG'' | ''LOSSY_WEBP'' | ''LOSSLESS_WEBP'' | "
	"''CCITTFAX4''')\n"
	"WHERE NEW.compression NOT IN ('NONE', 'DEFLATE', 'LZMA', "
	"'PNG', 'JPEG', 'LOSSY_WEBP', 'LOSSLESS_WEBP', " "'CCITTFAX4');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_quality_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"quality must be between 0 and 100')\n"
	"WHERE NEW.quality NOT BETWEEN 0 AND 100;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_quality_update\n"
	"BEFORE UPDATE OF 'quality' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"quality must be between 0 and 100')\n"
	"WHERE NEW.quality NOT BETWEEN 0 AND 100;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_tilew_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"tile_width must be an exact multiple of 8 between 256 and 1024')\n"
	"WHERE CastToInteger(NEW.tile_width) IS NULL OR "
	"NEW.tile_width NOT BETWEEN 256 AND 1024 OR (NEW.tile_width % 8) <> 0;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_tilew_update\n"
	"BEFORE UPDATE OF 'tile_width' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"tile_width must be an exact multiple of 8 between 256 and 1024')\n"
	"WHERE CastToInteger(NEW.tile_width) IS NULL OR "
	"NEW.tile_width NOT BETWEEN 256 AND 1024 OR (NEW.tile_width % 8) <> 0;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_tileh_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"tile_height must be an exact multiple of 8 between 256 and 1024')\n"
	"WHERE CastToInteger(NEW.tile_height) IS NULL OR "
	"NEW.tile_height NOT BETWEEN 256 AND 1024 OR (NEW.tile_height % 8) <> 0;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_tileh_update\n"
	"BEFORE UPDATE OF 'tile_height' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"tile_height must be an exact multiple of 8 between 256 and 1024')\n"
	"WHERE CastToInteger(NEW.tile_height) IS NULL OR "
	"NEW.tile_height NOT BETWEEN 256 AND 1024 OR (NEW.tile_height % 8) <> 0;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_horzres_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"horz_resolution must be positive')\n"
	"WHERE NEW.horz_resolution IS NOT NULL AND "
	"(NEW.horz_resolution <= 0.0 OR CastToDouble(NEW.horz_resolution) IS NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_horzres_update\n"
	"BEFORE UPDATE OF 'horz_resolution' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"horz_resolution must be positive')\n"
	"WHERE NEW.horz_resolution IS NOT NULL AND "
	"(NEW.horz_resolution <= 0.0 OR CastToDouble(NEW.horz_resolution) IS NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_vertres_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"vert_resolution must be positive')\n"
	"WHERE NEW.vert_resolution IS NOT NULL AND "
	"(NEW.vert_resolution <= 0.0 OR CastToDouble(NEW.vert_resolution) IS NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_vertres_update\n"
	"BEFORE UPDATE OF 'vert_resolution' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"vert_resolution must be positive')\n"
	"WHERE NEW.vert_resolution IS NOT NULL AND "
	"(NEW.vert_resolution <= 0.0 OR CastToDouble(NEW.vert_resolution) IS NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_nodata_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"invalid nodata_pixel')\nWHERE NEW.nodata_pixel IS NOT NULL AND "
	"IsValidPixel(NEW.nodata_pixel, NEW.sample_type, NEW.num_bands) <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_nodata_update\n"
	"BEFORE UPDATE OF 'nodata_pixel' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"invalid nodata_pixel')\nWHERE NEW.nodata_pixel IS NOT NULL AND "
	"IsValidPixel(NEW.nodata_pixel, NEW.sample_type, NEW.num_bands) <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_palette_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"invalid palette')\nWHERE NEW.palette IS NOT NULL AND "
	"(NEW.pixel_type <> 'PALETTE' OR NEW.num_bands <> 1 OR "
	"IsValidRasterPalette(NEW.palette, NEW.sample_type) <> 1);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_palette_update\n"
	"BEFORE UPDATE OF 'palette' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"invalid palette')\nWHERE NEW.palette IS NOT NULL AND "
	"(NEW.pixel_type <> 'PALETTE' OR NEW.num_bands <> 1 OR "
	"IsValidRasterPalette(NEW.palette, NEW.sample_type) <> 1);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_statistics_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"invalid statistics')\nWHERE NEW.statistics IS NOT NULL AND "
	"IsValidRasterStatistics(NEW.statistics, NEW.sample_type, NEW.num_bands) <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_statistics_update\n"
	"BEFORE UPDATE OF 'statistics' ON 'raster_coverages'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"invalid statistics')\nWHERE NEW.statistics IS NOT NULL AND "
	"IsValidRasterStatistics(NEW.statistics, NEW.sample_type, NEW.num_bands) <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_monosample_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent MONOCHROME sample_type')\nWHERE NEW.pixel_type = 'MONOCHROME' "
	"AND NEW.sample_type <> '1-BIT';\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_monosample_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent MONOCHROME sample_type')\nWHERE NEW.pixel_type = 'MONOCHROME' "
	"AND NEW.sample_type <>'1-BIT';\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_monocompr_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent MONOCHROME compression')\nWHERE NEW.pixel_type = 'MONOCHROME' "
	"AND NEW.compression NOT IN ('NONE', 'PNG', 'CCITTFAX4');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_monocompr_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent MONOCHROME compression')\nWHERE NEW.pixel_type = 'MONOCHROME' "
	"AND NEW.compression NOT IN ('NONE', 'PNG', 'CCITTFAX4');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_monobands_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent MONOCHROME num_bands')\nWHERE NEW.pixel_type = 'MONOCHROME' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_monobands_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent MONOCHROME num_bands')\nWHERE NEW.pixel_type = 'MONOCHROME' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pltsample_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent PALETTE sample_type')\nWHERE NEW.pixel_type = 'PALETTE' "
	"AND NEW.sample_type NOT IN ('1-BIT', '2-BIT', '4-BIT', 'UINT8');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pltsample_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent PALETTE sample_type')\nWHERE NEW.pixel_type = 'PALETTE' "
	"AND NEW.sample_type NOT IN ('1-BIT', '2-BIT', '4-BIT', 'UINT8');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pltcompr_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent PALETTE compression')\nWHERE NEW.pixel_type = 'PALETTE' "
	"AND NEW.compression NOT IN ('NONE', 'PNG');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pltcompr_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent PALETTE compression')\nWHERE NEW.pixel_type = 'PALETTE' "
	"AND NEW.compression NOT IN ('NONE', 'PNG');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pltbands_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent PALETTE num_bands')\nWHERE NEW.pixel_type = 'PALETTE' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_pltbands_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent PALETTE num_bands')\nWHERE NEW.pixel_type = 'PALETTE' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_graysample_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent GRAYSCALE sample_type')\nWHERE NEW.pixel_type = 'GRAYSCALE' "
	"AND NEW.sample_type NOT IN ('2-BIT', '4-BIT', 'UINT8');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_graysample_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent GRAYSCALE sample_type')\nWHERE NEW.pixel_type = 'GRAYSCALE' "
	"AND NEW.sample_type NOT IN ('2-BIT', '4-BIT', 'UINT8');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_graybands_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent GRAYSCALE num_bands')\nWHERE NEW.pixel_type = 'GRAYSCALE' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_graybands_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent GRAYSCALE num_bands')\nWHERE NEW.pixel_type = 'GRAYSCALE' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_graycompr_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent GRAYSCALE compression')\nWHERE NEW.pixel_type = 'GRAYSCALE' "
	"AND NEW.compression NOT IN ('NONE', 'PNG', 'JPEG', 'LOSSY_WEBP', "
	"'LOSSLESS_WEBP');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_graycompr_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent GRAYSCALE compression')\nWHERE NEW.pixel_type = 'GRAYSCALE' "
	"AND NEW.compression NOT IN ('NONE', 'PNG', 'JPEG', 'LOSSY_WEBP', "
	"'LOSSLESS_WEBP');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_rgbsample_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent RGB sample_type')\nWHERE NEW.pixel_type = 'RGB' "
	"AND NEW.sample_type NOT IN ('UINT8', 'UINT16');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_rgbsample_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent RGB sample_type')\nWHERE NEW.pixel_type = 'RGB' "
	"AND NEW.sample_type NOT IN ('UINT8', 'UINT16');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_rgbcompr_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent RGB compression')\nWHERE NEW.pixel_type = 'RGB' "
	"AND ((NEW.sample_type = 'UINT8' AND NEW.compression NOT IN ("
	"'NONE', 'PNG', 'JPEG', 'LOSSY_WEBP', 'LOSSLESS_WEBP') OR "
	"(NEW.sample_type = 'UINT16' AND NEW.compression NOT IN ("
	"'NONE', 'DEFLATE', 'LZMA'))));\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_rgbcompr_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent RGB compression')\nWHERE NEW.pixel_type = 'RGB' "
	"AND ((NEW.sample_type = 'UINT8' AND NEW.compression NOT IN ("
	"'NONE', 'PNG', 'JPEG', 'LOSSY_WEBP', 'LOSSLESS_WEBP') OR "
	"(NEW.sample_type = 'UINT16' AND NEW.compression NOT IN ("
	"'NONE', 'DEFLATE', 'LZMA'))));\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_rgbbands_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent RGB num_bands')\nWHERE NEW.pixel_type = 'RGB' "
	"AND NEW.num_bands <> 3;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_rgbbands_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent RGB num_bands')\nWHERE NEW.pixel_type = 'RGB' "
	"AND NEW.num_bands <> 3;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_multisample_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent MULTIBAND sample_type')\nWHERE NEW.pixel_type = 'MULTIBAND' "
	"AND NEW.sample_type NOT IN ('UINT8', 'UINT16');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_multisample_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent MULTIBAND sample_type')\nWHERE NEW.pixel_type = 'MULTIBAND' "
	"AND NEW.sample_type NOT IN ('UINT8', 'UINT16');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_multicompr_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent MULTIBAND compression')\nWHERE NEW.pixel_type = 'MULTIBAND' "
	"AND NEW.compression NOT IN ('NONE', 'DEFLATE', 'LZMA');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_multibands_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent MULTIBAND num_bands')\nWHERE NEW.pixel_type = 'MULTIBAND' "
	"AND NEW.num_bands < 2;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_multibands_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent MULTIBAND num_bands')\nWHERE NEW.pixel_type = 'MULTIBAND' "
	"AND NEW.num_bands < 2;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_multicompr_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent MULTIBAND compression')\nWHERE NEW.pixel_type = 'MULTIBAND' "
	"AND NEW.compression NOT IN ('NONE', 'DEFLATE', 'LZMA');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_gridsample_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent DATAGRID sample_type')\nWHERE NEW.pixel_type = 'DATAGRID' "
	"AND NEW.sample_type NOT IN ('INT8', 'UINT8', 'INT16', 'UINT16', "
	"'INT32', 'UINT32', 'FLOAT', 'DOUBLE');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_gridsample_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent DATAGRID sample_type')\nWHERE NEW.pixel_type = 'DATAGRID' "
	"AND NEW.sample_type NOT IN ('INT8', 'UINT8', 'INT16', 'UINT16', "
	"'INT32', 'UINT32', 'FLOAT', 'DOUBLE');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_gridcompr_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent DATAGRID compression')\nWHERE NEW.pixel_type = 'DATAGRID' "
	"AND NEW.compression NOT IN ('NONE', 'DEFLATE', 'LZMA');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_gridcompr_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent DATAGRID compression')\nWHERE NEW.pixel_type = 'DATAGRID' "
	"AND NEW.compression NOT IN ('NONE', 'DEFLATE', 'LZMA');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_gridbands_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent DATAGRID num_bands')\nWHERE NEW.pixel_type = 'DATAGRID' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_gridbands_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"inconsistent DATAGRID num_bands')\nWHERE NEW.pixel_type = 'DATAGRID' "
	"AND NEW.num_bands <> 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_georef_insert\n"
	"BEFORE INSERT ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on raster_coverages violates constraint: "
	"inconsistent georeferencing infos')\n"
	"WHERE NOT ((NEW.horz_resolution IS NULL AND NEW.vert_resolution IS NULL "
	"AND NEW.srid IS NULL) OR (NEW.horz_resolution IS NOT NULL "
	"AND NEW.vert_resolution IS NOT NULL AND NEW.srid IS NOT NULL));\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_georef_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on raster_coverages violates constraint: "
	"inconsistent georeferencing infos')\n"
	"WHERE NOT ((NEW.horz_resolution IS NULL AND NEW.vert_resolution IS NULL "
	"AND NEW.srid IS NULL) OR (NEW.horz_resolution IS NOT NULL "
	"AND NEW.vert_resolution IS NOT NULL AND NEW.srid IS NOT NULL));\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_update\n"
	"BEFORE UPDATE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on raster_coverages violates constraint: "
	"attempting to change the definition of an already populated Coverage')\n"
	"WHERE IsPopulatedCoverage(OLD.coverage_name) = 1 AND "
	"((OLD.sample_type <> NEW.sample_type) AND (OLD.pixel_type <> NEW.sample_type) "
	"OR (OLD.num_bands <> NEW.num_bands) OR (OLD.compression <> NEW.compression) "
	"OR (OLD.quality <> NEW.quality) OR (OLD.tile_width <> NEW.tile_width) "
	"OR (OLD.tile_height <> NEW.tile_height) OR (OLD.horz_resolution <> NEW.horz_resolution) "
	"OR (OLD.vert_resolution <> NEW.vert_resolution) OR "
	"(OLD.srid <> NEW.srid) OR (OLD.nodata_pixel <> NEW.nodata_pixel));\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER raster_coverages_delete\n"
	"BEFORE DELETE ON 'raster_coverages'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'delete on raster_coverages violates constraint: "
	"attempting to delete the definition of an already populated Coverage')\n"
	"WHERE IsPopulatedCoverage(OLD.coverage_name) = 1;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the raster_coverages_ref_sys view */
    sql = "CREATE VIEW raster_coverages_ref_sys AS\n"
	"SELECT c.coverage_name AS coverage_name, c.title AS title, "
	"c.abstract AS abstract, c.sample_type AS sample_type, "
	"c.pixel_type AS pixel_type, c.num_bands AS num_bands, "
	"c.compression AS compression, c.quality AS quality, "
	"c.tile_width AS tile_width, c.tile_height AS tile_height, "
	"c.horz_resolution AS horz_resolution, c.vert_resolution AS vert_resolution, "
	"c.nodata_pixel AS nodata_pixel, c.palette AS palette, "
	"c.statistics AS statistics, c.extent_minx AS extent_minx, "
	"c.extent_miny AS extent_miny, c.extent_maxx AS extent_maxx, "
	"c.extent_maxy AS extent_maxy, c.srid AS srid, "
	"s.auth_name AS auth_name, s.auth_srid AS auth_srid, "
	"s.ref_sys_name AS ref_sys_name, s.proj4text AS proj4text\n"
	"FROM raster_coverages AS c\n"
	"LEFT JOIN spatial_ref_sys AS s ON (c.srid = s.srid)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW 'raster_coverages_ref_sys' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE int
createRasterCoveragesTable (void *p_sqlite)
{
/* Creating the main RasterCoverages table */
    int ok_table;
    sqlite3 *sqlite = p_sqlite;

/* checking if already defined */
    ok_table = check_raster_coverages (sqlite);
    if (ok_table)
      {
	  spatialite_e
	      ("CreateRasterCoveragesTable() error: table 'raster_coverages' already exists\n");
	  goto error;
      }
    ok_table = check_raster_coverages_ref_sys (sqlite);
    if (ok_table)
      {
	  spatialite_e
	      ("CreateRasterCoveragesTable() error: view 'raster_coverages_ref_sys' already exists\n");
	  goto error;
      }

/* creating the main RasterCoverages table */
    if (!create_raster_coverages (sqlite))
	goto error;
    return 1;

  error:
    return 0;
}

static int
check_if_coverage_exists (sqlite3 * sqlite, const char *coverage)
{
/* checking if a Coverage table already exists */
    int exists = 0;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    sql_statement =
	sqlite3_mprintf ("SELECT name FROM sqlite_master WHERE type = 'table' "
			 "AND Upper(name) = Upper(%Q)", coverage);
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	exists = 1;
    sqlite3_free_table (results);
    return exists;
}

SPATIALITE_PRIVATE int
checkPopulatedCoverage (void *p_sqlite, const char *coverage_name)
{
/* checking if a Coverage table is already populated */
    int is_populated = 0;
    char *xname;
    char *xxname;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    sqlite3 *sqlite = p_sqlite;
    xname = sqlite3_mprintf ("%s_tile_data", coverage_name);
    if (!check_if_coverage_exists (sqlite, xname))
      {
	  sqlite3_free (xname);
	  return 0;
      }
    xxname = gaiaDoubleQuotedSql (xname);
    sqlite3_free (xname);
    sql_statement =
	sqlite3_mprintf ("SELECT ROWID FROM \"%s\" LIMIT 10", xxname);
    free (xxname);
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	is_populated = 1;
    sqlite3_free_table (results);
    return is_populated;
}

#ifdef ENABLE_LIBXML2		/* including LIBXML2 */

static int
check_styling_table (sqlite3 * sqlite, const char *table, int is_view)
{
/* checking if some SLD/SE Styling-related table/view already exists */
    int exists = 0;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    sql_statement =
	sqlite3_mprintf ("SELECT name FROM sqlite_master WHERE type = '%s'"
			 "AND Upper(name) = Upper(%Q)",
			 (!is_view) ? "table" : "view", table);
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	exists = 1;
    sqlite3_free_table (results);
    return exists;
}

static int
create_external_graphics (sqlite3 * sqlite)
{
/* creating the SE_external_graphics table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_external_graphics (\n"
	"xlink_href TEXT NOT NULL PRIMARY KEY,\n"
	"title TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"abstract TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"resource BLOB NOT NULL,\n"
	"file_name TEXT NOT NULL DEFAULT '*** undefined ***')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_external_graphics' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_external_graphics triggers */
    sql = "CREATE TRIGGER sextgr_mime_type_insert\n"
	"BEFORE INSERT ON 'SE_external_graphics'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_external_graphics violates constraint: "
	"GetMimeType(resource) must be one of ''image/gif'' | ''image/png'' | "
	"''image/jpeg'' | ''image/svg+xml''')\n"
	"WHERE GetMimeType(NEW.resource) NOT IN ('image/gif', 'image/png', "
	"'image/jpeg', 'image/svg+xml');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sextgr_mime_type_update\n"
	"BEFORE UPDATE OF 'mime_type' ON 'SE_external_graphics'"
	"\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on SE_external_graphics violates constraint: "
	"GetMimeType(resource) must be one of ''image/gif'' | ''image/png'' | "
	"''image/jpeg'' | ''image/svg+xml''')\n"
	"WHERE GetMimeType(NEW.resource) NOT IN ('image/gif', 'image/png', "
	"'image/jpeg', 'image/svg+xml');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_vector_styled_layers (sqlite3 * sqlite, int relaxed)
{
/* creating the SE_vector_styled_layers table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_vector_styled_layers (\n"
	"f_table_name TEXT NOT NULL,\n"
	"f_geometry_column TEXT NOT NULL,\n"
	"style_id INTEGER NOT NULL,\n"
	"style_name TEXT NOT NULL DEFAULT 'missing_name',\n"
	"style BLOB NOT NULL,\n"
	"CONSTRAINT pk_sevstl PRIMARY KEY "
	"(f_table_name, f_geometry_column, style_id),\n"
	"CONSTRAINT fk_sevstl FOREIGN KEY (f_table_name, f_geometry_column) "
	"REFERENCES geometry_columns (f_table_name, f_geometry_column) "
	"ON DELETE CASCADE)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_vector_styled_layers' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the layer-style UNIQUE index */
    sql = "CREATE UNIQUE INDEX idx_vector_style ON SE_vector_styled_layers "
	"(f_table_name, f_geometry_column, style_name)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX 'idx_vector_style' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_vector_styled_layers triggers */
    sql = "CREATE TRIGGER sevstl_f_table_name_insert\n"
	"BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sevstl_f_table_name_update\n"
	"BEFORE UPDATE OF 'f_table_name' ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sevstl_f_geometry_column_insert\n"
	"BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER sevstl_f_geometry_column_update\n"
	"BEFORE UPDATE OF 'f_geometry_column' ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Vector Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Vector Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER sevstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_vector_styled_layers violates constraint: "
	      "not a valid SLD/SE Vector Style')\n"
	      "WHERE XB_IsSldSeVectorStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* automatically setting the style_name after inserting */
    sql = "CREATE TRIGGER sevstl_style_name_ins\n"
	"AFTER INSERT ON 'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"UPDATE SE_vector_styled_layers "
	"SET style_name = XB_GetName(NEW.style) "
	"WHERE f_table_name = NEW.f_table_name "
	"AND f_geometry_column = NEW.f_geometry_column "
	"AND style_id = NEW.style_id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* automatically setting the style_name after updating */
    sql = "CREATE TRIGGER sevstl_style_name_upd\n"
	"AFTER UPDATE OF style ON "
	"'SE_vector_styled_layers'\nFOR EACH ROW BEGIN\n"
	"UPDATE SE_vector_styled_layers "
	"SET style_name = XB_GetName(NEW.style) "
	"WHERE f_table_name = NEW.f_table_name "
	"AND f_geometry_column = NEW.f_geometry_column "
	"AND style_id = NEW.style_id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_raster_styled_layers (sqlite3 * sqlite, int relaxed)
{
/* creating the SE_raster_styled_layers table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_raster_styled_layers (\n"
	"coverage_name TEXT NOT NULL,\n"
	"style_id INTEGER NOT NULL,\n"
	"style_name TEXT NOT NULL DEFAULT 'missing_name',\n"
	"style BLOB NOT NULL,\n"
	"CONSTRAINT pk_serstl PRIMARY KEY " "(coverage_name, style_id),\n"
	"CONSTRAINT fk_serstl FOREIGN KEY (coverage_name) "
	"REFERENCES raster_coverages (coverage_name) " "ON DELETE CASCADE)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_raster_styled_layers' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the layer-style UNIQUE index */
    sql = "CREATE UNIQUE INDEX idx_raster_style ON SE_raster_styled_layers "
	"(coverage_name, style_name)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX 'idx_raster_style' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_raster_styled_layers triggers */
    sql = "CREATE TRIGGER serstl_coverage_name_insert\n"
	"BEFORE INSERT ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER serstl_coverage_name_update\n"
	"BEFORE UPDATE OF 'coverage_name' ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Raster Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	      "not an XML Schema Validated SLD/SE Raster Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER serstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_raster_styled_layers violates constraint: "
	      "not a valid SLD/SE Raster Style')\n"
	      "WHERE XB_IsSldSeRasterStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* automatically setting the style_name after inserting */
    sql = "CREATE TRIGGER serstl_style_name_ins\n"
	"AFTER INSERT ON 'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	"UPDATE SE_raster_styled_layers "
	"SET style_name = XB_GetName(NEW.style) "
	"WHERE coverage_name = NEW.coverage_name "
	"AND style_id = NEW.style_id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* automatically setting the style_name after updating */
    sql = "CREATE TRIGGER serstl_style_name_upd\n"
	"AFTER UPDATE OF style ON "
	"'SE_raster_styled_layers'\nFOR EACH ROW BEGIN\n"
	"UPDATE SE_raster_styled_layers "
	"SET style_name = XB_GetName(NEW.style) "
	"WHERE coverage_name = NEW.coverage_name "
	"AND style_id = NEW.style_id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_styled_groups (sqlite3 * sqlite)
{
/* creating the SE_styled_groups table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_styled_groups (\n"
	"group_name TEXT NOT NULL PRIMARY KEY,\n"
	"title TEXT NOT NULL DEFAULT '*** undefined ***',\n"
	"abstract TEXT NOT NULL DEFAULT '*** undefined ***')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_styled_groups' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_styled_groups triggers */
    sql = "CREATE TRIGGER segrp_group_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_groups'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_groups violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_groups violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_groups violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrp_group_name_update\n"
	"BEFORE UPDATE OF 'group_name' ON 'SE_styled_groups'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_groups violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_groups violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_groups violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_styled_group_refs (sqlite3 * sqlite)
{
/* creating the SE_styled_group_refs table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_styled_group_refs (\n"
	"id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
	"group_name TEXT NOT NULL,\n"
	"paint_order INTEGER NOT NULL,\n"
	"f_table_name TEXT,\n"
	"f_geometry_column TEXT,\n"
	"coverage_name TEXT,\n"
	"CONSTRAINT fk_se_refs FOREIGN KEY (group_name) "
	"REFERENCES SE_styled_groups (group_name) "
	"ON DELETE CASCADE,\n"
	"CONSTRAINT fk_se_group_vector FOREIGN KEY "
	"(f_table_name, f_geometry_column) "
	"REFERENCES geometry_columns "
	"(f_table_name, f_geometry_column) "
	"ON DELETE CASCADE,\n"
	"CONSTRAINT fk_se_group_raster "
	"FOREIGN KEY (coverage_name) "
	"REFERENCES raster_coverages (coverage_name) " "ON DELETE CASCADE)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TABLE 'SE_styled_group_refs' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_styled_group_refs triggers */
    sql = "CREATE TRIGGER segrrefs_group_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_group_name_update\n"
	"BEFORE UPDATE OF 'group_name' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_table_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_table_name_update\n"
	"BEFORE UPDATE OF 'f_table_name' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a single quote')\n"
	"WHERE NEW.f_table_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_table_name value must not contain a double quote')\n"
	"WHERE NEW.f_table_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_table_name value must be lower case')\n"
	"WHERE NEW.f_table_name <> lower(NEW.f_table_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_geometry_column_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_f_geometry_column_update\n"
	"BEFORE UPDATE OF 'f_geometry_column' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a single quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must not contain a double quote')\n"
	"WHERE NEW.f_geometry_column LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"f_geometry_column value must be lower case')\n"
	"WHERE NEW.f_geometry_column <> lower(NEW.f_geometry_column);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_coverage_name_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_coverage_name_update\n"
	"BEFORE UPDATE OF 'coverage_name' ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a single quote')\n"
	"WHERE NEW.coverage_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"coverage_name value must not contain a double quote')\n"
	"WHERE NEW.coverage_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"coverage_name value must be lower case')\n"
	"WHERE NEW.coverage_name <> lower(NEW.coverage_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_insert\n"
	"BEFORE INSERT ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_styled_group_refs violates constraint: "
	"cannot reference both Vector and Raster at the same time')\n"
	"WHERE (NEW.f_table_name IS NOT NULL OR NEW.f_geometry_column IS NOT NULL) "
	"AND NEW.coverage_name IS NOT NULL;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrrefs_update\n"
	"BEFORE UPDATE ON 'SE_styled_group_refs'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_styled_group_refs violates constraint: "
	"cannot reference both Vector and Raster at the same time')\n"
	"WHERE (NEW.f_table_name IS NOT NULL OR NEW.f_geometry_column IS NOT NULL) "
	"AND NEW.coverage_name IS NOT NULL;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating any Index on SE_styled_group_refs */
    sql = "CREATE INDEX idx_SE_styled_vgroups ON "
	"SE_styled_group_refs " "(f_table_name, f_geometry_column)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Create Index 'idx_SE_styled_vgroups' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE INDEX idx_SE_styled_rgroups ON "
	"SE_styled_group_refs " "(coverage_name)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Create Index 'idx_SE_styled_rgroups' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE INDEX idx_SE_styled_groups_paint ON "
	"SE_styled_group_refs " "(group_name, paint_order)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Create Index 'idx_SE_styled_groups_paint' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_group_styles (sqlite3 * sqlite, int relaxed)
{
/* creating the SE_group_styles table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE SE_group_styles (\n"
	"group_name TEXT NOT NULL,\n"
	"style_id INTEGER NOT NULL,\n"
	"style_name TEXT NOT NULL DEFAULT 'missing_name',\n"
	"style BLOB NOT NULL,\n"
	"CONSTRAINT pk_segrpstl PRIMARY KEY " "(group_name, style_id),\n"
	"CONSTRAINT fk_segrpstl FOREIGN KEY (group_name) "
	"REFERENCES SE_styled_groups (group_name) " "ON DELETE CASCADE)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'SE_group_styles' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the layer-style UNIQUE index */
    sql = "CREATE UNIQUE INDEX idx_group_style ON SE_group_styles "
	"(group_name, style_name)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX 'idx_group_style' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the SE_group_styles triggers */
    sql = "CREATE TRIGGER segrpstl_group_name_insert\n"
	"BEFORE INSERT ON 'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on SE_group_styles violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on SE_group_styles violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on SE_group_styles violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER segrpstl_group_name_update\n"
	"BEFORE UPDATE OF 'group_name' ON 'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on SE_group_styles violates constraint: "
	"group_name value must not contain a single quote')\n"
	"WHERE NEW.group_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on SE_group_styles violates constraint: "
	"group_name value must not contain a double quote')\n"
	"WHERE NEW.group_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on SE_group_styles violates constraint: "
	"group_name value must be lower case')\n"
	"WHERE NEW.group_name <> lower(NEW.group_name);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER segrpstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_group_styles violates constraint: "
	      "not a valid SLD Style')\n"
	      "WHERE XB_IsSldStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'insert on SE_group_styles violates constraint: "
	      "not an XML Schema Validated SLD Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER segrpstl_style_insert\n"
	      "BEFORE INSERT ON 'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on SE_group_styles violates constraint: "
	      "not a valid SLD Style')\n"
	      "WHERE XB_IsSldStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER segrpstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_group_styles violates constraint: "
	      "not a valid SLD Style')\n"
	      "WHERE XB_IsSldStyle(NEW.style) <> 1;\n"
	      "SELECT RAISE(ABORT,'update on SE_group_styles violates constraint: "
	      "not an XML Schema Validated SLD Style')\n"
	      "WHERE XB_IsSchemaValidated(NEW.style) <> 1;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER segrpstl_style_update\n"
	      "BEFORE UPDATE ON 'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on SE_group_styles violates constraint: "
	      "not a valid SLD Raster Style')\n"
	      "WHERE XB_IsSldStyle(NEW.style) <> 1;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* automatically setting the style_name after inserting */
    sql = "CREATE TRIGGER segrpstl_style_name_ins\n"
	"AFTER INSERT ON 'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	"UPDATE SE_group_styles "
	"SET style_name = XB_GetName(NEW.style) "
	"WHERE group_name = NEW.group_name "
	"AND style_id = NEW.style_id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* automatically setting the style_name after updating */
    sql = "CREATE TRIGGER segrpstl_style_name_upd\n"
	"AFTER UPDATE OF style ON "
	"'SE_group_styles'\nFOR EACH ROW BEGIN\n"
	"UPDATE SE_group_styles "
	"SET style_name = XB_GetName(NEW.style) "
	"WHERE group_name = NEW.group_name "
	"AND style_id = NEW.style_id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_external_graphics_view (sqlite3 * sqlite)
{
/* creating the SE_external_graphics_view view */
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    sql_statement =
	sqlite3_mprintf
	("CREATE VIEW SE_external_graphics_view AS\n"
	 "SELECT xlink_href AS xlink_href, title AS title, "
	 "abstract AS abstract, resource AS resource, "
	 "file_name AS file_name, GetMimeType(resource) AS mime_type\n"
	 "FROM SE_external_graphics");
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_external_graphics_view' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_vector_styled_layers_view (sqlite3 * sqlite)
{
/* creating the SE_vector_styled_layers_view view */
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    sql_statement =
	sqlite3_mprintf ("CREATE VIEW SE_vector_styled_layers_view AS \n"
			 "SELECT f_table_name AS f_table_name, f_geometry_column AS f_geometry_column, "
			 "style_id AS style_id, style_name AS name, XB_GetTitle(style) AS title, "
			 "XB_GetAbstract(style) AS abstract, style AS style, "
			 "XB_IsSchemaValidated(style) AS schema_validated, "
			 "XB_GetSchemaURI(style) AS schema_uri\n"
			 "FROM SE_vector_styled_layers");
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_vector_styled_layers_view' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_raster_styled_layers_view (sqlite3 * sqlite)
{
/* creating the SE_raster_styled_layers_view view */
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    sql_statement =
	sqlite3_mprintf ("CREATE VIEW SE_raster_styled_layers_view AS \n"
			 "SELECT coverage_name AS coverage_name, style_id AS style_id, "
			 "style_name AS name, XB_GetTitle(style) AS title, "
			 "XB_GetAbstract(style) AS abstract, style AS style, "
			 "XB_IsSchemaValidated(style) AS schema_validated, "
			 "XB_GetSchemaURI(style) AS schema_uri\n"
			 "FROM SE_raster_styled_layers");
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_raster_styled_layers_view' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_styled_groups_view (sqlite3 * sqlite)
{
/* creating the SE_styled_groups_view view */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE VIEW SE_styled_groups_view AS "
	"SELECT g.group_name AS group_name, g.title AS group_title, "
	"g.abstract AS group_abstract, gr.paint_order AS paint_order, "
	"'vector' AS type, v.f_table_name AS layer_name, "
	"v.f_geometry_column AS geometry_column, "
	"v.geometry_type AS geometry_type, v.coord_dimension AS coord_dimension, "
	"v.srid AS srid FROM SE_styled_groups AS g "
	"JOIN SE_styled_group_refs AS gr ON (g.group_name = gr.group_name) "
	"JOIN geometry_columns AS v ON (gr.f_table_name = v.f_table_name "
	"AND gr.f_geometry_column = v.f_geometry_column) UNION "
	"SELECT g.group_name AS group_name, g.title AS group_title, "
	"g.abstract AS group_abstract, gr.paint_order AS paint_order, "
	"'raster' AS type, r.coverage_name AS layer_name, NULL AS geometry_column, "
	"NULL AS geometry_type, NULL AS coord_dimension, r.srid AS srid "
	"FROM SE_styled_groups AS g "
	"JOIN SE_styled_group_refs AS gr ON (g.group_name = gr.group_name) "
	"JOIN raster_coverages AS r ON (gr.coverage_name = r.coverage_name)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_styled_groups_view' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_group_styles_view (sqlite3 * sqlite)
{
/* creating the SE_group_styles_view view */
    char *sql_statement;
    int ret;
    char *err_msg = NULL;
    sql_statement =
	sqlite3_mprintf ("CREATE VIEW SE_group_styles_view AS \n"
			 "SELECT group_name AS group_name, style_id AS style_id, "
			 "style_name AS name, XB_GetTitle(style) AS title, "
			 "XB_GetAbstract(style) AS abstract, style AS style, "
			 "XB_IsSchemaValidated(style) AS schema_validated, "
			 "XB_GetSchemaURI(style) AS schema_uri\n"
			 "FROM SE_group_styles");
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &err_msg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE VIEW 'SE_group_styles_view' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE int
createStylingTables (void *p_sqlite, int relaxed)
{
/* Creating the SE Styling tables */
    const char *tables[12];
    int views[11];
    const char **p_tbl;
    int *p_view;
    int ok_table;
    sqlite3 *sqlite = p_sqlite;

/* checking SLD/SE Styling tables */
    tables[0] = "SE_external_graphics";
    tables[1] = "SE_vector_styled_layers";
    tables[2] = "SE_raster_styled_layers";
    tables[3] = "SE_styled_groups";
    tables[4] = "SE_styled_group_refs";
    tables[5] = "SE_group_styles";
    tables[6] = "SE_external_graphics_view";
    tables[7] = "SE_vector_styled_layers_view";
    tables[8] = "SE_raster_styled_layers_view";
    tables[9] = "SE_styled_groups_view";
    tables[10] = "SE_group_styles_view";
    tables[11] = NULL;
    views[0] = 0;
    views[1] = 0;
    views[2] = 0;
    views[3] = 0;
    views[4] = 0;
    views[5] = 0;
    views[6] = 1;
    views[7] = 1;
    views[8] = 1;
    views[9] = 1;
    views[10] = 1;
    p_tbl = tables;
    p_view = views;
    while (*p_tbl != NULL)
      {
	  ok_table = check_styling_table (sqlite, *p_tbl, *p_view);
	  if (ok_table)
	    {
		spatialite_e
		    ("CreateStylingTables() error: table '%s' already exists\n",
		     *p_tbl);
		goto error;
	    }
	  p_tbl++;
	  p_view++;
      }

/* creating the SLD/SE Styling tables */
    if (!check_raster_coverages (sqlite))
      {
	  /* creating the main RasterCoverages table as well */
	  if (!create_raster_coverages (sqlite))
	      goto error;
      }
    if (!create_external_graphics (sqlite))
	goto error;
    if (!create_vector_styled_layers (sqlite, relaxed))
	goto error;
    if (!create_raster_styled_layers (sqlite, relaxed))
	goto error;
    if (!create_styled_groups (sqlite))
	goto error;
    if (!create_styled_group_refs (sqlite))
	goto error;
    if (!create_group_styles (sqlite, relaxed))
	goto error;
    if (!create_external_graphics_view (sqlite))
	goto error;
    if (!create_vector_styled_layers_view (sqlite))
	goto error;
    if (!create_raster_styled_layers_view (sqlite))
	goto error;
    if (!create_styled_groups_view (sqlite))
	goto error;
    if (!create_group_styles_view (sqlite))
	goto error;
    return 1;

  error:
    return 0;
}

static int
check_iso_metadata_table (sqlite3 * sqlite, const char *table, int is_view)
{
/* checking if some ISO Metadata-related table/view already exists */
    int exists = 0;
    char *sql_statement;
    char *errMsg = NULL;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    sql_statement =
	sqlite3_mprintf ("SELECT name FROM sqlite_master WHERE type = '%s'"
			 "AND Upper(name) = Upper(%Q)",
			 (!is_view) ? "table" : "view", table);
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	exists = 1;
    sqlite3_free_table (results);
    return exists;
}

static int
create_iso_metadata (sqlite3 * sqlite, int relaxed)
{
/* creating the ISO_metadata table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE ISO_metadata (\n"
	"id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
	"md_scope TEXT NOT NULL DEFAULT 'dataset',\n"
	"metadata BLOB NOT NULL DEFAULT (zeroblob(4)),\n"
	"fileId TEXT,\nparentId TEXT)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'ISO_metadata' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* adding the Geometry column */
    sql =
	"SELECT AddGeometryColumn('ISO_metadata', 'geometry', 4326, 'MULTIPOLYGON', 'XY')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      (" AddGeometryColumn 'ISO_metadata'.'geometry' error:%s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* adding a Spatial Index */
    sql = "SELECT CreateSpatialIndex ('ISO_metadata', 'geometry')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex 'ISO_metadata'.'geometry' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the ISO_metadata triggers */
    sql = "CREATE TRIGGER 'ISO_metadata_md_scope_insert'\n"
	"BEFORE INSERT ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'insert on table ISO_metadata violates constraint: "
	"md_scope must be one of ''undefined'' | ''fieldSession'' | ''collectionSession'' "
	"| ''series'' | ''dataset'' | ''featureType'' | ''feature'' | ''attributeType'' "
	"| ''attribute'' | ''tile'' | ''model'' | ''catalogue'' | ''schema'' "
	"| ''taxonomy'' | ''software'' | ''service'' | ''collectionHardware'' "
	"| ''nonGeographicDataset'' | ''dimensionGroup''')\n"
	"WHERE NOT(NEW.md_scope IN ('undefined','fieldSession','collectionSession',"
	"'series','dataset','featureType','feature','attributeType','attribute',"
	"'tile','model','catalogue','schema','taxonomy','software','service',"
	"'collectionHardware','nonGeographicDataset','dimensionGroup'));\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_md_scope_update'\n"
	"BEFORE UPDATE OF 'md_scope' ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'update on table ISO_metadata violates constraint: "
	"md_scope must be one of ''undefined'' | ''fieldSession'' | ''collectionSession'' "
	"| ''series'' | ''dataset'' | ''featureType'' | ''feature'' | ''attributeType'' |"
	" ''attribute'' | ''tile'' | ''model'' | ''catalogue'' | ''schema'' "
	"| ''taxonomy'' | ''software'' | ''service'' | ''collectionHardware'' "
	"| ''nonGeographicDataset'' | ''dimensionGroup''')\n"
	"WHERE NOT(NEW.md_scope IN ('undefined','fieldSession','collectionSession',"
	"'series','dataset','featureType','feature','attributeType','attribute',"
	"'tile','model','catalogue','schema','taxonomy','software','service',"
	"'collectionHardware','nonGeographicDataset','dimensionGroup'));\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_fileIdentifier_insert'\n"
	"AFTER INSERT ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	"UPDATE ISO_metadata SET fileId = XB_GetFileId(NEW.metadata), "
	"parentId = XB_GetParentId(NEW.metadata), "
	"geometry = XB_GetGeometry(NEW.metadata) WHERE id = NEW.id;\n"
	"UPDATE ISO_metadata_reference "
	"SET md_parent_id = GetIsoMetadataId(NEW.parentId) "
	"WHERE md_file_id = NEW.id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_fileIdentifier_update'\n"
	"AFTER UPDATE ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	"UPDATE ISO_metadata SET fileId = XB_GetFileId(NEW.metadata), "
	"parentId = XB_GetParentId(NEW.metadata), "
	"geometry = XB_GetGeometry(NEW.metadata) WHERE id = NEW.id;\n"
	"UPDATE ISO_metadata_reference "
	"SET md_parent_id = GetIsoMetadataId(NEW.parentId) "
	"WHERE md_file_id = NEW.id;\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER ISO_metadata_insert\n"
	      "BEFORE INSERT ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on ISO_metadata violates constraint: "
	      "not a valid ISO Metadata XML')\n"
	      "WHERE XB_IsIsoMetadata(NEW.metadata) <> 1 AND NEW.id <> 0;\n"
	      "SELECT RAISE(ABORT,'insert on ISO_metadata violates constraint: "
	      "not an XML Schema Validated ISO Metadata')\n"
	      "WHERE XB_IsSchemaValidated(NEW.metadata) <> 1 AND NEW.id <> 0;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER ISO_metadata_insert\n"
	      "BEFORE INSERT ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'insert on ISO_metadata violates constraint: "
	      "not a valid ISO Metadata XML')\n"
	      "WHERE XB_IsIsoMetadata(NEW.metadata) <> 1 AND NEW.id <> 0;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if (relaxed == 0)
      {
	  /* strong trigger - imposing XML schema validation */
	  sql = "CREATE TRIGGER ISO_metadata_update\n"
	      "BEFORE UPDATE ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on ISO_metadata violates constraint: "
	      "not a valid ISO Metadata XML')\n"
	      "WHERE XB_IsIsoMetadata(NEW.metadata) <> 1 AND NEW.id <> 0;\n"
	      "SELECT RAISE(ABORT,'update on ISO_metadata violates constraint: "
	      "not an XML Schema Validated ISO Metadata')\n"
	      "WHERE XB_IsSchemaValidated(NEW.metadata) <> 1 AND NEW.id <> 0;\nEND";
      }
    else
      {
	  /* relaxed trigger - not imposing XML schema validation */
	  sql = "CREATE TRIGGER ISO_metadata_update\n"
	      "BEFORE UPDATE ON 'ISO_metadata'\nFOR EACH ROW BEGIN\n"
	      "SELECT RAISE(ABORT,'update on ISO_metadata violates constraint: "
	      "not a valid ISO Metadata XML')\n"
	      "WHERE XB_IsIsoMetadata(NEW.metadata) <> 1 AND NEW.id <> 0;\nEND";
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating any Index on ISO_metadata */
    sql = "CREATE UNIQUE INDEX idx_ISO_metadata_ids ON "
	"ISO_metadata (fileId)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Create Index 'idx_ISO_metadata_ids' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE INDEX idx_ISO_metadata_parents ON " "ISO_metadata (parentId)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Create Index 'idx_ISO_metadata_parents' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_iso_metadata_reference (sqlite3 * sqlite)
{
/* creating the ISO_metadata_reference table */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE TABLE ISO_metadata_reference (\n"
	"reference_scope TEXT NOT NULL DEFAULT 'table',\n"
	"table_name TEXT NOT NULL DEFAULT 'undefined',\n"
	"column_name TEXT NOT NULL DEFAULT 'undefined',\n"
	"row_id_value INTEGER NOT NULL DEFAULT 0,\n"
	"timestamp TEXT NOT NULL DEFAULT ("
	"strftime('%Y-%m-%dT%H:%M:%fZ',CURRENT_TIMESTAMP)),\n"
	"md_file_id INTEGER NOT NULL DEFAULT 0,\n"
	"md_parent_id INTEGER NOT NULL DEFAULT 0,\n"
	"CONSTRAINT fk_isometa_mfi FOREIGN KEY (md_file_id) "
	"REFERENCES ISO_metadata(id),\n"
	"CONSTRAINT fk_isometa_mpi FOREIGN KEY (md_parent_id) "
	"REFERENCES ISO_metadata(id))";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE 'ISO_metadata_reference' error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating the ISO_metadata_reference triggers */
    sql = "CREATE TRIGGER 'ISO_metadata_reference_scope_insert'\n"
	"BEFORE INSERT ON 'ISO_metadata_reference'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'insert on table ISO_metadata_reference violates constraint: "
	"reference_scope must be one of ''table'' | ''column'' | ''row'' | ''row/col''')\n"
	"WHERE NOT NEW.reference_scope IN ('table','column','row','row/col');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_reference_scope_update'\n"
	"BEFORE UPDATE OF 'reference_scope' ON 'ISO_metadata_reference'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'update on table ISO_metadata_reference violates constraint: "
	"referrence_scope must be one of ''table'' | ''column'' | ''row'' | ''row/col''')\n"
	"WHERE NOT NEW.reference_scope IN ('table','column','row','row/col');\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_reference_table_name_insert'\n"
	"BEFORE INSERT ON 'ISO_metadata_reference'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'insert on table ISO_metadata_reference violates constraint: "
	"table_name must be the name of a table in geometry_columns')\n"
	"WHERE NOT NEW.table_name IN (\n"
	"SELECT f_table_name AS table_name FROM geometry_columns);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_reference_table_name_update'\n"
	"BEFORE UPDATE OF 'table_name' ON 'ISO_metadata_reference'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'update on table ISO_metadata_reference violates constraint: "
	"table_name must be the name of a table in geometry_columns')\n"
	"WHERE NOT NEW.table_name IN (\n"
	"SELECT f_table_name AS table_name FROM geometry_columns);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_reference_row_id_value_insert'\n"
	"BEFORE INSERT ON 'ISO_metadata_reference'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'insert on ISO_table ISO_metadata_reference violates constraint: "
	"row_id_value must be 0 when reference_scope is ''table'' or ''column''')\n"
	"WHERE NEW.reference_scope IN ('table','column') AND NEW.row_id_value <> 0;\n"
	"SELECT RAISE(ROLLBACK, 'insert on table ISO_metadata_reference violates constraint: "
	"row_id_value must exist in specified table when reference_scope is ''row'' or ''row/col''')\n"
	"WHERE NEW.reference_scope IN ('row','row/col') AND NOT EXISTS\n"
	"(SELECT rowid FROM (SELECT NEW.table_name AS table_name) "
	"WHERE rowid = NEW.row_id_value);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_reference_row_id_value_update'\n"
	"BEFORE UPDATE OF 'row_id_value' ON 'ISO_metadata_reference'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'update on table ISO_metadata_reference violates constraint: "
	"row_id_value must be 0 when reference_scope is ''table'' or ''column''')\n"
	"WHERE NEW.reference_scope IN ('table','column') AND NEW.row_id_value <> 0;\n"
	"SELECT RAISE(ROLLBACK, 'update on ISO_table metadata_reference violates constraint: "
	"row_id_value must exist in specified table when reference_scope is ''row'' or ''row/col''')\n"
	"WHERE NEW.reference_scope IN ('row','row/col') AND NOT EXISTS\n"
	"(SELECT rowid FROM (SELECT NEW.table_name AS table_name) "
	"WHERE rowid = NEW.row_id_value);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_reference_timestamp_insert'\n"
	"BEFORE INSERT ON 'ISO_metadata_reference'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'insert on table ISO_metadata_reference violates constraint: "
	"timestamp must be a valid time in ISO 8601 ''yyyy-mm-ddThh:mm:ss.cccZ'' form')\n"
	"WHERE NOT (NEW.timestamp GLOB'[1-2][0-9][0-9][0-9]-[0-1][0-9]-[1-3][0-9]T"
	"[0-2][0-9]:[0-5][0-9]:[0-5][0-9].[0-9][0-9][0-9]Z' AND strftime('%s',"
	"NEW.timestamp) NOT NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER 'ISO_metadata_reference_timestamp_update'\n"
	"BEFORE UPDATE OF 'timestamp' ON 'ISO_metadata_reference'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'update on table ISO_metadata_reference violates constraint: "
	"timestamp must be a valid time in ISO 8601 ''yyyy-mm-ddThh:mm:ss.cccZ'' form')\n"
	"WHERE NOT (NEW.timestamp GLOB'[1-2][0-9][0-9][0-9]-[0-1][0-9]-[1-3][0-9]T"
	"[0-2][0-9]:[0-5][0-9]:[0-5][0-9].[0-9][0-9][0-9]Z' AND strftime('%s',"
	"NEW.timestamp) NOT NULL);\nEND";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
/* creating any Index on ISO_metadata_reference */
    sql = "CREATE INDEX idx_ISO_metadata_reference_ids ON "
	"ISO_metadata_reference (md_file_id)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Create Index 'idx_ISO_metadata_reference_ids' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE INDEX idx_ISO_metadata_reference_parents ON "
	"ISO_metadata_reference (md_parent_id)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Create Index 'idx_ISO_metadata_reference_parents' error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_iso_metadata_view (sqlite3 * sqlite)
{
/* creating the ISO_metadata_view view */
    char *sql;
    int ret;
    char *err_msg = NULL;
    sql = "CREATE VIEW ISO_metadata_view AS\n"
	"SELECT id AS id, md_scope AS md_scope, XB_GetTitle(metadata) AS title, "
	"XB_GetAbstract(metadata) AS abstract, geometry AS geometry, "
	"fileId AS fileIdentifier, parentId AS parentIdentifier, metadata AS metadata, "
	"XB_IsSchemaValidated(metadata) AS schema_validated, "
	"XB_GetSchemaURI(metadata) AS metadata_schema_URI\n"
	"FROM ISO_metadata";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW 'ISO_metadata_view' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE int
createIsoMetadataTables (void *p_sqlite, int relaxed)
{
/* Creating the ISO Metadata tables */
    const char *tables[4];
    int views[3];
    const char **p_tbl;
    int *p_view;
    int ok_table;
    int ret;
    char *err_msg = NULL;
    sqlite3 *sqlite = p_sqlite;

/* checking ISO Metadata tables */
    tables[0] = "ISO_metadata";
    tables[1] = "ISO_metadata_reference";
    tables[2] = "ISO_metadata_view";
    tables[3] = NULL;
    views[0] = 0;
    views[1] = 0;
    views[2] = 1;
    p_tbl = tables;
    p_view = views;
    while (*p_tbl != NULL)
      {
	  ok_table = check_iso_metadata_table (sqlite, *p_tbl, *p_view);
	  if (ok_table)
	    {
		spatialite_e
		    ("CreateIsoMetadataTables() error: table '%s' already exists\n",
		     *p_tbl);
		goto error;
	    }
	  p_tbl++;
	  p_view++;
      }

/* creating the ISO Metadata tables */
    if (!create_iso_metadata (sqlite, relaxed))
	goto error;
    if (!create_iso_metadata_reference (sqlite))
	goto error;
    if (!create_iso_metadata_view (sqlite))
	goto error;
/* inserting the default "undef" row into ISO_metadata */
    ret =
	sqlite3_exec (sqlite,
		      "INSERT INTO ISO_metadata (id, md_scope) VALUES (0, 'undefined')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Insert default 'undefined' ISO_metadata row - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;

  error:
    return 0;
}

SPATIALITE_PRIVATE int
register_external_graphic (void *p_sqlite, const char *xlink_href,
			   const unsigned char *p_blob, int n_bytes,
			   const char *title, const char *abstract,
			   const char *file_name)
{
/* auxiliary function: inserts or updates an ExternalGraphic Resource */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int extras = 0;
    int retval = 0;

/* checking if already exists */
    sql = "SELECT xlink_href FROM SE_external_graphics WHERE xlink_href = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerExternalGraphic: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists = 1;
      }
    sqlite3_finalize (stmt);

    if (title != NULL && abstract != NULL && file_name != NULL)
	extras = 1;
    if (exists)
      {
	  /* update */
	  if (extras)
	    {
		/* full infos */
		sql = "UPDATE SE_external_graphics "
		    "SET resource = ?, title = ?, abstract = ?, file_name = ? "
		    "WHERE xlink_href = ?";
	    }
	  else
	    {
		/* limited basic infos */
		sql = "UPDATE SE_external_graphics "
		    "SET resource = ? WHERE xlink_href = ?";
	    }
      }
    else
      {
	  /* insert */
	  if (extras)
	    {
		/* full infos */
		sql = "INSERT INTO SE_external_graphics "
		    "(xlink_href, resource, title, abstract, file_name) "
		    "VALUES (?, ?, ?, ?, ?)";
	    }
	  else
	    {
		/* limited basic infos */
		sql = "INSERT INTO SE_external_graphics "
		    "(xlink_href, resource) VALUES (?, ?)";
	    }
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerExternalGraphic: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  if (extras)
	    {
		/* full infos */
		sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
		sqlite3_bind_text (stmt, 2, title, strlen (title),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 3, abstract, strlen (abstract),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 4, file_name, strlen (file_name),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 5, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
	    }
	  else
	    {
		/* limited basic infos */
		sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
		sqlite3_bind_text (stmt, 2, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
	    }
      }
    else
      {
	  /* insert */
	  if (extras)
	    {
		/* full infos */
		sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
		sqlite3_bind_blob (stmt, 2, p_blob, n_bytes, SQLITE_STATIC);
		sqlite3_bind_text (stmt, 3, title, strlen (title),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 4, abstract, strlen (abstract),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 5, file_name, strlen (file_name),
				   SQLITE_STATIC);
	    }
	  else
	    {
		/* limited basic infos */
		sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
		sqlite3_bind_blob (stmt, 2, p_blob, n_bytes, SQLITE_STATIC);
	    }
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerExternalGraphic() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_vector_styled_layer (void *p_sqlite, const char *f_table_name,
			      const char *f_geometry_column, int style_id,
			      const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: inserts or updates a Vector Styled Layer */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

    if (style_id >= 0)
      {
	  /* checking if already exists */
	  sql = "SELECT style_id FROM SE_vector_styled_layers "
	      "WHERE f_table_name = Lower(?) AND f_geometry_column = Lower(?) "
	      "AND style_id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorStyledLayer: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, f_table_name, strlen (f_table_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_geometry_column,
			     strlen (f_geometry_column), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, style_id);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		    exists = 1;
	    }
	  sqlite3_finalize (stmt);
      }
    else
      {
	  /* assigning the next style_id value */
	  style_id = 0;
	  sql = "SELECT Max(style_id) FROM SE_vector_styled_layers "
	      "WHERE f_table_name = Lower(?) AND f_geometry_column = Lower(?) ";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorStyledLayer: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, f_table_name, strlen (f_table_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_geometry_column,
			     strlen (f_geometry_column), SQLITE_STATIC);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
			  style_id = sqlite3_column_int (stmt, 0) + 1;
		  }
	    }
	  sqlite3_finalize (stmt);
      }

    if (exists)
      {
	  /* update */
	  sql = "UPDATE SE_vector_styled_layers SET style = ? "
	      "WHERE f_table_name = Lower(?) AND f_geometry_column = Lower(?) "
	      "AND style_id = ?";
      }
    else
      {
	  /* insert */
	  sql = "INSERT INTO SE_vector_styled_layers "
	      "(f_table_name, f_geometry_column, style_id, style) VALUES "
	      "(?, ?, ?, ?)";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerVectorStyledLayer: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_table_name, strlen (f_table_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, f_geometry_column,
			     strlen (f_geometry_column), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 4, style_id);
      }
    else
      {
	  /* insert */
	  sqlite3_bind_text (stmt, 1, f_table_name, strlen (f_table_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_geometry_column,
			     strlen (f_geometry_column), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, style_id);
	  sqlite3_bind_blob (stmt, 4, p_blob, n_bytes, SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerVectorStyledLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_raster_styled_layer (void *p_sqlite, const char *coverage_name,
			      int style_id, const unsigned char *p_blob,
			      int n_bytes)
{
/* auxiliary function: inserts or updates a Raster Styled Layer */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

    if (style_id >= 0)
      {
	  /* checking if already exists */
	  sql = "SELECT style_id FROM SE_raster_styled_layers "
	      "WHERE coverage_name = Lower(?) AND style_id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerRasterStyledLayer: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 2, style_id);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		    exists = 1;
	    }
	  sqlite3_finalize (stmt);
      }
    else
      {
	  /* assigning the next style_id value */
	  style_id = 0;
	  sql = "SELECT Max(style_id) FROM SE_raster_styled_layers "
	      "WHERE coverage_name = Lower(?) ";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorStyledLayer: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
			  style_id = sqlite3_column_int (stmt, 0) + 1;
		  }
	    }
	  sqlite3_finalize (stmt);
      }

    if (exists)
      {
	  /* update */
	  sql = "UPDATE SE_raster_styled_layers SET style = ? "
	      "WHERE coverage_name = Lower(?) AND style_id = ?";
      }
    else
      {
	  /* insert */
	  sql = "INSERT INTO SE_raster_styled_layers "
	      "(coverage_name, style_id, style) VALUES (?, ?, ?)";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerRasterStyledLayer: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, style_id);
      }
    else
      {
	  /* insert */
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 2, style_id);
	  sqlite3_bind_blob (stmt, 3, p_blob, n_bytes, SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerRasterStyledLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_styled_group (void *p_sqlite, const char *group_name,
		       const char *f_table_name,
		       const char *f_geometry_column,
		       const char *coverage_name, int paint_order)
{
/* auxiliary function: inserts or updates a Styled Group Item */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists_group = 0;
    int exists = 0;
    int retval = 0;
    sqlite3_int64 id;

    /* checking if the Group already exists */
    sql = "SELECT group_name FROM SE_styled_groups "
	"WHERE group_name = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerStyledGroupsRefs: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists_group = 1;
      }
    sqlite3_finalize (stmt);

    if (!exists_group)
      {
	  /* insert group */
	  sql = "INSERT INTO SE_styled_groups (group_name) VALUES (?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerStyledGroupsRefs: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      retval = 1;
	  else
	      spatialite_e ("registerStyledGroupsRefs() error: \"%s\"\n",
			    sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  if (retval == 0)
	      goto stop;
	  retval = 0;
      }

    if (paint_order >= 0)
      {
	  /* checking if the group-item already exists */
	  sql = "SELECT id FROM SE_styled_group_refs "
	      "WHERE group_name = Lower(?) AND paint_order = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerStyledGroupsRefs: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 2, paint_order);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      id = sqlite3_column_int64 (stmt, 0);
		      exists++;
		  }
	    }
	  sqlite3_finalize (stmt);
	  if (exists != 1)
	      goto stop;
      }

    if (paint_order < 0)
      {
	  /* assigning the next paint_order value */
	  paint_order = 0;
	  sql = "SELECT Max(paint_order) FROM SE_styled_group_refs "
	      "WHERE group_name = Lower(?) ";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerStyledGroupsRefs: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
			  paint_order = sqlite3_column_int (stmt, 0) + 1;
		  }
	    }
	  sqlite3_finalize (stmt);
      }

    if (exists)
      {
	  /* update */
	  sql = "UPDATE SE_styled_group_refs SET paint_order = ? "
	      "WHERE id = ?";
      }
    else
      {
	  /* insert */
	  if (coverage_name == NULL)
	    {
		/* vector styled layer */
		sql = "INSERT INTO SE_styled_group_refs "
		    "(id, group_name, f_table_name, f_geometry_column, paint_order) "
		    "VALUES (NULL, ?, ?, ?, ?)";
	    }
	  else
	    {
		/* raster styled layer */
		sql = "INSERT INTO SE_styled_group_refs "
		    "(id, group_name, coverage_name, paint_order) "
		    "VALUES (NULL, ?, ?, ?)";
	    }
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerStyledGroupsRefs: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  sqlite3_bind_int (stmt, 1, paint_order);
	  sqlite3_bind_int64 (stmt, 2, id);
      }
    else
      {
	  /* insert */
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  if (coverage_name == NULL)
	    {
		/* vector styled layer */
		sqlite3_bind_text (stmt, 2, f_table_name,
				   strlen (f_table_name), SQLITE_STATIC);
		sqlite3_bind_text (stmt, 3, f_geometry_column,
				   strlen (f_geometry_column), SQLITE_STATIC);
		sqlite3_bind_int (stmt, 4, paint_order);
	    }
	  else
	    {
		/* raster styled layer */
		sqlite3_bind_text (stmt, 2, coverage_name,
				   strlen (coverage_name), SQLITE_STATIC);
		sqlite3_bind_int (stmt, 3, paint_order);
	    }
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerStyledGroupsRefs() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
styled_group_set_infos (void *p_sqlite, const char *group_name,
			const char *title, const char *abstract)
{
/* auxiliary function: inserts or updates the Styled Group descriptive infos */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

    /* checking if the Group already exists */
    sql = "SELECT group_name FROM SE_styled_groups "
	"WHERE group_name = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("styledGroupSetInfos: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists = 1;
      }
    sqlite3_finalize (stmt);

    if (!exists)
      {
	  /* insert group */
	  sql =
	      "INSERT INTO SE_styled_groups (group_name, title, abstract) VALUES (?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("styledGroupSetInfos: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      retval = 1;
	  else
	      spatialite_e ("styledGroupSetInfos() error: \"%s\"\n",
			    sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
      }
    else
      {
	  /* update group */
	  sql =
	      "UPDATE SE_styled_groups SET title = ?, abstract = ? WHERE group_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("styledGroupSetInfos: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      retval = 1;
	  else
	      spatialite_e ("styledGroupSetInfos() error: \"%s\"\n",
			    sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
      }
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_group_style (void *p_sqlite, const char *group_name, int style_id,
		      const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: inserts or updates a Group Style */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

    if (style_id >= 0)
      {
	  /* checking if already exists */
	  sql = "SELECT style_id FROM SE_group_styles "
	      "WHERE group_name = Lower(?) AND style_id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerGroupStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 2, style_id);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		    exists = 1;
	    }
	  sqlite3_finalize (stmt);
      }
    else
      {
	  /* assigning the next style_id value */
	  style_id = 0;
	  sql = "SELECT Max(style_id) FROM SE_group_styles "
	      "WHERE group_name = Lower(?) ";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerGroupStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
			  style_id = sqlite3_column_int (stmt, 0) + 1;
		  }
	    }
	  sqlite3_finalize (stmt);
      }

    if (exists)
      {
	  /* update */
	  sql = "UPDATE SE_group_styles SET style = ? "
	      "WHERE group_name = Lower(?) AND style_id = ?";
      }
    else
      {
	  /* insert */
	  sql = "INSERT INTO SE_group_styles "
	      "(group_name, style_id, style) VALUES (?, ?, ?)";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerGroupStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, style_id);
      }
    else
      {
	  /* insert */
	  sqlite3_bind_text (stmt, 1, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 2, style_id);
	  sqlite3_bind_blob (stmt, 3, p_blob, n_bytes, SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerGroupStyled() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
get_iso_metadata_id (void *p_sqlite, const char *fileIdentifier, void *p_id)
{
/* auxiliary function: return the ID of the row corresponding to "fileIdentifier" */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 *p64 = (sqlite3_int64 *) p_id;
    sqlite3_int64 id;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int ok = 0;

    sql = "SELECT id FROM ISO_metadata WHERE fileId = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("getIsoMetadataId: \"%s\"\n", sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, fileIdentifier, strlen (fileIdentifier),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		ok++;
		id = sqlite3_column_int64 (stmt, 0);
	    }
      }
    sqlite3_finalize (stmt);

    if (ok == 1)
      {
	  *p64 = id;
	  return 1;
      }
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_iso_metadata (void *p_sqlite, const char *scope,
		       const unsigned char *p_blob, int n_bytes, void *p_id,
		       const char *fileIdentifier)
{
/* auxiliary function: inserts or updates an ISO Metadata */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 *p64 = (sqlite3_int64 *) p_id;
    sqlite3_int64 id = *p64;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

    if (id >= 0)
      {
	  /* checking if already exists - by ID */
	  sql = "SELECT id FROM ISO_metadata WHERE id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerIsoMetadata: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, id);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		    exists = 1;
	    }
	  sqlite3_finalize (stmt);
      }
    if (fileIdentifier != NULL)
      {
	  /* checking if already exists - by fileIdentifier */
	  sql = "SELECT id FROM ISO_metadata WHERE fileId = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerIsoMetadata: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, fileIdentifier, strlen (fileIdentifier),
			     SQLITE_STATIC);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      exists = 1;
		      id = sqlite3_column_int64 (stmt, 0);
		  }
	    }
	  sqlite3_finalize (stmt);
      }

    if (exists)
      {
	  /* update */
	  sql = "UPDATE ISO_metadata SET md_scope = ?, metadata = ? "
	      "WHERE id = ?";
      }
    else
      {
	  /* insert */
	  sql = "INSERT INTO ISO_metadata "
	      "(id, md_scope, metadata) VALUES (?, ?, ?)";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerIsoMetadata: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  sqlite3_bind_text (stmt, 1, scope, strlen (scope), SQLITE_STATIC);
	  sqlite3_bind_blob (stmt, 2, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, id);
      }
    else
      {
	  /* insert */
	  if (id < 0)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_int64 (stmt, 1, id);
	  sqlite3_bind_text (stmt, 2, scope, strlen (scope), SQLITE_STATIC);
	  sqlite3_bind_blob (stmt, 3, p_blob, n_bytes, SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerIsoMetadata() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

#endif /* end including LIBXML2 */
