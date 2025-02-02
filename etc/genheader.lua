#!/usr/bin/env lua

--
--	$Id: genheader.lua,v 1.1 2006/08/25 02:36:51 tmueller Exp $
--	teklib/bin/genheader.lua - TEKlib header generator
--
--	Written by Timm S. Mueller <tmueller at neoscientists.org>
--	See copyright notice in teklib/COPYRIGHT
--

local Args = require "tek.lib.args"
local insert = table.insert

-- utils ----------------------------------------------------------------------

local say = io.write

local function trim(s)
	return s:gsub("^%s*(.-)%s*$", "%1")
end

local function gettypesym(arg)
	-- check for pointer types (struct Foo *bar, TTAGITEM*, ...)
	local type, sym = arg:match("^.-%s*(.-%s*%*+)%s*([%w_]*)$")
	type = type or arg:match("^.-%s*(.-%s*%*+)%s*$")
	if not type then
		type, sym = arg:match("^([%w_]*)%s*([%w_]-)$")
		type = type or arg:match("^([%w_]*)%s*$")
	end
	return type, sym
end

local function getargs(s, startpos)
	local tab = { }
	if startpos then
		s = s:sub(startpos)
	end
	for arg in s:gmatch("%s*([^,]+)%s*[, ]?") do
		insert(tab, arg)
	end
	return tab
end


-- stdcall generator ----------------------------------------------------------

local gen_stdcall = {

	init = function(state)
		state.out([[
#ifndef _TEK_STDCALL_]] .. state.filename:upper() .. [[_H
#define _TEK_STDCALL_]] .. state.filename:upper() .. [[_H

/*
**	teklib/tek/stdcall/]] .. state.filename .. ".h - " .. state.name ..
	" module interface\n")

		if state.author or state.copyright then
			state.out("**\n")
		end
		if state.author then
			state.out("**\tWritten by " .. state.author .. "\n")
		end
		if state.copyright then
			state.out("**\t" .. state.copyright .. "\n")
		end
		state.out([[
*/

]])
	end,

	func = function(state, func, ret, funcargs)
		if not state.private then
			local more
			state.out("#define " .. state.prefix .. func .. "(" .. state.sym)
			table.foreachi(funcargs, function(_, arg)
				state.out("," .. arg.sym)
			end)
			state.out(") \\\n\t(*(((TMODCALL " .. ret .. "(**)(" .. state.type)
			table.foreachi(funcargs, function(_, arg)
				local type, sym = arg.name, arg.sym
				state.out("," .. type)
			end)
			state.out("))(" .. state.base .. "))[-" .. state.offset ..
				"]))(" .. state.sym)
			table.foreachi(funcargs,
				function(_, arg) state.out("," .. arg.sym)
			end)
			state.out(")\n\n")
		end
	end,

	comment = function(state, text)
		state.out("/*" .. text .. "*/\n\n")
	end,

	exit = function(state)
		state.out("#endif /* _TEK_STDCALL_" .. state.filename:upper() ..
			"_H */\n")
	end,
}


-- inline generator -----------------------------------------------------------

local gen_inline = {

	init = function(state)
		state.out([[
#ifndef _TEK_INLINE_]] .. state.filename:upper() .. [[_H
#define _TEK_INLINE_]] .. state.filename:upper() .. [[_H

/*
**	teklib/tek/inline/]] .. state.filename .. ".h - " .. state.name ..
	" inline calls\n")

		if state.author or state.copyright then
			state.out("**\n")
		end
		if state.author then
			state.out("**\tWritten by " .. state.author .. "\n")
		end
		if state.copyright then
			state.out("**\t" .. state.copyright .. "\n")
		end
		state.out([[
*/

#include <tek/proto/]] .. state.filename .. [[
.h>

]])
	end,

	func = function(state, func, ret, funcargs)
		if not state.private and state.inline then
			local more
			state.out("#define " .. state.iprefix .. func .. "(")
			table.foreachi(funcargs, function(_, arg)
				if more then
					state.out("," .. arg.sym)
				else
					state.out(arg.sym)
					more = true
				end
			end)
			state.out(") \\\n\t(*(((TMODCALL " .. ret .. "(**)(" .. state.type)
			table.foreachi(funcargs, function(_, arg)
				local type, sym = arg.name, arg.sym
				state.out("," .. type)
			end)
			state.out("))(" .. state.isym .. "))[-" .. state.offset ..
				"]))(" .. state.isym)
			table.foreachi(funcargs,
				function(_, arg) state.out("," .. arg.sym)
			end)
			state.out(")\n\n")
		end
	end,

	exit = function(state)
		state.out("#endif /* _TEK_INLINE_" .. state.filename:upper() ..
			"_H */\n")
	end,
}

