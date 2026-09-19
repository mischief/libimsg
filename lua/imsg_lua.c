/// OpenBSD imsg wrapper
// @module imsg
// @author Nick Owens <mischief@offblast.org>
// @license ISC

// SPDX-License-Identifier: ISC

#include <sys/queue.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <imsg.h>

#include <lua.h>
#include <lauxlib.h>

#define IMSGBUF_MT "imsgbuf_mt"
#define IMSG_MT "imsg_mt"

/*
 * An imsgbuf owns no file descriptor: imsgbuf_clear() does not close one.
 * The fd is kept here so fileno() can report it and close() can optionally
 * close it.
 */
struct limsgbuf {
	struct imsgbuf	 buf;
	int		 fd;
	int		 closed;
};

/*
 * imsg_get_fd() hands the descriptor to the caller and forgets it, so it can
 * only answer once. The fd is claimed here when the imsg is created, and
 * closed on collection unless Lua took it with fd().
 */
struct limsg {
	struct imsg	 msg;
	int		 fd;
	int		 claimed;
};

static struct limsgbuf *
checkimsgbuf(lua_State *L, int idx)
{
	struct limsgbuf *lim = luaL_checkudata(L, idx, IMSGBUF_MT);

	if(lim->closed)
		luaL_error(L, "imsgbuf is closed");

	return lim;
}

/***
imsg
@section imsg
*/

/***
Retrive data from an imsg
@function data
@treturn string data from imsg
*/
static int
lua_imsg_data(lua_State *L)
{
	struct imsg *msg = &((struct limsg *)luaL_checkudata(L, 1, IMSG_MT))->msg;
	struct ibuf ibuf;
	size_t left;
	char *p;
	luaL_Buffer b;

	luaL_buffinit(L, &b);

	if(imsg_get_ibuf(msg, &ibuf) < 0)
		luaL_error(L, "imsg_get_ibuf");

	while((left = ibuf_size(&ibuf)) > 0){
		left = left > LUAL_BUFFERSIZE ? LUAL_BUFFERSIZE : left;
		p = luaL_prepbuffer(&b);
		if(ibuf_get(&ibuf, p, left) < 0)
			luaL_error(L, "ibuf_get: %s", strerror(errno));

		luaL_addsize(&b, left);
	}

	luaL_pushresult(&b);
	return 1;
}

/***
Retrive fd from an imsg.

-1 is returned if there is no fd. The same descriptor is returned by every
call. The caller owns the descriptor after the first call and must close it;
a descriptor never asked for is closed with the imsg.
@function fd
@treturn int file descriptor
*/
static int
lua_imsg_fd(lua_State *L)
{
	struct limsg *lmsg = luaL_checkudata(L, 1, IMSG_MT);

	if(lmsg->fd >= 0)
		lmsg->claimed = 1;

	lua_pushinteger(L, lmsg->fd);
	return 1;
}

/***
Retrive id from an imsg.
@function id
@treturn int id
*/
static int
lua_imsg_id(lua_State *L)
{
	struct imsg *msg = &((struct limsg *)luaL_checkudata(L, 1, IMSG_MT))->msg;
	lua_pushinteger(L, imsg_get_id(msg));
	return 1;
}

/***
Retrive length of data.
@function len
@treturn int length
*/
static int
lua_imsg_len(lua_State *L)
{
	struct imsg *msg = &((struct limsg *)luaL_checkudata(L, 1, IMSG_MT))->msg;
	lua_pushinteger(L, imsg_get_len(msg));
	return 1;
}

/***
Retrive PID of the sender of the imsg.
@function pid
@treturn int PID
*/
static int
lua_imsg_pid(lua_State *L)
{
	struct imsg *msg = &((struct limsg *)luaL_checkudata(L, 1, IMSG_MT))->msg;
	lua_pushinteger(L, imsg_get_pid(msg));
	return 1;
}

/***
Retrive type of the imsg.
@function type
@treturn int type
*/
static int
lua_imsg_type(lua_State *L)
{
	struct imsg *msg = &((struct limsg *)luaL_checkudata(L, 1, IMSG_MT))->msg;
	lua_pushinteger(L, imsg_get_type(msg));
	return 1;
}

/***
Forward this imsg to another imsgbuf.

The imsg is queued for sending on the target imsgbuf.
@function forward
@tparam imsgbuf imsgbuf imsgbuf to forward this imsg to
@raise errno
*/
static int
lua_imsg_forward(lua_State *L)
{
	struct imsg *msg = &((struct limsg *)luaL_checkudata(L, 1, IMSG_MT))->msg;
	struct imsgbuf *im = &checkimsgbuf(L, 2)->buf;

	if(imsg_forward(im, msg) < 0)
		luaL_error(L, "imsg_forward: %s", strerror(errno));

	return 0;
}

