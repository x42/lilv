/* SLV2
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2008 Krzysztof Foltman
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <librdf.h>
#include <sys/stat.h>
#include "slv2/slv2.h"

static char bundle_dir_name[1024];
static char bundle_dir_uri[1024];
static char manifest_name[1024];
static char content_name[1024];

static SLV2World world;

int test_count  = 0;
int error_count = 0;

void
delete_bundle()
{
	unlink(content_name);
	unlink(manifest_name);
	rmdir(bundle_dir_name);
}

void
init_tests()
{
	strncpy(bundle_dir_name, getenv("HOME"), 900);
	strcat(bundle_dir_name, "/.lv2/slv2-test.lv2");
	sprintf(bundle_dir_uri, "file:///%s/", bundle_dir_name);
	sprintf(manifest_name, "%s/manifest.ttl", bundle_dir_name);
	sprintf(content_name, "%s/plugin.ttl", bundle_dir_name);

	delete_bundle();
}

void
fatal_error(const char *err, const char *arg)
{
	/* TODO: possibly change to vfprintf later */
	fprintf(stderr, err, arg);
	/* IMHO, the bundle should be left in place after an error, for possible investigation */
	/* delete_bundle(); */
	exit(1);
}

void
write_file(const char *name, const char *content)
{
	FILE* f = fopen(name, "w");
	size_t len = strlen(content);
	if (fwrite(content, 1, len, f) != len)
		fatal_error("Cannot write file %s\n", name);
	fclose(f);
}

int
init_world()
{
	world = slv2_world_new();
	return world != NULL;
}

int
load_all_bundles()
{
	if (!init_world())
		return 0;
	slv2_world_load_all(world);
	return 1;
}

int
load_bundle()
{
	SLV2Value uri;
	if (!init_world())
		return 0;
	uri = slv2_value_new_uri(world, bundle_dir_uri);
	slv2_world_load_bundle(world, uri);
	slv2_value_free(uri);
	return 1;
}

void
create_bundle(char *manifest, char *content)
{
	if (mkdir(bundle_dir_name, 0700))
		fatal_error("Cannot create directory %s\n", bundle_dir_name);
	write_file(manifest_name, manifest);
	write_file(content_name, content);
}

int
start_bundle(char *manifest, char *content, int load_all)
{
	create_bundle(manifest, content);
	if (load_all)
		return load_all_bundles();
	else
		return load_bundle();
}

void
unload_bundle()
{
	if (world)
		slv2_world_free(world);
	world = NULL;
}

void
cleanup()
{
	delete_bundle();
}

/*****************************************************************************/

#define TESTCASE(name) { #name, test_##name }
#define TESTITEM(check) do { test_count++; if (!(check)) { error_count++; fprintf(stderr, "Failed: %s\n", #check); } } while(0);

typedef int (*TestFunc) ();

struct TestCase {
	const char *title;
	TestFunc func;
};

#define PREFIX_LINE "@prefix : <http://example.com/> .\n"
#define PREFIX_LV2 "@prefix lv2: <http://lv2plug.in/ns/lv2core#> .\n"
#define PREFIX_RDFS "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
#define PREFIX_FOAF "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
#define PREFIX_DOAP "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"

#define MANIFEST_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDFS
#define BUNDLE_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDFS PREFIX_FOAF PREFIX_DOAP
#define PLUGIN_NAME(name) "doap:name \"" name "\""
#define LICENSE_GPL "doap:license <http://usefulinc.com/doap/licenses/gpl>"

static char *uris_plugin = "http://example.com/plug";
static SLV2Value plugin_uri_value, plugin2_uri_value;

/*****************************************************************************/

void
init_uris()
{
	plugin_uri_value = slv2_value_new_uri(world, uris_plugin);
	plugin2_uri_value = slv2_value_new_uri(world, "http://example.com/foobar");
	TESTITEM(plugin_uri_value);
	TESTITEM(plugin2_uri_value);
}

void
cleanup_uris()
{
	slv2_value_free(plugin2_uri_value);
	slv2_value_free(plugin_uri_value);
	plugin2_uri_value = NULL;
	plugin_uri_value = NULL;
}

/*****************************************************************************/

