-------------------------------------------------------------------------------
--
--	$Id: dirscan.lua,v 1.1 2006/08/25 02:39:34 tmueller Exp $
--	teklib/mods/lua/scripts/dirscan.lua -
--	demonstrates TEKlib argument parsing and directory examination
--
--	Written by Timm S. Mueller <tmueller at neoscientists.org>
--	See copyright notice in teklib/COPYRIGHT
--

-------------------------------------------------------------------------------

require "tek"

local state = {}

local function say(...)
	tek.stdout:write(unpack(arg))
end

local function sayentry(indent, type, name, time, size)
	local tt = ""
	if state.long then
		tt = tek.date("*t", time)
		tt = string.format("% 8d %04d-%02d-%02d %02d:%02d ",
			size or 0, tt.year, tt.month, tt.day, tt.hour, tt.min)
	end
	local aa = ""
	if state.all then
		aa = string.rep("|  ", indent) .. "+- "
	end
	if type == 2 then
		say(tt, aa, name, "/\n")
	else
		say(tt, aa, name, "\n")
	end
end

local function recurse(dirname, indent)
	local lock = tek.lock(dirname)
	if not lock then
		say("*** Could not lock ", dirname, ".\n")
		return 10
	end
	local type, size, date = lock:examine()
	if type ~= 2 then -- not directory
		sayentry(indent, 1, dirname, date, size)
		return
	end
	local list = {}
	while 1 do
		local name, type, size, date = lock:exnext()
		if not name then
			break -- end of current directory
		end
		if type == 2 then -- entry is directory
			if state.sort then
				table.insert(list, { name = name, type = 2, date = date})
			else
				sayentry(indent, type, name, date)
				if state.all then -- enter recursion
					local ret = recurse(tek.addpart(dirname, name), indent + 1)
					if ret then 
						return ret 
					end
				end
			end
		elseif not state.dironly then
			if state.sort then
				table.insert(list,
					{ name = name, type = 1, date = date, size = size})
			else
				sayentry(indent, type, name, date, size)
			end
		end
	end
	if state.sort then
		table.sort(list, function(a, b)
			if a.type == b.type then
				return string.lower(a.name) < string.lower(b.name)
			end
			if a.type > b.type then
				return true
			end
			return false
		end)
		for _, v in ipairs(list) do
			sayentry(indent, v.type, v.name, v.date, v.size)
			if state.all and v.type == 2 then
				local ret = recurse(tek.addpart(dirname, v.name), indent + 1)
				if ret then
					return ret 
				end
			end
		end
	end
end

-------------------------------------------------------------------------------

local ret = 20
local tmpl = "-f=FROM/M,-r=ALL/S,-s=SORT/S,-d=DIRONLY/S,-l=LONG/S,-h=HELP/S"
local success, from, all, sort, dironly, long, help = 
	tek.readargs(tmpl, unpack(arg))

if success and not help then
	state.all = all
	state.sort = sort
	state.dironly = dironly
	state.long = long
	ret = 0
	if #from == 0 then
		recurse("", 0)
	else
		for i, dirname in ipairs(from) do
			if #from > 1 then
				if i > 1 then
					say("\n")
				end
				say(dirname, ":\n")
			end
			local err = recurse(dirname, 0)
			if err then
				ret = err
			end
		end
	end
else
	say("dirscan.lua ", tmpl, "\n")
end

return ret