static int
lua_imsg_gc(lua_State *L)
{
	struct limsg *lmsg = luaL_checkudata(L, 1, IMSG_MT);

	imsg_free(&lmsg->msg);

	if(lmsg->fd >= 0 && !lmsg->claimed)
		close(lmsg->fd);
	lmsg->fd = -1;

	return 0;
}

static const luaL_Reg imsg_meta[] = {
	{"data", 	lua_imsg_data},
	{"fd",	 	lua_imsg_fd},
	{"id", 		lua_imsg_id},
	{"len", 	lua_imsg_len},
	{"pid", 	lua_imsg_pid},
	{"type", 	lua_imsg_type},
	{"forward",	lua_imsg_forward},
	{"__gc",	lua_imsg_gc},
	{0, 0}
};

/***
imsgbuf
@section imsgbuf
*/

/***
Compose a new imsg.

The imsg is queued for sending after creation.
@function compose
@int type Type of this message
@int id ID of this message
@int pid 0 means use the current PID.
@int fd -1 means send no file descriptor
@string data data to send
*/
static int
lua_imsgbuf_compose(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;
	int typ = luaL_checkinteger(L, 2);
	int id = luaL_checkinteger(L, 3);
	int pid = luaL_checkinteger(L, 4);
	int fd = luaL_checkinteger(L, 5);
	size_t sz;
	const char *buf = luaL_checklstring(L, 6, &sz);

	if(imsg_compose(im, typ, id, pid, fd, buf, sz) < 0)
		luaL_error(L, "imsg_compose: %s", strerror(errno));

	return 0;
}

/***
Write out queued messages.
@function write
*/
static int
lua_imsgbuf_write(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;

	if(imsgbuf_write(im) < 0)
		luaL_error(L, "imsgbuf_write: %s", strerror(errno));

	return 0;
}

/***
Flush queued messages.

Calls @{write} in a loop until all imsgs in the output buffer are sent.

Should not be called on non-blocking sockets.
@function flush
*/
static int
lua_imsgbuf_flush(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;

	if(imsgbuf_flush(im) < 0)
		luaL_error(L, "imsgbuf_flush: %s", strerror(errno));

	return 0;
}

/***
Read pending data, and queue imsgs into imsgbuf.

Individual imsgs can be retrieved with @{get}.
@function read
*/
static int
lua_imsgbuf_read(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;

	errno = 0;

	switch(imsgbuf_read(im)){
	case 0:
		lua_pushnil(L);
		return 1;
	case -1:
		luaL_error(L, "imsgbuf_read: %s", strerror(errno));
	}

	lua_pushboolean(L, 1);

	return 1;
}

/***
Get an imsg from imsgbuf.

If no messages are ready, returns nil.
@function get
@treturn[1] imsg returned @{imsg}
@treturn[2] nil if there's no messages left
@raise errno
*/
static int
lua_imsgbuf_get(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;
	struct limsg *lmsg;

	lmsg = lua_newuserdata(L, sizeof(*lmsg));
	lmsg->fd = -1;
	lmsg->claimed = 0;

	switch(imsg_get(im, &lmsg->msg)){
	case 0:
		lua_pushnil(L);
		return 1;
	case -1:
		luaL_error(L, "imsg_get: %s", strerror(errno));
	}

	luaL_setmetatable(L, IMSG_MT);
	lmsg->fd = imsg_get_fd(&lmsg->msg);

	return 1;
}

/***
Enable file descriptor passing.

Allows file descriptor passing in both directions for this imsgbuf.
@function allow_fdpass
*/
static int
lua_imsgbuf_allow_fdpass(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;

	imsgbuf_allow_fdpass(im);

	return 0;
}

/***
Set the maximum message size for this imsgbuf.

Must be at least IMSG_HEADER_SIZE.
@function set_maxsize
@int maxsize maximum message size, including the header
@raise errno
*/
static int
lua_imsgbuf_set_maxsize(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;
	int maxsize = luaL_checkinteger(L, 2);
	if(maxsize < IMSG_HEADER_SIZE)
		luaL_error(L, "expected positive integer greater than IMSG_HEADER_SIZE (%d)", IMSG_HEADER_SIZE);

	imsgbuf_set_maxsize(im, (uint32_t) maxsize);

	return 0;
}