-- iface generator ------------------------------------------------------------

local gen_iface = {

	init = function(state)
		state.out([[
#ifndef _TEK_IFACE_]] .. state.filename:upper() .. [[_H
#define _TEK_IFACE_]] .. state.filename:upper() .. [[_H

/*
**	teklib/tek/iface/]] .. state.filename .. ".h - " .. state.name ..
	" interface\n")

		if state.author or state.copyright then
			state.out("**\n")
		end
		if state.author then
			state.out("**\tWritten by " .. state.author .. "\n")
		end
		if state.copyright then
			state.out("**\t" .. state.copyright .. "\n")
		end
		state.out([[
*/

#include <tek/exec.h>
#include <tek/proto/]] .. state.filename .. [[
.h>

struct ]] .. state.prefix .. [[IFace
{
	struct TInterface IFace;
]])
	end,

	func = function(state, func, ret, funcargs)
		if not state.private and state.interface then
			local arg = { state.ibase }
			for k, v in ipairs(funcargs) do
				local type, sym = v.name, v.sym
				if type:sub(-1) ~= "*" then
					type = type .. " "
				end
				insert(arg, ("%s%s"):format(type, sym))
			end
			arg = table.concat(arg, ", ")
			if ret:sub(-1) ~= "*" then
				ret = ret .. " "
			end
			state.out(("\tTMODCALL %s(*%s)(%s);\n"):format(ret, func, arg))
		end
	end,

	comment = function(state, text)
		if not state.private and state.interface then
			state.out("\t/*" .. text .. "*/\n")
		end
	end,

	exit = function(state)
		state.out([[
};

#endif /* _TEK_IFACE_]] .. state.filename:upper() .. [[ */
]])
	end,
}

-- linklib generator ----------------------------------------------------------

local gen_linklib = {

	init = function(state)
		state.out([[
#ifndef _TEK_]] .. state.filename:upper() .. [[_H
#define _TEK_]] .. state.filename:upper() .. [[_H

/*
**	teklib/tek/]] .. state.filename .. ".h - " .. state.name ..
	" link library functions\n")

		if state.author or state.copyright then
			state.out("**\n")
		end
		if state.author then
			state.out("**\tWritten by " .. state.author .. "\n")
		end
		if state.copyright then
			state.out("**\t" .. state.copyright .. "\n")
		end
		state.out([[
*/

#include <tek/exec.h>

#ifdef __cplusplus
extern "C" {
#endif

]])
	end,

	func = function(state, func, ret, funcargs)
		if not state.private then
			local arg = { }
			for k, v in ipairs(funcargs) do
				local type, sym = v.name, v.sym
				if type:sub(-1) ~= "*" then
					type = type .. " "
				end
				insert(arg, ("%s%s"):format(type, sym))
			end
			arg = table.concat(arg, ", ")
			if ret:sub(-1) ~= "*" then
				ret = ret .. " "
			end
			state.out(("TLIBAPI %s%s(%s);\n"):format(ret, func, arg))
		end
	end,

	comment = function(state, text)
		if not state.private and state.interface then
			state.out("\t/*" .. text .. "*/\n")
		end
	end,

	exit = function(state)
		state.out([[

#ifdef __cplusplus
}
#endif

#endif /* _TEK_]] .. state.filename:upper() .. [[ */
]])
	end,
}

-- parser ---------------------------------------------------------------------