int
test_utils()
{
	TESTITEM(!strcmp(slv2_uri_to_path("file:///tmp/blah"), "/tmp/blah"));
	TESTITEM(!slv2_uri_to_path("file:/example.com/blah"));
	TESTITEM(!slv2_uri_to_path("http://example.com/blah"));
	return 1;
}

/*****************************************************************************/

int
test_value()
{
	SLV2Value v1, v2, v3;
	const char *uri = "http://example.com/";
	char *res;

	init_world();
	v1 = slv2_value_new_uri(world, "http://example.com/");
	TESTITEM(v1);
	TESTITEM(slv2_value_is_uri(v1));
	TESTITEM(!strcmp(slv2_value_as_uri(v1), uri));
	TESTITEM(!slv2_value_is_literal(v1));
	TESTITEM(!slv2_value_is_string(v1));
	TESTITEM(!slv2_value_is_float(v1));
	TESTITEM(!slv2_value_is_int(v1));
	res = slv2_value_get_turtle_token(v1);
	TESTITEM(!strcmp(res, "<http://example.com/>"));

	v2 = slv2_value_new_uri(world, uri);
	TESTITEM(v2);
	TESTITEM(slv2_value_is_uri(v2));
	TESTITEM(!strcmp(slv2_value_as_uri(v2), uri));

	TESTITEM(slv2_value_equals(v1, v2));

	v3 = slv2_value_new_uri(world, "http://example.com/another");
	TESTITEM(v3);
	TESTITEM(slv2_value_is_uri(v3));
	TESTITEM(!strcmp(slv2_value_as_uri(v3), "http://example.com/another"));
	TESTITEM(!slv2_value_equals(v1, v3));

	slv2_value_free(v2);
	v2 = slv2_value_duplicate(v1);
	TESTITEM(slv2_value_equals(v1, v2));
	TESTITEM(slv2_value_is_uri(v2));
	TESTITEM(!strcmp(slv2_value_as_uri(v2), uri));
	TESTITEM(!slv2_value_is_literal(v2));
	TESTITEM(!slv2_value_is_string(v2));
	TESTITEM(!slv2_value_is_float(v2));
	TESTITEM(!slv2_value_is_int(v2));

	slv2_value_free(v3);
	slv2_value_free(v2);
	slv2_value_free(v1);
	return 1;
}

/*****************************************************************************/

int
test_values()
{
	SLV2Value v0;
	SLV2Values vs1;

	init_world();
	v0 = slv2_value_new_uri(world, "http://example.com/");
	vs1 = slv2_values_new();
	TESTITEM(vs1);
	TESTITEM(!slv2_values_size(vs1));
	TESTITEM(!slv2_values_contains(vs1, v0));
	slv2_values_free(vs1);
	slv2_value_free(v0);
	return 1;
}

/*****************************************************************************/

static int discovery_plugin_found = 0;

static bool
discovery_plugin_filter_all(SLV2Plugin plugin)
{
	return true;
}

static bool
discovery_plugin_filter_none(SLV2Plugin plugin)
{
	return false;
}

static bool
discovery_plugin_filter_ours(SLV2Plugin plugin)
{
	return slv2_value_equals(slv2_plugin_get_uri(plugin), plugin_uri_value);
}

static bool
discovery_plugin_filter_fake(SLV2Plugin plugin)
{
	return slv2_value_equals(slv2_plugin_get_uri(plugin), plugin2_uri_value);
}

static void
discovery_verify_plugin(SLV2Plugin plugin)
{
	SLV2Value value = slv2_plugin_get_uri(plugin);
	if (slv2_value_equals(value, plugin_uri_value)) {
		SLV2Value lib_uri = NULL;
		TESTITEM(!slv2_value_equals(value, plugin2_uri_value));
		discovery_plugin_found = 1;
		lib_uri = slv2_plugin_get_library_uri(plugin);
		TESTITEM(lib_uri);
		TESTITEM(slv2_value_is_uri(lib_uri));
		TESTITEM(slv2_value_as_uri(lib_uri));
		TESTITEM(strstr(slv2_value_as_uri(lib_uri), "foo.so"));
		/* this is already being tested as ticket291, but the discovery and ticket291
		 * may diverge at some point, so I'm duplicating it here */
		TESTITEM(slv2_plugin_verify(plugin));
	}
}

