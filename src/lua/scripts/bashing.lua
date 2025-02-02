
--
--	tek.ui example
--	TODO: deinit-method
--

require "tek.ui"

-------------------------------------------------------------------------------
--
--	custom gadget
--

lissa = tek.ui.area:new()

function lissa:getborderstyle()
	return tek.ui.area.getborderstyle(self) or tek.ui.borders.simple
end

function lissa:isclick()
end

function lissa:init(group, app)
	self.numlines = 20
	self.s = { }
	self.d1 = { }
	self.d2 = { }
	for i = 1, 6 do
		self.s[i] = math.random() * math.pi
		self.d1[i] = math.random() * 0.3 + 0.03
		self.d2[i] = math.random() * 0.5 + 0.07
	end
	tek.ui.area.init(self, group, app)
end

function lissa:draw(app, drawall)
	
	local p = app.pens[self.active + 1]
	local v = app.v
	local x = self.rect[1]
	local y = self.rect[2]
	local w = self.rect[3] - x + 1
	local h = self.rect[4] - y + 1
	
	tek.ui.area.draw(self, app, drawall)
	
	local sx, sy = 0.8 * w / 2, 0.8 * h / 2
	local x1, y1
	local a, b, c, d, e, f = unpack(self.s)
	local da, db, dc, dd, de, df = unpack(self.d2)
	
	local sin = math.sin
	local x0 = sin(a) + sin(b) + sin(c) * sx + w / 2 + x
	local y0 = sin(d) + sin(e) + sin(f) * sy + h / 2 + y
	
	for i = 1, self.numlines do
		a, b, c, d, e, f = a + da, b + db, c + dc, d + dd, e + de, f + df
		x1 = sin(a) + sin(b) + sin(c) * sx + w / 2 + x
		y1 = sin(d) + sin(e) + sin(f) * sy + h / 2 + y
		v:line(x0, y0, x1, y1, p[2])
		x0, y0 = x1, y1
	end
end

function lissa:interval(app)
	for i = 1, 6 do
		self.s[i] = self.s[i] + self.d1[i]
		if self.s[i] >= math.pi * 2 then
			self.s[i] = self.s[i] - math.pi * 2
		end
	end
	return true -- want to be redrawn
end

-------------------------------------------------------------------------------
--
--	sample application
--

app = tek.ui.app:new {
	title = "Visual Multibashing",
	gui = tek.ui.vgroup:new {
		margin = { 1, 1, 1, 1 },
		childs = {
			tek.ui.vgroup:new {
 				borderstyle = "group",
				childs = {
					tek.ui.hgroup:new { hmax = true, vmax = true,
						borderstyle = "group",
						childs = {
							lissa:new { hmax = true, vmax = true },
							lissa:new { hmax = true, vmax = true },
							lissa:new { hmax = true, vmax = true },
						},
					},
					tek.ui.hgroup:new { hmax = true, vmax = true,
						borderstyle = "group",
						childs = {
							lissa:new { hmax = true, vmax = true },
							lissa:new { hmax = true, vmax = true },
							lissa:new { hmax = true, vmax = true },
						},
					},
				},
			},
		}
	}
}

-------------------------------------------------------------------------------
--
--	main loop
--

repeat
	app:wait()
	repeat
		local msg = app:getmsg()
		if msg then
			if msg[2] == 1 or
				(msg[2] == 256 and msg[3] == 27) then
				abort = true
			else
				app:passmsg(msg)
			end
		end
	until not msg
until abort