local function genheader(state, ...)
	local arg = { ... }
	arg.n = select("#", ...)

	local function err(text)
		say("*** Error in line " .. state.lnr .. ": " .. text .. "\n")
		error()
	end

	local function dowriters(tabname, ...)
		for _, f in ipairs(state.genfuncs) do
			if f[tabname] then
				f[tabname](...)
			end
		end
	end

	local function configure()
		state.name, state.filename =
			state.name or state.filename or "unknown",
			state.filename or state.name or "unknown"
		state.prefix = state.prefix or ""
		state.iprefix = state.iprefix or ""
		state.ibase = state.ibase or ("TAPTR " .. (state.name or "base"))
		state.type, state.sym =
			gettypesym(state.this or ("TAPTR " .. (state.name or "base")))
		_, state.isym = gettypesym(state.ibase)
		state.type, state.sym =
			state.type or "TAPTR", state.sym or "base"
		state.base = state.base or state.sym
	end

	-- handle control words

	local function f_set(pat, var, ...)
		local n = select("#", ...)
		if n == 0 then
			state[var] = nil
			return
		elseif n == 1 then
			local s = select(1, ...):match(pat)
			if s then
				state[var] = s
				return
			end
		end
		return false
	end

	local function f_offset(...)
		if select("#", ...) == 1 then
			local s = select(1, ...):match("^(%d+)$")
			if s then
				state.offset = tonumber(s)
				return
			end
		end
		return false
	end

	local function f_toggle(var, keyon, keyoff, ...)
		local n = select("#", ...)
		if n == 0 then
			state[var] = true
			return
		elseif n == 1 then
			local s = select(1, ...):lower()
			if s == keyon then
				state[var] = true
				return
 			elseif s == keyoff then
				state[var] = nil
				return
			end
		end
		return false
	end

	local function f_def(pat, tab, ...)
		for _, v in ipairs { ... } do
			if not v:match(pat) then
				err("illegal type '"..v.."'\n")
			end
			state.types.all[v] = v
			state.types[tab][v] = v
		end
	end

	local ctrlwords = {
		class = function(...) end,
		name = function(...) return f_set("^([%a][%w_]*)$", "name", ...) end,
		filename = function(...) return f_set("^([%a][%w_]*)$", "filename", ...) end,
		base = function(...) return f_set("^([%a_][%w_%s%(%)]*)$", "base", ...) end,
		this = function(...) return f_set("^([%a_][%w_%s%(%)%*]*)$", "this", ...) end,
		author = function(...) return f_set("^(.*)$", "author", ...) end,
		copyright = function(...) return f_set("^(.*)$", "copyright", ...) end,
		prefix = function(...) return f_set("^([%a_][%w_]*)$", "prefix", ...) end,
		defv = function(...) f_def("^([%a_][%w%s_]*)$", "v", ...) end,
		defp = function(...) f_def("^([%a_][%w%s_]*%s*%**)$", "p", ...) end,
		defi = function(...) f_def("^([%a_][%w%s_]*)$", "i", ...) end,
		defs = function(...) f_def("^([%a_][%w%s_]*)$", "s", ...) end,
		deff = function(...) f_def("^([%a_][%w%s_]*)$", "f", ...) end,
		defl = function(...) f_def("^([%a_][%w%s_]*)$", "l", ...) end,
		defd = function(...) f_def("^([%a_][%w%s_]*)$", "d", ...) end,
		defb = function(...) f_def("^([%a_][%w%s_]*)$", "b", ...) end,
		deft = function(...) f_def("^([%a_][%w%s_]*)$", "t", ...) end,
		offset = f_offset,
		private = function(...) return f_toggle("private", "on", "off", ...) end,
		public = function(...) return f_toggle("public", "off", "on", ...) end,
		inline = function(...) return f_toggle("inline", "on", "off", ...) end,
		interface = function(...) return f_toggle("interface", "on", "off", ...) end,
		iprefix = function(...) return f_set("^([%a_][%w_]*)$", "iprefix", ...) end,
		ibase = function(...) return f_set("^([%a_][%w%s_]*)$", "ibase", ...) end,
		["end"] = function(...) state.parse = false end,
	}

	-- parse

	state.lnr = 1
	state.offset = 1
	state.parse = true
	state.types = { all = {}, v = {}, p = {}, i = {}, s = {},
		f = {}, l = {}, d = {}, b = {}, t = {} }

	for line in state.infile:lines() do
		if state.parse then
			-- try to parse as C comment
			local comment = line:match("^%s*/%*(.*)%*/%s*$")
			if comment then
				if not state.first then
					configure()
					dowriters("init", state)
					state.first = true
				end
				dowriters("comment", state, comment)
			else
				-- remove comments and trim
				local s = trim(line:gsub("^(.-);.-$", "%1"))
				if s ~= "" then
					-- try to parse as control statement
					local _, apos, ctrl = s:find("^%.(%w*)")
					if ctrl then
						local ctrlargs = getargs(s, apos + 1)
						local f = ctrlwords[ctrl]
						if f then
							if f(unpack(ctrlargs)) == false then
								err("illegal arguments to '." ..
									ctrl .. "'")
							end
						else
							err("unknown control keyword '." .. ctrl .."'")
						end
					else
						-- try to parse as function
						local ret, func, args = s:match("^(.-%s+%*%s*)(.-)%s*%((.*)%)")
						if not ret then
							ret, func, args = s:match("^(.*)%s+(.*)%s*%((.*)%)")
						end
						if ret then
							ret = trim(ret)
							if ret:sub(-1) ~= "*" and not state.types.all[ret] then
								err("undefined return type '" .. ret .. "'")
							end
							local rawargs = getargs(args)
							local funcargs, argsyms = {}, { [""] = "" }
							local symchar = 0

							for _, arg in ipairs(rawargs) do
								local t, sym = gettypesym(arg)
								if t:sub(-1) ~= "*"
									and not state.types.all[t] then
									err("undefined type '"..t.."'")
								end
								while argsyms[sym] do
									symchar = symchar + 1
									if symchar > 26 then
										err("too many arguments to function")
									end
									sym = string.char(96 + symchar)
								end
								argsyms[sym] = sym
								insert(funcargs, { name = t, sym = sym })
							end
							if not state.first then
								configure()
								dowriters("init", state)
								state.first = true
							end
							dowriters("func", state, func, ret, funcargs)
							state.offset = state.offset + 1
						else
							err("syntax error")
						end
					end
				end
			end
		end
		state.lnr = state.lnr + 1
	end
	dowriters("exit", state)
