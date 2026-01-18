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