int
test_discovery_variant(int load_all)
{
	SLV2Plugins plugins;
	SLV2Plugin explug, explug2;

	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ;"
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:port [ a lv2:ControlPort ; a lv2:InputPort ;"
			" lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ; ] .",
			load_all))
		return 0;

	init_uris();

	/* lookup 1: all plugins (get_all_plugins)
	 * lookup 2: all plugins (get_plugins_by_filter, always true)
	 * lookup 3: no plugins (get_plugins_by_filter, always false)
	 * lookup 4: only example plugin (get_plugins_by_filter)
	 * lookup 5: no plugins (get_plugins_by_filter, non-existing plugin)
	 */
	for (int lookup = 1; lookup <= 5; lookup++) {
		printf("Lookup variant %d\n", lookup);
		int expect_found = 0;
		switch (lookup) {
		case 1:
			plugins = slv2_world_get_all_plugins(world);
			TESTITEM(slv2_plugins_size(plugins) > 0);
			expect_found = 1;
			break;
		case 2:
			plugins =
			    slv2_world_get_plugins_by_filter(world,
			                                     discovery_plugin_filter_all);
			TESTITEM(slv2_plugins_size(plugins) > 0);
			expect_found = 1;
			break;
		case 3:
			plugins =
			    slv2_world_get_plugins_by_filter(world,
			                                     discovery_plugin_filter_none);
			TESTITEM(slv2_plugins_size(plugins) == 0);
			break;
		case 4:
			plugins =
			    slv2_world_get_plugins_by_filter(world,
			                                     discovery_plugin_filter_ours);
			TESTITEM(slv2_plugins_size(plugins) == 1);
			expect_found = 1;
			break;
		case 5:
			plugins =
			    slv2_world_get_plugins_by_filter(world,
			                                     discovery_plugin_filter_fake);
			TESTITEM(slv2_plugins_size(plugins) == 0);
			break;
		}

		explug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
		TESTITEM((explug != NULL) == expect_found);
		explug2 = slv2_plugins_get_by_uri(plugins, plugin2_uri_value);
		TESTITEM(explug2 == NULL);

		if (explug && expect_found) {
			TESTITEM(!strcmp
			         (slv2_value_as_string(slv2_plugin_get_name(explug)),
			          "Test plugin"));
		}

		discovery_plugin_found = 0;
		for (size_t i = 0; i < slv2_plugins_size(plugins); i++)
			discovery_verify_plugin(slv2_plugins_get_at(plugins, i));

		TESTITEM(discovery_plugin_found == expect_found);
		slv2_plugins_free(world, plugins);
		plugins = NULL;
	}

	cleanup_uris();

	return 1;
}

int
test_discovery_load_bundle()
{
	return test_discovery_variant(0);
}

int
test_discovery_load_all()
{
	return test_discovery_variant(1);
}

/*****************************************************************************/

int
test_verify()
{
	SLV2Plugins plugins;
	SLV2Plugin explug;

	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:port [ a lv2:ControlPort ; a lv2:InputPort ;"
			" lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ] .",
			1))
		return 0;

	init_uris();
	plugins = slv2_world_get_all_plugins(world);
	explug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
	TESTITEM(explug);
	slv2_plugin_verify(explug);
	slv2_plugins_free(world, plugins);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

/* add tests here */
static struct TestCase tests[] = {
	TESTCASE(utils),
	TESTCASE(value),
	TESTCASE(values),
	/* TESTCASE(discovery_load_bundle), */
	TESTCASE(verify),
	TESTCASE(discovery_load_all),
	{ NULL, NULL }
};

void
run_tests()
{
	int i;
	for (i = 0; tests[i].title; i++) {
		printf("\n--- Test: %s\n", tests[i].title);
		if (!tests[i].func()) {
			printf("\nTest failed\n");
			/* test case that wasn't able to be executed at all counts as 1 test + 1 error */
			error_count++;
			test_count++;
		}
		unload_bundle();
		cleanup();
	}
}

int
main(int argc, char *argv[])
{
	if (argc != 1) {
		printf("Syntax: %s\n", argv[0]);
		return 0;
	}
	init_tests();
	run_tests();
	cleanup();
	printf("\n--- Results: %d tests, %d errors\n", test_count, error_count);
	return error_count ? 1 : 0;
}