end

-- main -----------------------------------------------------------------------

local function handledebug(debug, func, arg)
	if debug then
		func(arg) -- call unprotected
		return true
	else
 		return pcall(func, arg) -- call protected
	end
end

local modes = {
	stdcall = gen_stdcall,
	inline = gen_inline,
	iface = gen_iface,
	linklib = gen_linklib,
}

local TEMPLATE = "-f=FROM/A,-t=TO/K,-m=MODE/K,-d=DEBUG/S,-h=HELP/S"
local args = Args.read(TEMPLATE, arg)
if args and not args["-h"] then
	local fd_in = io.open(args["-f"])
	if fd_in then
		local state = { infile = fd_in, genfuncs = { } }
		if handledebug(args["-d"], genheader, state) then
			-- syntax ok, open out file
			local fd_out
			local dest = args["-t"]
			if dest then
				fd_out = io.open(dest, "w")
			end
			if not dest or fd_out then
				-- rewind infile, rerun
				fd_in:seek("set")
				state = { infile = fd_in, outfile = fd_out or io.stdout,
					genfuncs = { modes[(args["-m"] or "stdcall"):lower()] } }
				state.out = function(...) state.outfile:write(...) end
				if handledebug(args["-d"], genheader, state) then
					-- done.
				else
					say("*** Error in script or during write.\n")
				end
			else
				say("*** Can't open " .. dest .. " for writing.\n")
			end
		else
			say("*** Syntax check failed.\n")
		end
	else
		say("*** Can't open " .. args["-f"] .. " for reading.\n")
	end
else
	say("TEKlib header generator\n")
	say("Usage: " .. TEMPLATE .. "\n")
	say("Specify an interface description file FROM and a conversion type MODE.\n")
	say("The output is written to stdout, unless a file is specified with TO.\n")
	say("Use the DEBUG option to print a stacktrace in case of internal errors.\n")
	say("Modes: STDCALL - create proto header with ANSI-conformant calls [default]\n")
	say("       INLINE  - create module inline header\n")
	say("       IFACE   - create interface header\n")
	say("       LINKLIB - create link library header\n")
end
