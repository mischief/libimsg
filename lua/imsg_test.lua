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
-- fd() is idempotent
assert(msg:fd() == gotfd)
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

-- fileno reports the socket
assert(buf0:fileno() == p0)
assert(buf1:fileno() == p1)

-- queuelen tracks messages waiting to be written
assert(buf0:queuelen() == 0)
buf0:compose(typ, id, 0, -1, "queued")
assert(buf0:queuelen() == 1)
buf0:compose(typ, id, 0, -1, "queued too")
assert(buf0:queuelen() == 2)
buf0:flush()
assert(buf0:queuelen() == 0)

-- readlen tracks messages received but not yet handed out
assert(buf1:readlen() == 0)
buf1:read()
assert(buf1:readlen() == 2)
assert(buf1:get():data() == "queued")
assert(buf1:readlen() == 1)
assert(buf1:get():data() == "queued too")
assert(buf1:readlen() == 0)

-- an unclaimed fd is closed with the imsg
local probe = posix_unistd.dup(0)
posix_unistd.close(probe)
buf0:compose(typ, id, 0, posix_unistd.dup(0), "drop")
buf0:flush()
buf1:read()
msg = buf1:get()
msg = nil
collectgarbage()
collectgarbage()
-- the dropped fd is free again, so dup hands back the same number
local again = posix_unistd.dup(0)
assert(again == probe)
posix_unistd.close(again)

-- close releases the imsgbuf but leaves the socket open
buf3:close()
assert(buf3:fileno() == -1)
-- closing twice is fine
buf3:close()
assert(not pcall(function() return buf3:queuelen() end))
-- the socket is still ours to close
assert(posix_unistd.close(p3))

-- close(true) closes the socket too
buf2:close(true)
assert(buf2:fileno() == -1)
assert(not posix_unistd.close(p2))
