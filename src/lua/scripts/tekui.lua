
--
--	tek.ui example
--

require "tek.ui"

-------------------------------------------------------------------------------
--
--	custom gadget
--

boing = tek.ui.area:new()

function boing:getborderstyle()
	return tek.ui.area.getborderstyle(self) or tek.ui.borders.simple
end

function boing:isclick()
end

function boing:init(group, app)
	self.boing = { 0.5, 0.5 }
	self.boing[3] = 0.03456
	self.boing[4] = 0.02345
	tek.ui.area.init(self, group, app)
end

function boing:draw(app, drawall)
	local p = app.pens[self.active + 1]
	local v = app.v
	local w = self.rect[3] - self.rect[1] + 1
	local h = self.rect[4] - self.rect[2] + 1
	local x0, y0, x1, y1
	local w2 = w - w/20
	local h2 = h - h/20
	x0 = self.boing[1] * w2 + self.rect[1]
	y0 = self.boing[2] * h2 + self.rect[2]
	tek.ui.area.draw(self, app, drawall)
	v:frect(x0, y0, x0 + w/20 - 1, y0 + h/20 - 1, p[2])
end

function boing:interval(app)
	if self.running == true then
		local b = self.boing
		b[1] = b[1] + b[3]
		b[2] = b[2] + b[4]
		if b[1] <= 0 or b[1] >= 1 then
			b[3] = -b[3]
			b[1] = b[1] + b[3]
		end
		if b[2] <= 0 or b[2] >= 1 then
			b[4] = -b[4]
			b[2] = b[2] + b[4]
		end
		return true -- i want to be redrawn
	end
end

-------------------------------------------------------------------------------
--
--	sample application
--

myboing = boing:new()

app = tek.ui.app:new {
-- 	width = 300, height = 500, title = "Choose file...",
	gui = tek.ui.vgroup:new {
		margin = { 1, 1, 1, 1 },
		childs = {
			tek.ui.hgroup:new {
 				borderstyle = "group",
				childs = {
					tek.ui.button:new { label = "Add", 
						onrelease = function(e, app)
							app:add(tek.ui.button:new { label = "Remove",
								onrelease = function(e, app)
									app:remove(e)
								end,
							}, app.gui.childs[1])
						end,
					},
				},
			},
			tek.ui.hgroup:new {
 				borderstyle = "group", label = "Gruppe",
				childs = {
					tek.ui.vgroup:new {
						hmax = true,
						childs = {
		 					myboing:new { label = "1", hmax = true, vmax = true },
		 					tek.ui.hgroup:new {
		 						childs = {
									tek.ui.button:new { label = "On", hmax = true,
										onrelease = function(e, app)
											myboing.running = true
										end,
									},
									tek.ui.button:new { label = "Off", hmax = true,
										onrelease = function(e, app)
											myboing.running = false
										end,
									},
		 						}
		 					},
		 				},
		 			},
					tek.ui.handle:new { vmax = true },
					tek.ui.listview:new { hmax = true },
 					tek.ui.handle:new { vmax = true },
					tek.ui.listview:new { },
				},
			},
			tek.ui.hgroup:new {
				childs = {
					tek.ui.button:new { label = "Directory", hmax = true },
					tek.ui.button:new { label = "Reload", minw = 75 },
				},
			},
			tek.ui.hgroup:new {
				childs = {
					tek.ui.button:new { label = "Filename", hmax = true },
					tek.ui.button:new { label = "Hidden", minw = 75 },
				},
			},
			tek.ui.hgroup:new {
				align = 1,
				childs = {
-- 					tek.ui.vgroup:new {
-- 						childs = {
-- 							tek.ui.button:new { label = "Eins" },
-- 							tek.ui.button:new { label = "Zwei" },
-- 						},
-- 					},
					tek.ui.button:new { label = "Open" },
					tek.ui.textarea:new { label = "Hallo", hmax = true },
					tek.ui.button:new { label = "Cancel", borderstyle = "button", border = { 4, 2, 4, 2 },
						onrelease = function(e, app)
							abort = true
					 	end,
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