/***
Number of messages waiting to be written.

A write leaves messages queued when the socket is full. An event loop uses
this to decide whether it must wait for the socket to become writable again.
@function queuelen
@treturn int number of queued messages
*/
static int
lua_imsgbuf_queuelen(lua_State *L)
{
	struct imsgbuf *im = &checkimsgbuf(L, 1)->buf;

	lua_pushinteger(L, imsgbuf_queuelen(im));

	return 1;
}

/***
File descriptor of this imsgbuf.

-1 is returned once the imsgbuf is closed.
@function fileno
@treturn int file descriptor
*/
static int
lua_imsgbuf_fileno(lua_State *L)
{
	struct limsgbuf *lim = luaL_checkudata(L, 1, IMSGBUF_MT);

	lua_pushinteger(L, lim->closed ? -1 : lim->fd);

	return 1;
}

static void
imsgbuf_close(struct limsgbuf *lim, int closefd)
{
	if(lim->closed)
		return;

	imsgbuf_clear(&lim->buf);
	lim->closed = 1;

	if(closefd && lim->fd >= 0)
		close(lim->fd);
	lim->fd = -1;
}

/***
Release this imsgbuf.

Queued messages are discarded. Further calls on the imsgbuf raise an error.
Closing twice is not an error.

The imsgbuf does not own its file descriptor, so the descriptor is left open
unless closefd is true.
@function close
@bool[opt=false] closefd also close the file descriptor
*/
static int
lua_imsgbuf_close(lua_State *L)
{
	struct limsgbuf *lim = luaL_checkudata(L, 1, IMSGBUF_MT);

	imsgbuf_close(lim, lua_toboolean(L, 2));

	return 0;
}

static int
lua_imsgbuf_gc(lua_State *L)
{
	struct limsgbuf *lim = luaL_checkudata(L, 1, IMSGBUF_MT);

	imsgbuf_close(lim, 0);

	return 0;
}

#if LUA_VERSION_NUM >= 504
/* __close gets the error object as its second argument, never a flag. */
static int
lua_imsgbuf_closemeta(lua_State *L)
{
	struct limsgbuf *lim = luaL_checkudata(L, 1, IMSGBUF_MT);

	imsgbuf_close(lim, 0);

	return 0;
}
#endif

static const luaL_Reg imsgbuf_meta[] = {
	{"compose",	lua_imsgbuf_compose},
	{"write",	lua_imsgbuf_write},
	{"flush",	lua_imsgbuf_flush},
	{"read",	lua_imsgbuf_read},
	{"get",		lua_imsgbuf_get},
	{"allow_fdpass",lua_imsgbuf_allow_fdpass},
	{"set_maxsize", lua_imsgbuf_set_maxsize},
	{"queuelen",	lua_imsgbuf_queuelen},
	{"fileno",	lua_imsgbuf_fileno},
	{"close",	lua_imsgbuf_close},
	{"__gc",	lua_imsgbuf_gc},
#if LUA_VERSION_NUM >= 504
	{"__close",	lua_imsgbuf_closemeta},
#endif
	{0, 0}
};

/***
Functions
@section functions
*/

/***
Create a new imsgbuf
@function new
@int fd file descriptor
@treturn imsgbuf a new imsgbuf
*/
static int
lua_imsgbuf_new(lua_State *L)
{
	struct limsgbuf *lim;
	int fd = luaL_checkinteger(L, 1);

	lim = lua_newuserdata(L, sizeof(*lim));
	lim->fd = fd;
	lim->closed = 1;	/* nothing to clear until imsgbuf_init succeeds */
	luaL_setmetatable(L, IMSGBUF_MT);

	if(imsgbuf_init(&lim->buf, fd) < 0)
		luaL_error(L, "imsgbuf_init");
	lim->closed = 0;

	return 1;
}

static luaL_Reg const eventlib[] = {
	{ "new", lua_imsgbuf_new},
	{ 0, 0 }
};

/***
Constants
@section constants
*/

/***
@table constants
@int IMSG_HEADER_SIZE Size of the imsg message header.
@int MAX_IMSGSIZE Default maximum imsg size.
*/

int
luaopen_imsg(lua_State* L)
{
	luaL_newlib(L, eventlib);

	lua_pushinteger(L, IMSG_HEADER_SIZE);
	lua_setfield(L, -2, "IMSG_HEADER_SIZE");

	lua_pushinteger(L, MAX_IMSGSIZE);
	lua_setfield(L, -2, "MAX_IMSGSIZE");

	luaL_newmetatable(L, IMSGBUF_MT);
	luaL_setfuncs(L, imsgbuf_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, IMSG_MT);
	luaL_setfuncs(L, imsg_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}
