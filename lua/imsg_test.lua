local posix_socket = require('posix.sys.socket')
local posix_unistd = require('posix.unistd')

local socketpair = posix_socket.socketpair

local imsg = require("imsg")

local p0, p1 = socketpair(posix_socket.AF_UNIX, posix_socket.SOCK_STREAM, 0)

local buf0, buf1 = imsg.new(p0), imsg.new(p1)

buf0:allow_fdpass()
buf0:set_maxsize(imsg.IMSG_HEADER_SIZE+imsg.MAX_IMSGSIZE)

buf1:set_maxsize(imsg.IMSG_HEADER_SIZE+imsg.MAX_IMSGSIZE)

local typ, id, payload = 42, 69, "hello, world!"

buf0:compose(typ, id, 0, -1, payload)
buf0:flush()

buf1:read()
local msg = buf1:get()

-- no fd passed so should be -1
assert(msg:fd() == -1)

assert(msg:len() == #payload)

-- should be our pid since we sent it
assert(msg:pid() == posix_unistd.getpid())

assert(msg:type() == typ)
assert(msg:id() == id)
assert(msg:data() == payload)

-- big msg
payload = string.rep('A', imsg.MAX_IMSGSIZE)
buf0:compose(typ, id, 0, -1, payload)
buf0:flush()

buf1:read()
msg = buf1:get()
assert(msg:len() == #payload)
assert(msg:data() == payload)


-- fd passing
local fd = posix_unistd.dup(0)
buf1:allow_fdpass()
buf0:compose(typ, id, 0, fd, "fd")
buf0:flush()

buf1:read()
msg = buf1:get()
local gotfd = msg:fd()
assert(gotfd ~= -1)
assert(msg:data() == "fd")
posix_unistd.close(gotfd)

-- forwarding
local p2, p3 = socketpair(posix_socket.AF_UNIX, posix_socket.SOCK_STREAM, 0)
local buf2, buf3 = imsg.new(p2), imsg.new(p3)

buf0:compose(typ, id, 0, -1, "forward me")
buf0:flush()
buf1:read()
msg = buf1:get()
msg:forward(buf2)
buf2:flush()

buf3:read()
local fwd = buf3:get()
assert(fwd:type() == typ)
assert(fwd:id() == id)
assert(fwd:data() == "forward me")

-- no more messages queued
assert(buf3:get() == nil)
