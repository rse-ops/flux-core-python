AM_CFLAGS = \
	$(WARNING_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS)

AM_LDFLAGS = \
	$(CODE_COVERAGE_LIBS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/common/libccan \
	-I$(top_builddir)/src/common/libflux \
	$(FLUX_SECURITY_CFLAGS) \
	$(JANSSON_CFLAGS)

fluxmod_LTLIBRARIES = job-list.la

job_list_la_SOURCES = \
	job-list.c \
	job-list.h \
	job_state.h \
	job_state.c \
	job_data.h \
	job_data.c \
	list.h \
	list.c \
	job_util.h \
	job_util.c \
	idsync.h \
	idsync.c \
	stats.h \
	stats.c

job_list_la_LDFLAGS = $(fluxmod_ldflags) -module
job_list_la_LIBADD = \
	$(top_builddir)/src/common/libjob/libjob.la \
	$(top_builddir)/src/common/libflux-internal.la \
	$(top_builddir)/src/common/libflux-core.la \
	$(top_builddir)/src/common/libflux-optparse.la \
	$(top_builddir)/src/common/librlist/librlist.la \
	$(JANSSON_LIBS) \
	$(HWLOC_LIBS)

TESTS = \
	test_job_data.t

test_ldadd = \
	$(top_builddir)/src/common/libtap/libtap.la \
	$(top_builddir)/src/common/libjob/libjob.la \
	$(top_builddir)/src/common/librlist/librlist.la \
	$(top_builddir)/src/common/libflux-internal.la \
	$(top_builddir)/src/common/libflux-core.la \
	$(JANSSON_LIBS) \
	$(HWLOC_LIBS)

test_cppflags = \
	$(AM_CPPFLAGS)

test_ldflags = \
	-no-install

check_PROGRAMS = $(TESTS)

TEST_EXTENSIONS = .t
T_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
       $(top_srcdir)/config/tap-driver.sh

test_job_data_t_SOURCES = test/job_data.c
test_job_data_t_CPPFLAGS = \
	-DTEST_SRCDIR=\"$(top_srcdir)/src/modules/job-list/test\" \
	$(test_cppflags)
test_job_data_t_LDADD = \
	$(top_builddir)/src/modules/job-list/job_data.o \
	$(test_ldadd)
test_job_data_t_LDFLAGS = \
	$(test_ldflags)

EXTRA_DIST = \
	test/R/1node_1core.R \
	test/R/1node_4core.R \
	test/R/4node_1core.R \
	test/R/4node_4core.R \
	test/R/invalid_R_lite.R \
	test/R/invalid_json.R \
	test/R/invalid_nodelist.R \
	test/R/invalid_version.R \
	test/R/missing_R_lite.R \
	test/R/missing_expiration.R \
	test/R/missing_nodelist.R \
	test/R/missing_starttime.R \
	test/R/missing_version.R \
	test/jobspec/1node.jobspec \
	test/jobspec/1node_1slot_nonexclusive.jobspec \
	test/jobspec/1node_perresourcecore4.jobspec \
	test/jobspec/1node_perresourcenode4.jobspec \
	test/jobspec/1slot.jobspec \
	test/jobspec/1slot_4core.jobspec \
	test/jobspec/1slot_perresourcecore4.jobspec \
	test/jobspec/4node.jobspec \
	test/jobspec/4node_1slot_nonexclusive.jobspec \
	test/jobspec/4node_4slot_nonexclusive.jobspec \
	test/jobspec/4node_perresourcecore4.jobspec \
	test/jobspec/4node_perresourcenode4.jobspec \
	test/jobspec/4slot.jobspec \
	test/jobspec/4slot_perresourcecore4.jobspec \
	test/jobspec/duration_alt.jobspec \
	test/jobspec/invalid_attributes_system_job.jobspec \
	test/jobspec/invalid_command_array.jobspec \
	test/jobspec/invalid_command_string.jobspec \
	test/jobspec/invalid_json.jobspec \
	test/jobspec/invalid_per_resource_missing_type.jobspec \
	test/jobspec/invalid_resources.jobspec \
	test/jobspec/invalid_resources_invalid_count.jobspec \
	test/jobspec/invalid_resources_invalid_type.jobspec \
	test/jobspec/invalid_resources_missing_count.jobspec \
	test/jobspec/invalid_resources_missing_type.jobspec \
	test/jobspec/invalid_resources_nocores.jobspec \
	test/jobspec/invalid_resources_noslots.jobspec \
	test/jobspec/invalid_tasks_array.jobspec \
	test/jobspec/invalid_tasks_missing_command.jobspec \
	test/jobspec/invalid_version.jobspec \
	test/jobspec/job_name_alt.jobspec \
	test/jobspec/missing_attributes.jobspec \
	test/jobspec/missing_attributes_system.jobspec \
	test/jobspec/missing_resources.jobspec \
	test/jobspec/missing_tasks.jobspec \
	test/jobspec/missing_version.jobspec
