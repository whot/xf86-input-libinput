AM_CPPFLAGS = $(XORG_CFLAGS) \
	      $(CWARNFLAGS) \
	      -I$(top_srcdir)/include \
	      -I$(top_srcdir)/src

tests = test-draglock

noinst_PROGRAMS = $(tests)

test_draglock_SOURCES = test-draglock.c
test_draglock_LDADD = ../src/libdraglock.la

TESTS = $(tests)
