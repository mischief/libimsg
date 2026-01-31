/// OpenBSD imsg wrapper
// @module imsg
// @author Nick Owens <mischief@offblast.org>
// @license ISC

// SPDX-License-Identifier: ISC

#include <sys/queue.h>
#include <string.h>
#include <errno.h>
#include <imsg.h>

#include <lua.h>
#include <lauxlib.h>

#define IMSGBUF_MT "imsgbuf_mt"
#define IMSG_MT "imsg_mt"

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
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
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

-1 is returned if there is no fd.
@function fd
@treturn int file descriptor
*/
static int
lua_imsg_fd(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	lua_pushinteger(L, imsg_get_fd(msg));
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
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
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
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
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
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
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
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	lua_pushinteger(L, imsg_get_type(msg));
	return 1;
}

/***
Forward this imsg to another imsgbuf.
@function type
@tparam imsgbuf imsgbuf to forward this imsg to
*/
static int
lua_imsg_forward(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

	if(imsg_forward(im, msg) < 0)
		luaL_error(L, "imsg_forward: %s", strerror(errno));

	return 0;
}

static int
lua_imsg_gc(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);

	imsg_free(msg);
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
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);
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
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

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
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

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
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

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
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);
	struct imsg *msg;
	ssize_t rv;

	msg = lua_newuserdata(L, sizeof(*msg));

	switch((rv = imsg_get(im, msg))){
	case 0:
		lua_pushnil(L);
		return 1;
	case -1:
		luaL_error(L, "imsg_get: %s", strerror(errno));
	}

	luaL_setmetatable(L, IMSG_MT);

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
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

	imsgbuf_allow_fdpass(im);

	return 0;
}

/***
Set the maximum message size for this imsgbuf.

Must be at least IMSG_HEADER_SIZE.
@function set_maxsize
*/
static int
lua_imsgbuf_set_maxsize(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);
	int maxsize = luaL_checkinteger(L, 2);
	if(maxsize < IMSG_HEADER_SIZE)
		luaL_error(L, "expected positive integer greater than IMSG_HEADER_SIZE (%d)", IMSG_HEADER_SIZE);

	imsgbuf_set_maxsize(im, (uint32_t) maxsize);

	return 0;
}

static int
lua_imsgbuf_gc(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

	imsgbuf_clear(im);

	return 0;
}

static const luaL_Reg imsgbuf_meta[] = {
	{"compose",	lua_imsgbuf_compose},
	{"write",	lua_imsgbuf_write},
	{"flush",	lua_imsgbuf_flush},
	{"read",	lua_imsgbuf_read},
	{"get",		lua_imsgbuf_get},
	{"allow_fdpass",lua_imsgbuf_allow_fdpass},
	{"set_maxsize", lua_imsgbuf_set_maxsize},
	{"__gc",	lua_imsgbuf_gc},
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
	struct imsgbuf *im;
	int fd = luaL_checkinteger(L, 1);

	im = lua_newuserdata(L, sizeof(*im));
	luaL_setmetatable(L, IMSGBUF_MT);

	if(imsgbuf_init(im, fd) < 0)
		luaL_error(L, "imsgbuf_init");

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
