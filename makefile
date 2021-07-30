CC = g++
CFLAGS = -Wall -pthread  -g

LIBDIR = Lib
BINDIR = Bin
OBJDIR = $(BINDIR)/obj
TARGET = $(BINDIR)/execute

INCLUDE += 	header/cond \
			header/event \
			header/http \
			header/log	\
			header/mutex \
			header/queue \
			header/ISocket \
			header/threadpool \
			header/authenHandler \
			header/utils

SRCDIR += src
SRCDIR += src/cond
SRCDIR += src/event
SRCDIR += src/http
SRCDIR += src/mutex
SRCDIR += src/ISocket
SRCDIR += src/threadpool

include allrules.mk
